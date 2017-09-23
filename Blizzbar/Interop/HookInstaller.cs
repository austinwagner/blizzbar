using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace Blizzbar.Interop
{
    internal class HookInstaller : IDisposable
    {
        private readonly Win32.SafeHookHandle _hook;
        private readonly Win32.SafeLibraryHandle _lib;

        public HookInstaller(HookType type, string libName, string funcName)
        {
            _lib = Win32.LoadLibrary(libName);
            if (_lib.IsInvalid)
            {
                var err = Marshal.GetLastWin32Error();
                throw new FatalException($"Unable to load library '{libName}'.", new Win32Exception(err));
            }

            var procAddr = _lib.GetFunction(funcName);
            if (procAddr == IntPtr.Zero)
            {
                var err = Marshal.GetLastWin32Error();
                throw new FatalException($"Unable to find function '{funcName}' in library '{libName}'.",
                    new Win32Exception(err));
            }

            _hook = Win32.SetWindowsHookEx(type, procAddr, _lib.DangerousGetHandle(), 0);
            if (_hook.IsInvalid)
            {
                var err = Marshal.GetLastWin32Error();
                _lib.Dispose();
                throw new FatalException($"Unable to set hook '{funcName}'.", new Win32Exception(err));
            }
        }

        public void Dispose()
        {
            _hook.Dispose();
            _lib.Dispose();
        }
    }
}