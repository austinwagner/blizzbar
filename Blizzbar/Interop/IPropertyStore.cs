using System;
using System.Runtime.InteropServices;

namespace Blizzbar.Interop
{
    [StructLayout(LayoutKind.Sequential)]
    public struct PropertyKey
    {
        public Guid fmtid;
        public uint pid;

        public PropertyKey(uint a, ushort b, ushort c, byte d, byte e, byte f, byte g, byte h, byte i, byte j, byte k,
            uint propertyId)
        {
            fmtid = new Guid(a, b, c, d, e, f, g, h, i, j, k);
            pid = propertyId;
        }

        public static class AppUserModel
        {
            public static PropertyKey Id = new PropertyKey(0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D,
                0xE1, 0xD5, 0xF3, 5);

            public static PropertyKey PreventPinning = new PropertyKey(0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1,
                0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 9);

            public static PropertyKey RelaunchCommand = new PropertyKey(0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1,
                0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 2);

            public static PropertyKey RelaunchDisplayNameResource = new PropertyKey(0x9F4C2855, 0x9F79, 0x4B39, 0xA8,
                0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 4);

            public static PropertyKey RelaunchIconResource = new PropertyKey(0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0,
                0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 3);
        }
    }

    public static class IID
    {
        public static Guid IPropertyStore = new Guid("886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99");
    }

    [ComImport]
    [Guid("886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IPropertyStore
    {
        [PreserveSig]
        int GetCount([Out] out uint cProps);

        [PreserveSig]
        int GetAt([In] uint iProp, out PropertyKey pkey);

        [PreserveSig]
        int GetValue([In] ref PropertyKey key, out PropVariant pv);

        [PreserveSig]
        int SetValue([In] ref PropertyKey key, [In] ref PropVariant pv);

        [PreserveSig]
        int Commit();
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PropVariant
    {
        public VarEnum vt;

        // There's 3 wReserved, but vt is 2 bytes while VarEnum is 4
        // By removing one of the wReserved we maintain the layout
        private readonly ushort wReserved1;

        private readonly ushort wReserved2;
        public IntPtr p;
        public int p2;
    }
}