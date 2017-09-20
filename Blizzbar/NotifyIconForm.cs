using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using Blizzbar.Agent;
using Blizzbar.Interop;

namespace Blizzbar
{
    public partial class NotifyIconForm : Form
    {
        // HACK: Grab this private method so the context menu can properly be shown on left click.
        private static readonly MethodInfo ShowContextMenu =
            typeof(NotifyIcon).GetMethod("ShowContextMenu", BindingFlags.Instance | BindingFlags.NonPublic);

        private readonly Client _agentClient = new Client();
        private readonly UpdateHandler _updateHandler = new UpdateHandler();
        private bool _exiting;
        private HookInstaller _hook;
        private Process _surrogate;

        private Mutex _surrogateBarrier;
        private Thread _surrogateMonitor;

        public NotifyIconForm()
        {
            InitializeComponent();
        }

        private void exit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void nicon_Click(object sender, EventArgs e)
        {
            ShowContextMenu.Invoke(nicon, null);
        }

        private async void NotifyIconForm_Load(object sender, EventArgs e)
        {
            try
            {
                try
                {
                    await PollAgent();
                }
                catch (Exception ex)
                {
                    throw new FatalException("Failed to load initial configuration from Battle.net Agent.", ex);
                }

                InstallHook();
                StartSurrogate();
            }
            catch (FatalException ex)
            {
                ex.Exit();
            }
        }

        private void InstallHook()
        {
            var hookLibName = $"BlizzbarHooks{(Environment.Is64BitProcess ? "64" : "32")}.dll";
            _hook = new HookInstaller(hookLibName, "ShellHookHandler");
        }

        private void StartSurrogate()
        {
            if (Environment.Is64BitProcess || !Environment.Is64BitOperatingSystem)
            {
                return;
            }

            try
            {
                _surrogateBarrier = new Mutex(true, "Local\\" + Strings.AppGuid + ".surrogate_barrier");
                var exePath = Path.Combine(Application.StartupPath, "BlizzbarSurrogate.exe");
                _surrogate = Process.Start(exePath);
                Debug.Assert(_surrogate !=
                             null); // Documentation says the function won't return null when starting an exe
            }
            catch (Exception ex)
            {
                throw new FatalException("Failed to start Blizzbar surrogate process.", ex);
            }

            _surrogateMonitor = new Thread(() =>
            {
                _surrogate.WaitForExit();

                if (_exiting) return;

                Invoke((Action)delegate
                {
                    MessageBox.Show("Blizzbar surrogate process exited unexpectedly. Shutting down main process.",
                        "Blizzbar", MessageBoxButtons.OK, MessageBoxIcon.Error);

                    Application.Exit();
                });
            });

            _surrogateMonitor.Start();
        }

        private async void agentPollTimer_Tick(object sender, EventArgs e)
        {
            try
            {
                await PollAgent();
            }
            catch (Exception ex)
            {
                Debug.Print(ex.ToString());
            }
        }

        private async Task PollAgent()
        {
            Debug.Print("Agent poll started");
            var games = await _agentClient.GetAllGameDetails();
            Debug.Print("Agent poll completed with {0:D} games:", games.Count);
            if (Debug.Listeners.Count > 0)
            {
                Debug.Indent();
                foreach (var g in games)
                {
                    Debug.Print(g.ToString());
                }
                Debug.Unindent();
            }
            _updateHandler.SetGameInfo(games);
        }

        private async void refresh_Click(object sender, EventArgs e)
        {
            try
            {
                await PollAgent();
                MessageBox.Show("Blizzbar configuration refreshed.", "Blizzbar", MessageBoxButtons.OK);
            }
            catch (Exception ex)
            {
                Debug.Print(ex.ToString());
                if (MessageBox.Show($"Blizzbar configuration refresh failed!\n{ex.Message}", "Blizzbar",
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Error) == DialogResult.Retry)
                {
                    refresh.PerformClick();
                }
            }
        }

        private void NotifyIconForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            Debug.Print("Disposing main form");
            _exiting = true;
            _updateHandler.Dispose();
            _surrogateBarrier?.ReleaseMutex();
            _surrogateBarrier?.Dispose();
            _surrogate?.Dispose();
            _hook?.Dispose();
        }
    }
}