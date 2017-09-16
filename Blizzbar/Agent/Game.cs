using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Blizzbar.Agent
{
    internal sealed class Game
    {
        public readonly string Name;
        public readonly string Uri;

        public Game(string name, string uri)
        {
            Name = name;
            Uri = uri;
        }

        public override string ToString()
        {
            return $"{{Name={Name}, Uri={Uri}}}";
        }
    }
}
