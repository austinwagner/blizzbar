using System;
using System.Collections.Generic;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Blizzbar.Interop
{
    internal class UpdateHandler : IDisposable
    {
        private const int MmapNameStartOffset = sizeof(ushort);
        private static readonly int FullMmapNameXOffset = MmapNameStartOffset + Strings.MmapNameXOffset;

        private readonly Mutex _writeLock;
        private readonly MemoryMappedFile _syncMap;
        private readonly MemoryMappedViewAccessor _syncMapView;
        private MemoryMappedFile _mmap;
        private MemoryMappedViewAccessor _mmapView;

        public UpdateHandler()
        {
            _writeLock = new Mutex(false, Strings.MmapWriteMutexName);

            _syncMap = MemoryMappedFile.CreateNew(Strings.MmapSyncName, MmapNameStartOffset + Strings.MmapNameX.Length, MemoryMappedFileAccess.ReadWrite);
            _syncMapView = _syncMap.CreateViewAccessor();

            _mmap = MemoryMappedFile.CreateNew(Strings.MmapName1, sizeof(uint), MemoryMappedFileAccess.ReadWrite);
            _mmapView = _mmap.CreateViewAccessor();

            _syncMapView.Write(0, (ushort)0);
            _syncMapView.WriteArray(MmapNameStartOffset, Strings.MmapNameX, 0, Strings.MmapNameX.Length);
            _syncMapView.Write(FullMmapNameXOffset, (ushort)'1');

            _mmapView.Write(0, (uint)0);
        }

        public void SetGameInfo(ICollection<GameDetails> gameInfoCollection)
        {
            try
            {
                _writeLock.WaitOne();

                var mmapGameInfoSize = (32 + 32 + 64 + 64) * gameInfoCollection.Count;
                var mmapSize = sizeof(uint) + mmapGameInfoSize;
                
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
                
                newView.Write(0, (uint)gameInfoCollection.Count);
                var offset = sizeof(uint);
                foreach (var gi in gameInfoCollection)
                {
                    WriteCStr(newView, ref offset, gi.Exe32, 32);
                    WriteCStr(newView, ref offset, gi.Exe64, 32);
                    WriteCStr(newView, ref offset, "BlizzardEnterainment.BattleNet." + gi.ShortName, 64);
                    WriteCStr(newView, ref offset, "battlenet://" + gi.ShortName, 64);
                }

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

        public void Dispose()
        {
            _mmapView.Dispose();
            _mmap.Dispose();
            _syncMapView.Dispose();
            _syncMap.Dispose();
            _writeLock.Dispose();
        }

        private static void WriteCStr(UnmanagedMemoryAccessor view, ref int offset, string str, int maxCLen)
        {
            var cstr = ToCStr(str, maxCLen);
            view.WriteArray(offset, cstr, 0, cstr.Length);
            offset += maxCLen;
        }

        private static byte[] ToCStr(string str, int maxCLen)
        {
            if (str.Length >= maxCLen)
            {
                throw new Exception($"'{str}' exceeds maximum length of {(maxCLen - 1):D}.");
            }

            var result = new byte[str.Length + 2];
            Encoding.Unicode.GetBytes(str, 0, str.Length, result, 0);
            result[str.Length] = 0;
            result[str.Length + 1] = 0;
            return result;
        }
    }
}
