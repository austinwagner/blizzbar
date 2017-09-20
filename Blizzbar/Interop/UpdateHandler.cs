using System;
using System.Collections.Generic;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Text;
using System.Threading;

namespace Blizzbar.Interop
{
    internal class UpdateHandler : IDisposable
    {
        private const int MmapNameStartOffset = sizeof(ushort);

        private static readonly int FullMmapNameXOffset =
            MmapNameStartOffset + Strings.MmapNameXOffset * sizeof(ushort);

        private readonly MemoryMappedFile _syncMap;
        private readonly MemoryMappedViewAccessor _syncMapView;

        private readonly Mutex _writeLock;
        private MemoryMappedFile _mmap;
        private MemoryMappedViewAccessor _mmapView;

        public UpdateHandler()
        {
            _writeLock = new Mutex(false, Strings.MmapWriteMutexName);

            _syncMap = MemoryMappedFile.CreateNew(Strings.MmapSyncName, MmapNameStartOffset + Strings.MmapNameX.Length,
                MemoryMappedFileAccess.ReadWrite);
            _syncMapView = _syncMap.CreateViewAccessor();

            _mmap = MemoryMappedFile.CreateNew(Strings.MmapName1, sizeof(uint), MemoryMappedFileAccess.ReadWrite);
            _mmapView = _mmap.CreateViewAccessor();

            _syncMapView.Write(0, (ushort)0);
            _syncMapView.WriteArray(MmapNameStartOffset, Strings.MmapNameX, 0, Strings.MmapNameX.Length);
            _syncMapView.Write(FullMmapNameXOffset, (ushort)'1');

            _mmapView.Write(0, (uint)0);
        }

        public void Dispose()
        {
            _mmapView.Dispose();
            _mmap.Dispose();
            _syncMapView.Dispose();
            _syncMap.Dispose();
            _writeLock.Dispose();
        }

        public void SetGameInfo(ICollection<GameDetails> gameInfoCollection)
        {
            try
            {
                _writeLock.WaitOne();

                var mmapSize =
                    gameInfoCollection.Sum(x =>
                        sizeof(ushort) + x.Regex.CStrLen() + sizeof(ushort) + x.ShortName.CStrLen()) + sizeof(ushort);

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

                var newMmap = MemoryMappedFile.CreateNew(newMmapName, mmapSize, MemoryMappedFileAccess.ReadWrite);
                var newView = newMmap.CreateViewAccessor();

                newView.Write(0, (uint)mmapSize);
                var offset = sizeof(uint);
                foreach (var gi in gameInfoCollection)
                {
                    WriteCStr(newView, ref offset, gi.Regex);
                    WriteCStr(newView, ref offset, gi.ShortName);
                }
                newView.Write(offset, (ushort)0);

                _mmapView.Dispose();
                _mmap.Dispose();

                _mmapView = newView;
                _mmap = newMmap;
            }
            finally
            {
                _writeLock.ReleaseMutex();
            }
        }

        private static void WriteCStr(UnmanagedMemoryAccessor view, ref int offset, string str)
        {
            view.Write(offset, (ushort)str.Length);
            offset += sizeof(ushort);

            var cstr = str.ToCStr();
            view.WriteArray(offset, cstr, 0, cstr.Length);
            offset += cstr.Length;
        }
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
    }
}