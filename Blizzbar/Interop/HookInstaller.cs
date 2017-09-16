using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32.SafeHandles;

namespace Blizzbar.Interop
{
    internal class HookInstaller : IDisposable
    {
        private readonly Win32.SafeLibraryHandle _lib;
        private readonly Win32.SafeHookHandle _hook;

        public HookInstaller(string libName, string funcName)
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
                throw new FatalException($"Unable to find function '{funcName}' in library '{libName}'.", new Win32Exception(err));
            }

            _hook = Win32.SetWindowsHookEx(HookType.Shell, procAddr, _lib.DangerousGetHandle(), 0);
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
