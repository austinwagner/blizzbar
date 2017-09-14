using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Blizzbar
{
    public static class Program
    {
        private static readonly Mutex SingleInstanceMutex = new Mutex(true, "{B7B2F090-43B9-4792-883F-7828743EDDF2}");

        [STAThread]
        public static void Main()
        {
            if (SingleInstanceMutex.WaitOne(TimeSpan.Zero, true))
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new NotifyIconForm());
                SingleInstanceMutex.ReleaseMutex();
            }
        }
    }
}
