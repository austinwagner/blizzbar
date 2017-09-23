using System;
using System.Text;
using System.Windows.Forms;
using NLog;

namespace Blizzbar
{
    internal class FatalException : Exception
    {
        public static readonly ILogger Log = LogManager.GetCurrentClassLogger();

        public FatalException(string message)
            : base(message) { }

        public FatalException(string message, Exception innerException)
            : base(message, innerException) { }

        public void Exit()
        {
            Log.Fatal(this);

            var sb = new StringBuilder();
            sb.AppendLine(Message);
            for (var ie = InnerException; ie != null; ie = ie.InnerException)
            {
                sb.Append("   ").AppendLine(ie.Message);
            }
            sb.AppendLine("Blizzbar will now terminate.");

            // Dummy topmost parent form prevents this message from ending up behind other windows
            MessageBox.Show(new Form {TopMost = true}, sb.ToString(), "Blizzbar", MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            Application.Exit();
        }
    }
}