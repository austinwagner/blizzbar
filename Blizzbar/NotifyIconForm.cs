using System;
using System.ComponentModel;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows.Forms;
using Blizzbar.Agent;
using NLog;

namespace Blizzbar
{
    public partial class NotifyIconForm : Form
    {
        private static readonly ILogger Log = LogManager.GetCurrentClassLogger();

        // HACK: Grab this private method so the context menu can properly be shown on left click.
        private static readonly MethodInfo ShowContextMenu =
            typeof(NotifyIcon).GetMethod("ShowContextMenu", BindingFlags.Instance | BindingFlags.NonPublic);

        private readonly Client _agentClient = new Client();
        private readonly UpdateHandler _updateHandler = new UpdateHandler();
        private bool _exiting;
        private IDisposable _hooks;

        public NotifyIconForm()
        {
            InitializeComponent();
        }

        private void ExitClickHandler(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void NotifyIconClickHandler(object sender, EventArgs e)
        {
            ShowContextMenu.Invoke(_nicon, null);
        }

        private async Task PollAgentWithRetries()
        {
            var interval = TimeSpan.FromSeconds(5);
            const int tries = 5;
            var i = 0;

            while (true)
            {
                try
                {
                    await PollAgent();
                    return;
                }
                catch (AgentRequestException ex)
                {
                    i += 1;

                    if (i >= 5)
                    {
                        Log.Error("Poll attempt {0:D} of {0:D} failed. Aborting.", tries);
                        throw new FatalException(
                            $"Failed to connect to Battle.net Agent after {tries:D} tries.\nMake sure the Battle.net Client is running and logged in.");
                    }

                    Log.Info("Poll attempt {0:D} of {1:D} failed. Retrying in {2:F0} seconds.", i, tries,
                        interval.TotalSeconds);
                    Log.Debug(ex);
                    await Task.Delay(interval);
                }
                catch (Exception ex)
                {
                    throw new FatalException(
                        "A problem occurred while trying to communicate with the Battle.net Agent.", ex);
                }
            }
        }

        private async void FormLoadHandler(object sender, EventArgs e)
        {
            Hide();
            try
            {
                await PollAgentWithRetries();

                _hooks = _updateHandler.InstallHooks(() =>
                {
                    if (_exiting) return;

                    Invoke(() =>
                    {
                        MessageBox.Show("Blizzbar surrogate process exited unexpectedly. Shutting down main process.",
                            "Blizzbar", MessageBoxButtons.OK, MessageBoxIcon.Error);

                        Application.Exit();
                    });
                });
            }
            catch (FatalException ex)
            {
                ex.Exit();
            }
        }


        private async void AgentPollTimerTickHandler(object sender, EventArgs e)
        {
            try
            {
                await PollAgent();
            }
            catch (Exception ex)
            {
                Log.Warn(ex);
            }
        }

        private async Task PollAgent()
        {
            Log.Info("Agent poll started");
            var games = await _agentClient.GetAllGameDetails();
            Log.Info("Agent poll completed with {0:D} games:", games.Count);
            if (Log.IsInfoEnabled)
            {
                foreach (var g in games)
                {
                    Log.Info("  {0}", g);
                }
            }

            await _updateHandler.SetGameDetails(games);
        }

        private async void RefreshClickHandler(object sender, EventArgs e)
        {
            try
            {
                await PollAgent();
                MessageBox.Show("Blizzbar configuration refreshed.", "Blizzbar", MessageBoxButtons.OK);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Manual configuration refresh failed.");
                if (MessageBox.Show($"Blizzbar configuration refresh failed!\n{ex.Message}", "Blizzbar",
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Error) == DialogResult.Retry)
                {
                    Invoke(() => RefreshClickHandler(sender, e));
                }
            }
        }

        private void FormClosedHandler(object sender, FormClosedEventArgs e)
        {
            Log.Debug("Disposing main form");
            _exiting = true;
            _updateHandler.Dispose();
            _hooks?.Dispose();
        }

        private void MenuOpeningHandler(object sender, CancelEventArgs e)
        {
            _runOnStartup.Checked = Autostart.IsRegistered();
        }

        private void RunOnStartupClickHandler(object sender, EventArgs e)
        {
            if (_runOnStartup.Checked)
            {
                _runOnStartup.Checked = false;
                Autostart.Unregister();
            }
            else
            {
                _runOnStartup.Checked = true;
                Autostart.Register();
            }
        }

        private void Invoke(Action action)
        {
            Invoke((Delegate)action);
        }
    }
}