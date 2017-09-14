using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Blizzbar.Agent
{
    internal class AgentRequestException : Exception
    {
        public AgentRequestException(string message)
            : base(message)
        {
        }

        public AgentRequestException(string message, Exception innerException)
            : base(message, innerException)
        {
        }
    }
}