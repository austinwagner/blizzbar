﻿namespace Blizzbar
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
            this.nicon = new System.Windows.Forms.NotifyIcon(this.components);
            this.menu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.exit = new System.Windows.Forms.ToolStripMenuItem();
            this.refresh = new System.Windows.Forms.ToolStripMenuItem();
            this.agentPollTimer = new System.Windows.Forms.Timer(this.components);
            this.menu.SuspendLayout();
            this.SuspendLayout();
            // 
            // nicon
            // 
            this.nicon.ContextMenuStrip = this.menu;
            this.nicon.Icon = ((System.Drawing.Icon)(resources.GetObject("nicon.Icon")));
            this.nicon.Text = "Blizzbar";
            this.nicon.Visible = true;
            this.nicon.Click += new System.EventHandler(this.nicon_Click);
            // 
            // menu
            // 
            this.menu.ImageScalingSize = new System.Drawing.Size(36, 36);
            this.menu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.exit,
            this.refresh});
            this.menu.Name = "menu";
            this.menu.Size = new System.Drawing.Size(181, 88);
            // 
            // exit
            // 
            this.exit.Name = "exit";
            this.exit.Size = new System.Drawing.Size(180, 42);
            this.exit.Text = "E&xit";
            this.exit.Click += new System.EventHandler(this.exit_Click);
            // 
            // refresh
            // 
            this.refresh.Name = "refresh";
            this.refresh.Size = new System.Drawing.Size(180, 42);
            this.refresh.Text = "&Refresh";
            this.refresh.Click += new System.EventHandler(this.refresh_Click);
            // 
            // agentPollTimer
            // 
            this.agentPollTimer.Interval = 3600000;
            this.agentPollTimer.Tick += new System.EventHandler(this.agentPollTimer_Tick);
            // 
            // NotifyIconForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(14F, 29F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(663, 582);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Margin = new System.Windows.Forms.Padding(7, 7, 7, 7);
            this.Name = "NotifyIconForm";
            this.ShowInTaskbar = false;
            this.Text = "NotifyIconForm";
            this.WindowState = System.Windows.Forms.FormWindowState.Minimized;
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.NotifyIconForm_FormClosed);
            this.Load += new System.EventHandler(this.NotifyIconForm_Load);
            this.menu.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.NotifyIcon nicon;
        private System.Windows.Forms.ContextMenuStrip menu;
        private System.Windows.Forms.ToolStripMenuItem exit;
        private System.Windows.Forms.Timer agentPollTimer;
        private System.Windows.Forms.ToolStripMenuItem refresh;
    }
}