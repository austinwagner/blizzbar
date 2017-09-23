namespace Blizzbar
{
    partial class NotifyIconForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(NotifyIconForm));
            this._nicon = new System.Windows.Forms.NotifyIcon(this.components);
            this._menu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this._refresh = new System.Windows.Forms.ToolStripMenuItem();
            this._exit = new System.Windows.Forms.ToolStripMenuItem();
            this._agentPollTimer = new System.Windows.Forms.Timer(this.components);
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._runOnStartup = new System.Windows.Forms.ToolStripMenuItem();
            this._menu.SuspendLayout();
            this.SuspendLayout();
            // 
            // _nicon
            // 
            this._nicon.ContextMenuStrip = this._menu;
            this._nicon.Icon = ((System.Drawing.Icon)(resources.GetObject("_nicon.Icon")));
            this._nicon.Text = "Blizzbar";
            this._nicon.Visible = true;
            this._nicon.Click += new System.EventHandler(this.NotifyIconClickHandler);
            // 
            // _menu
            // 
            this._menu.ImageScalingSize = new System.Drawing.Size(36, 36);
            this._menu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._runOnStartup,
            this.toolStripSeparator1,
            this._refresh,
            this._exit});
            this._menu.Name = "menu";
            this._menu.ShowCheckMargin = true;
            this._menu.ShowImageMargin = false;
            this._menu.Size = new System.Drawing.Size(271, 186);
            this._menu.Opening += new System.ComponentModel.CancelEventHandler(this.MenuOpeningHandler);
            // 
            // _refresh
            // 
            this._refresh.Name = "_refresh";
            this._refresh.Size = new System.Drawing.Size(270, 42);
            this._refresh.Text = "&Refresh";
            this._refresh.Click += new System.EventHandler(this.RefreshClickHandler);
            // 
            // _exit
            // 
            this._exit.Name = "_exit";
            this._exit.Size = new System.Drawing.Size(270, 42);
            this._exit.Text = "E&xit";
            this._exit.Click += new System.EventHandler(this.ExitClickHandler);
            // 
            // _agentPollTimer
            // 
            this._agentPollTimer.Interval = 3600000;
            this._agentPollTimer.Tick += new System.EventHandler(this.AgentPollTimerTickHandler);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(267, 6);
            // 
            // _runOnStartup
            // 
            this._runOnStartup.Name = "_runOnStartup";
            this._runOnStartup.Size = new System.Drawing.Size(270, 42);
            this._runOnStartup.Text = "Run on Startup";
            this._runOnStartup.Click += new System.EventHandler(this.RunOnStartupClickHandler);
            // 
            // NotifyIconForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(14F, 29F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(0, 0);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.Margin = new System.Windows.Forms.Padding(7);
            this.Name = "NotifyIconForm";
            this.ShowInTaskbar = false;
            this.Text = "NotifyIconForm";
            this.WindowState = System.Windows.Forms.FormWindowState.Minimized;
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.FormClosedHandler);
            this.Load += new System.EventHandler(this.FormLoadHandler);
            this._menu.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.NotifyIcon _nicon;
        private System.Windows.Forms.ContextMenuStrip _menu;
        private System.Windows.Forms.ToolStripMenuItem _exit;
        private System.Windows.Forms.Timer _agentPollTimer;
        private System.Windows.Forms.ToolStripMenuItem _refresh;
        private System.Windows.Forms.ToolStripMenuItem _runOnStartup;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
    }
}