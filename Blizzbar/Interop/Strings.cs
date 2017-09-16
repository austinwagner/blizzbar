using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Blizzbar.Interop
{
    internal static class Strings
    {
        public const string AppGuid = "84fed2b1-9493-4e53-a569-9e5e5abe7da2";
#if X86
        public const string BitModifier = "32";
#else
        public const string BitModifier = "64";
#endif
        private const string MmapBaseName = "Local\\" + AppGuid + "." + BitModifier;
        private const string MmapNameXStr = MmapBaseName + ".x\0";

        public static readonly byte[] MmapNameX = Encoding.Unicode.GetBytes(MmapNameXStr);
        public static readonly int MmapNameXOffset = MmapNameXStr.Length - 1;

        public const string MmapName1 = MmapBaseName + ".1";
        public const string MmapName2 = MmapBaseName + ".2";

        public const string MmapSyncName = MmapBaseName + ".sync";

        private const string MmapMutexBaseName = AppGuid + "." + BitModifier;
        public const string MmapWriteMutexName = MmapMutexBaseName + ".write_mutex";
    }
}
