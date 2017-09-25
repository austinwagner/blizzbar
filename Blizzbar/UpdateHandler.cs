using System;
using System.Collections.Generic;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Management;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using Blizzbar.Interop;
using NLog;

namespace Blizzbar
{
    internal class UpdateHandler : IDisposable
    {
        private const int MmapNameStartOffset = sizeof(ushort);
        private static readonly ILogger Log = LogManager.GetCurrentClassLogger();

        private static readonly int FullMmapNameXOffset =
            MmapNameStartOffset + Strings.MmapNameXOffset * sizeof(ushort);

        private readonly MemoryMappedFile _syncMap;
        private readonly MemoryMappedViewAccessor _syncMapView;

        private readonly Mutex _writeLock;
        private MemoryMappedFile _mmap;

        public UpdateHandler()
        {
            _writeLock = new Mutex(false, Strings.MmapWriteMutexName);

            _syncMap = MemoryMappedFile.CreateNew(Strings.MmapSyncName, MmapNameStartOffset + Strings.MmapNameX.Length,
                MemoryMappedFileAccess.ReadWrite);
            _syncMapView = _syncMap.CreateViewAccessor();

            _mmap = MemoryMappedFile.CreateNew(Strings.MmapName1, sizeof(uint), MemoryMappedFileAccess.ReadWrite);

            // Initialize the synchronization map with a reader count of 0 and MmapName1
            _syncMapView.Write(0, (ushort)0);
            _syncMapView.WriteArray(MmapNameStartOffset, Strings.MmapNameX, 0, Strings.MmapNameX.Length);
            _syncMapView.Write(FullMmapNameXOffset, (ushort)'1');

            // Initialize the main mmap with its null terminator to indicate there are no entries
            using (var mmapView = _mmap.CreateViewAccessor())
            {
                mmapView.Write(0, (uint)0);
            }
        }

        public void Dispose()
        {
            _mmap.Dispose();
            _syncMapView.Dispose();
            _syncMap.Dispose();
            _writeLock.Dispose();
        }

        private static string QueryProcessPath(uint pid, ManagementObjectSearcher searcher)
        {
            // WMI is the best way to do this since a 32-bit process can't directly inspect a 64-bit process
            var query = new SelectQuery("Win32_Process", $"ProcessId = {pid:D} AND ExecutablePath IS NOT NULL",
                new[] {"ExecutablePath"});
            searcher.Query = query;
            var proc = searcher.Get().OfType<ManagementObject>().FirstOrDefault();
            return proc?["ExecutablePath"] as string;
        }

        private static (GameDetails gd, string path) MatchGameDetails(uint pid, ManagementObjectSearcher searcher,
            IEnumerable<GameDetails> gameDetailsCollection)
        {
            try
            {
                var path = QueryProcessPath(pid, searcher);
                if (path == null)
                {
                    Log.Debug("Failed to get path for PID {0:D}. Might not have permission.", pid);
                    return (null, null);
                }

                var gd = gameDetailsCollection.FirstOrDefault(x => Regex.IsMatch(path, x.Regex,
                    RegexOptions.ECMAScript | RegexOptions.IgnoreCase));
                Log.Debug("PID: {0:D}, Path: {1}", pid, path);
                return (gd, path);
            }
            catch (Exception ex)
            {
                Log.Warn(ex, "Failed to get path for PID {0:D}.", pid);
                return (null, null);
            }
        }

        private static (GameDetails gd, string path) LookupGameDetails(IntPtr windowHandle,
            ManagementObjectSearcher searcher,
            IEnumerable<GameDetails> gameDetailsCollection,
            IDictionary<uint, (GameDetails gd, string path)> checkedPids)
        {
            if (Win32.GetWindowThreadProcessId(windowHandle, out var pid) == 0)
            {
                Log.Debug("Could not get PID of window {0}. Probably no longer exists.", pid);
                return (null, null);
            }

            if (!checkedPids.TryGetValue(pid, out var pair))
            {
                pair = MatchGameDetails(pid, searcher, gameDetailsCollection);
                checkedPids[pid] = pair;
            }

            if (pair.gd != null)
            {
                Log.Info("Window {0} of PID {1} belongs to game {2}", windowHandle, pid, pair.gd.ShortName);
            }
            else
            {
                Log.Debug("Window {0} of PID {1} does not belong to a known game", windowHandle, pid);
            }

            return pair;
        }

        private static void SetExistingWindowProps(ICollection<GameDetails> gameDetailsCollection)
        {
            var checkedPids = new Dictionary<uint, (GameDetails gd, string path)>();
            using (var searcher = new ManagementObjectSearcher())
            {
                Win32.EnumWindows((windowHandle, _) =>
                {
                    var pair = LookupGameDetails(windowHandle, searcher, gameDetailsCollection, checkedPids);
                    if (pair.gd == null) return true;

                    Win32.SHGetPropertyStoreForWindow(windowHandle, ref IID.IPropertyStore, out var propStore);

                    try
                    {
                        propStore.SetString(ref PropertyKey.AppUserModel.Id,
                            "BlizzardEntertainment.BattleNet." + pair.gd.ShortName);
                        propStore.SetString(ref PropertyKey.AppUserModel.RelaunchCommand,
                            "battlenet://" + pair.gd.ShortName);
                        propStore.SetString(ref PropertyKey.AppUserModel.RelaunchIconResource,
                            "\"" + pair.path + "\",0");
                        propStore.SetString(ref PropertyKey.AppUserModel.RelaunchDisplayNameResource,
                            Win32.GetWindowText(windowHandle) ?? "Unknown Game");
                        propStore.SetBool(ref PropertyKey.AppUserModel.PreventPinning, false);
                    }
                    catch (Exception ex)
                    {
                        Log.Error(ex, "Failed to set properties on window {0} ({1})", windowHandle,
                            pair.gd.ShortName);
                    }

                    return true;
                }, IntPtr.Zero);
            }
        }

        public Task SetGameDetails(ICollection<GameDetails> gameDetailsCollection)
        {
            return Task.Run(() =>
            {
                MemoryMappedFile newMmap = null;
                try
                {
                    Log.Debug("Taking memory map write lock");
                    _writeLock.WaitOne();
                    Log.Debug("Write lock acquired");

                    var mmapSize =
                        gameDetailsCollection.Sum(x =>
                            sizeof(ushort) + x.Regex.CStrLen() + sizeof(ushort) + x.ShortName.CStrLen()) +
                        sizeof(ushort);

                    string newMmapName;
                    if (_syncMapView.ReadUInt16(FullMmapNameXOffset) == '1')
                    {
                        _syncMapView.Write(FullMmapNameXOffset, (ushort)'2');
                        newMmapName = Strings.MmapName2;
                    }
                    else
                    {
                        _syncMapView.Write(FullMmapNameXOffset, (ushort)'1');
                        newMmapName = Strings.MmapName1;
                    }

                    newMmap = MemoryMappedFile.CreateNew(newMmapName, mmapSize, MemoryMappedFileAccess.ReadWrite);
                    using (var newMmapView = newMmap.CreateViewAccessor())
                    {
                        newMmapView.Write(0, (uint)mmapSize);
                        var offset = sizeof(uint);
                        foreach (var gi in gameDetailsCollection)
                        {
                            WriteCStr(newMmapView, ref offset, gi.Regex);
                            WriteCStr(newMmapView, ref offset, gi.ShortName);
                        }
                        newMmapView.Write(offset, (ushort)0);
                    }

                    _mmap.Dispose();

                    _mmap = newMmap;
                    newMmap = null;
                }
                finally
                {
                    _writeLock.ReleaseMutex();
                    Log.Debug("Write lock released");

                    newMmap?.Dispose();
                }

                SetExistingWindowProps(gameDetailsCollection);
            });
        }

        private static void WriteCStr(UnmanagedMemoryAccessor view, ref int offset, string str)
        {
            view.Write(offset, (ushort)str.Length);
            offset += sizeof(ushort);

            var cstr = str.ToCStr();
            view.WriteArray(offset, cstr, 0, cstr.Length);
            offset += cstr.Length;
        }

        public HookManager InstallHooks() => new HookManager();
    }

    internal static class Extensions
    {
        public static int CStrLen(this string str) => (str.Length + 1) * sizeof(ushort);

        public static byte[] ToCStr(this string str)
        {
            var result = new byte[(str.Length + 1) * sizeof(ushort)];
            Encoding.Unicode.GetBytes(str, 0, str.Length, result, 0);
            result[result.Length - 1] = 0;
            result[result.Length - 2] = 0;
            return result;
        }

        public static void SetString(this IPropertyStore propStore, ref PropertyKey pkey, string str)
        {
            var pv = new PropVariant {vt = VarEnum.VT_BSTR};

            try
            {
                pv.p = Marshal.StringToBSTR(str);
                propStore.SetValue(ref pkey, ref pv);
            }
            finally
            {
                if (pv.p != IntPtr.Zero)
                {
                    Marshal.FreeBSTR(pv.p);
                }
            }
        }

        public static void SetBool(this IPropertyStore propStore, ref PropertyKey pkey, bool b)
        {
            var pv = new PropVariant
            {
                vt = VarEnum.VT_BOOL,
                p2 = b ? -1 : 0
            };

            propStore.SetValue(ref pkey, ref pv);
        }
    }
}