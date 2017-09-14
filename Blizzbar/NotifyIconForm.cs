using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Blizzbar
{
    public partial class NotifyIconForm : Form
    {
        // HACK: Grab this private method so the context menu can properly be shown on left click.
        private static readonly MethodInfo ShowContextMenu = typeof(NotifyIcon).GetMethod("ShowContextMenu", BindingFlags.Instance | BindingFlags.NonPublic);

        private readonly Agent.Client _agentClient = new Agent.Client();
        private readonly NativeHelper _nativeHelper = new NativeHelper();

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
                await PollAgent();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to load initial configuration from Battle.net Agent.\nError: {ex.Message}\nBlizzbar will now terminate.", "Blizzbar",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                Application.Exit();
            }
        }

        private async void agentPollTimer_Tick(object sender, EventArgs e)
        {
            try
            {
                await PollAgent();
            }
            catch (Exception)
            {
                // If our poll fails, just ignore it.
            }
        }

        private async Task PollAgent()
        {
            var games = await _agentClient.GetAllGameDetails();
            _nativeHelper.SetGameDetails(games);
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
                if (MessageBox.Show($"Blizzbar configuration refresh failed!\n{ex.Message}", "Blizzbar",
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Error) == DialogResult.Retry)
                {
                    refresh.PerformClick();
                }
            }
        }
    }
}
