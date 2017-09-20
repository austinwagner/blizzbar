using System;

namespace Blizzbar.Agent
{
    internal class AgentRequestException : Exception
    {
        public AgentRequestException(string message)
            : base(message) { }

        public AgentRequestException(string message, Exception innerException)
            : base(message, innerException) { }
    }
}