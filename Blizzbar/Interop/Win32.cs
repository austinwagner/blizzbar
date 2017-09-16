using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32.SafeHandles;

namespace Blizzbar.Interop
{
    internal static class Win32
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool CloseHandle(IntPtr handle);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern SafeHookHandle SetWindowsHookEx(HookType hookType, IntPtr hookFunc, IntPtr module, uint threadId);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool UnhookWindowsHookEx(IntPtr hookHandle);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern SafeLibraryHandle LoadLibrary(string filename);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetProcAddress(IntPtr module, string funcName);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern IntPtr SendMessageTimeout(
            IntPtr windowHandle,
            uint message,
            IntPtr wParam,
            IntPtr lParam,
            SendMessageTimeoutFlags flags,
            uint timeout,
            IntPtr unused);

        public sealed class SafeHookHandle : SafeHandleZeroOrMinusOneIsInvalid
        {
            private static readonly IntPtr HwndBroadcast = new IntPtr(0xFFFF);
            private const uint WmNull = 0;

            public SafeHookHandle(IntPtr preexistingHandle, bool ownsHandle)
                : base(ownsHandle)
            {
                SetHandle(preexistingHandle);
            }
            
            protected override bool ReleaseHandle()
            {
                UnhookWindowsHookEx(handle);
                SendMessageTimeout(HwndBroadcast, WmNull, IntPtr.Zero, IntPtr.Zero,
                    SendMessageTimeoutFlags.AbortIfHung | SendMessageTimeoutFlags.NoTimeoutIfHung, 1000, IntPtr.Zero);
                return true;
            }
        }

        public sealed class SafeLibraryHandle : SafeHandleZeroOrMinusOneIsInvalid
        {
            public SafeLibraryHandle(IntPtr preexistingHandle, bool ownsHandle)
                : base(ownsHandle)
            {
                SetHandle(preexistingHandle);
            }

            protected override bool ReleaseHandle()
            {
                return CloseHandle(handle);
            }

            public IntPtr GetFunction(string name)
            {
                return GetProcAddress(handle, name);
            }
        }
    }
}
