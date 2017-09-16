using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using Blizzbar.Interop;

namespace Blizzbar
{
    public partial class NotifyIconForm : Form
    {
        // HACK: Grab this private method so the context menu can properly be shown on left click.
        private static readonly MethodInfo ShowContextMenu = typeof(NotifyIcon).GetMethod("ShowContextMenu", BindingFlags.Instance | BindingFlags.NonPublic);

        private readonly Agent.Client _agentClient = new Agent.Client();
        private readonly UpdateHandler _updateHandler = new UpdateHandler();

        private Mutex _surrogateMutex;
        private bool _exiting;
        private Process _surrogate;
        private HookInstaller _hook;

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
                _surrogateMutex = new Mutex(true, "Local\\" + Strings.AppGuid + ".surrogate_mutex");
                var exePath = Path.Combine(Application.StartupPath, "BlizzbarSurrogate.exe");
                _surrogate = Process.Start(exePath);
                Debug.Assert(_surrogate != null); // Documentation says the function won't return null when starting an exe
            }
            catch (Exception ex)
            {
                throw new FatalException("Failed to start Blizzbar surrogate process.", ex);
            }

            _surrogate.Exited += (sender, args) =>
            {
                if (_exiting) return;

                MessageBox.Show("Blizzbar surrogate process exited unexpectedly. Shutting down main process.",
                    "Blizzbar", MessageBoxButtons.OK, MessageBoxIcon.Error);

                Application.Exit();
            };
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
            _surrogateMutex?.Dispose();
            _surrogate?.Dispose();
            _hook?.Dispose();
        }
    }
}
