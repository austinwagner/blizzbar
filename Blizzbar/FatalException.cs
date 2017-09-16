using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Blizzbar
{
    class FatalException : Exception
    {
        public FatalException(string message)
            : base(message)
        {
        }

        public FatalException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        public void Exit()
        {
            Debug.Print(ToString());

            var message = InnerException == null
                ? $"{Message}\nBlizzbar will now terminate."
                : $"{Message}\nError: {InnerException.Message}\nBlizzbar will now terminate.";
            MessageBox.Show(message, "Blizzbar", MessageBoxButtons.OK, MessageBoxIcon.Error);
            Application.Exit();
        }
    }
}
