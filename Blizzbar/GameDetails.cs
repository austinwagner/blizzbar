using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Blizzbar
{
    internal sealed class GameDetails
    {
        public readonly string Exe32;
        public readonly string Exe64;
        public readonly string ShortName;

        public GameDetails(string exe32, string exe64, string shortName)
        {
            Exe32 = exe32;
            Exe64 = exe64;
            ShortName = shortName;
        }

        public override string ToString()
        {
            return $"{{Exe32={Exe32}, Exe64={Exe64}, ShortName={ShortName}}}";
        }
    }
}
