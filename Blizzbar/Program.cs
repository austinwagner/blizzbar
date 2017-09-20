using System;
using System.Threading;
using System.Windows.Forms;

namespace Blizzbar
{
    public static class Program
    {
        [STAThread]
        public static void Main()
        {
            // Single instance guard
            using (var _ = new Mutex(true, "{B7B2F090-43B9-4792-883F-7828743EDDF2}", out var createdMutex))
            {
                if (!createdMutex) return;

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new NotifyIconForm());
            }
        }
    }
}