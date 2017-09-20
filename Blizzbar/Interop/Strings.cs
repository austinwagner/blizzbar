using System.Text;

namespace Blizzbar.Interop
{
    internal static class Strings
    {
        public const string AppGuid = "84fed2b1-9493-4e53-a569-9e5e5abe7da2";

        private const string MmapBaseName = "Local\\" + AppGuid;
        private const string MmapNameXStr = MmapBaseName + ".x\0";

        public const string MmapName1 = MmapBaseName + ".1";
        public const string MmapName2 = MmapBaseName + ".2";

        public const string MmapSyncName = MmapBaseName + ".sync";

        public const string MmapWriteMutexName = AppGuid + ".write_mutex";

        public static readonly byte[] MmapNameX = Encoding.Unicode.GetBytes(MmapNameXStr);
        public static readonly int MmapNameXOffset = MmapNameXStr.Length - 2;
    }
}