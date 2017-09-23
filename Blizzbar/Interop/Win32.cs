using System;
using System.Runtime.InteropServices;
using System.Text;
using JetBrains.Annotations;
using Microsoft.Win32.SafeHandles;

namespace Blizzbar.Interop
{
    internal static class Win32
    {
        public delegate bool EnumWindowsProc(IntPtr windowHandle, IntPtr lParam);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool CloseHandle(IntPtr handle);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern SafeHookHandle SetWindowsHookEx(HookType hookType, IntPtr hookFunc, IntPtr module,
            uint threadId);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool UnhookWindowsHookEx(IntPtr hookHandle);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern SafeLibraryHandle LoadLibrary(string filename);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool FreeLibrary(IntPtr handle);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetProcAddress(IntPtr module, string funcName);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern int GetWindowText(IntPtr windowHandle, StringBuilder windowText, int maxChars);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern IntPtr SendMessageTimeout(
            IntPtr windowHandle,
            uint message,
            IntPtr wParam,
            IntPtr lParam,
            SendMessageTimeoutFlags flags,
            uint timeout,
            IntPtr unused);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool EnumWindows([InstantHandle] EnumWindowsProc callback, IntPtr lParam);

        [DllImport("shell32.dll", SetLastError = true)]
        public static extern int SHGetPropertyStoreForWindow(IntPtr handle, ref Guid riid,
            out IPropertyStore propertyStore);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        public static string GetWindowText(IntPtr windowHandle)
        {
            var titleBuilder = new StringBuilder(127);
            return GetWindowText(windowHandle, titleBuilder, titleBuilder.Capacity + 1) == 0
                ? null
                : titleBuilder.ToString();
        }

        public sealed class SafeHookHandle : SafeHandleZeroOrMinusOneIsInvalid
        {
            private const uint WmNull = 0;
            private static readonly IntPtr HwndBroadcast = new IntPtr(0xFFFF);

            public SafeHookHandle() : base(true) { }

            public SafeHookHandle(bool ownsHandle) : base(ownsHandle) { }

            public SafeHookHandle(IntPtr preexistingHandle, bool ownsHandle)
                : base(ownsHandle)
            {
                SetHandle(preexistingHandle);
            }

            protected override bool ReleaseHandle()
            {
                if (!UnhookWindowsHookEx(handle)) return false;

                // Don't care much if this fails. The hook won't be fully uninstalled if a window misses this event,
                // but I don't give a crap if a process can't keep its own message pump running.
                SendMessageTimeout(HwndBroadcast, WmNull, IntPtr.Zero, IntPtr.Zero,
                    SendMessageTimeoutFlags.AbortIfHung | SendMessageTimeoutFlags.NoTimeoutIfHung, 1000, IntPtr.Zero);
                return true;
            }
        }

        public sealed class SafeLibraryHandle : SafeHandleZeroOrMinusOneIsInvalid
        {
            public SafeLibraryHandle() : base(true) { }

            public SafeLibraryHandle(bool ownsHandle) : base(ownsHandle) { }

            public SafeLibraryHandle(IntPtr preexistingHandle, bool ownsHandle)
                : base(ownsHandle)
            {
                SetHandle(preexistingHandle);
            }

            protected override bool ReleaseHandle() => FreeLibrary(handle);

            public IntPtr GetFunction(string name) => GetProcAddress(handle, name);
        }
    }
}