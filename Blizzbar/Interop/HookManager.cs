using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Windows.Forms;

namespace Blizzbar.Interop
{
    internal class HookManager : IDisposable
    {
        private readonly HookInstaller _hook;
        private readonly SurrogateHookInstaller _surrogateHook;

        public HookManager(Action surrogateFailureHandler)
        {
            var hookLibName = $"BlizzbarHooks{(Environment.Is64BitProcess ? "64" : "32")}.dll";
            _hook = new HookInstaller(HookType.Shell, hookLibName, "ShellHookHandler");

            try
            {
                _surrogateHook = new SurrogateHookInstaller(surrogateFailureHandler);
            }
            catch
            {
                _hook.Dispose();
                throw;
            }
        }

        public void Dispose()
        {
            _hook.Dispose();
            _surrogateHook.Dispose();
        }

        private class SurrogateHookInstaller : IDisposable
        {
            private readonly Mutex _barrier;
            private readonly Thread _monitor;
            private readonly Process _process;

            public SurrogateHookInstaller(Action unexpectedExitHandler)
            {
                if (Environment.Is64BitProcess || !Environment.Is64BitOperatingSystem)
                {
                    return;
                }

                try
                {
                    _barrier = new Mutex(true, "Local\\" + Strings.AppGuid + ".surrogate_barrier");
                    var exePath = Path.Combine(Application.StartupPath, "BlizzbarSurrogate.exe");
                    _process = Process.Start(exePath);

                    // Documentation says the function won't return null when starting an exe
                    Debug.Assert(_process != null);
                }
                catch (Exception ex)
                {
                    _barrier?.Dispose();
                    _process?.Dispose();
                    throw new FatalException("Failed to start Blizzbar surrogate process.", ex);
                }

                _monitor = new Thread(() =>
                {
                    _process.WaitForExit();
                    unexpectedExitHandler();
                });

                _monitor.Start();
            }

            public void Dispose()
            {
                _monitor.Interrupt();
                _barrier?.Dispose();
                _process?.Dispose();
            }
        }
    }
}