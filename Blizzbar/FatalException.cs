using System;
using System.Diagnostics;
using System.Text;
using System.Windows.Forms;

namespace Blizzbar
{
    internal class FatalException : Exception
    {
        public FatalException(string message)
            : base(message) { }

        public FatalException(string message, Exception innerException)
            : base(message, innerException) { }

        public void Exit()
        {
            Debug.Print(ToString());

            var sb = new StringBuilder();
            sb.AppendLine(Message);
            for (var ie = InnerException; ie != null; ie = ie.InnerException)
            {
                sb.Append("   ").AppendLine(ie.Message);
            }
            sb.AppendLine("Blizzbar will now terminate.");
            MessageBox.Show(sb.ToString(), "Blizzbar", MessageBoxButtons.OK, MessageBoxIcon.Error);
            Application.Exit();
        }
    }
}