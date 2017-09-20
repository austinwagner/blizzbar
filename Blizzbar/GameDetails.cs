using System.Text;

namespace Blizzbar
{
    internal sealed class GameDetails
    {
        public readonly string Regex;
        public readonly string ShortName;

        private GameDetails(string regex, string shortName)
        {
            Regex = regex;
            ShortName = shortName;
        }

        public static GameDetails FromRegex(string installDir, string regex, string shortName) => new GameDetails(
            EscapeStringForRegex(installDir.Replace("/", "\\")) + "\\\\" + regex.Replace("/", "\\\\"),
            shortName);

        public static GameDetails FromExecutable(string installDir, string exe, string shortName) =>
            new GameDetails(EscapeStringForRegex(installDir.Replace("/", "\\") + "\\" + exe.Replace("/", "\\")),
                shortName);

        public override string ToString() => $"{{Regex={Regex}, ShortName={ShortName}}}";

        private static string EscapeStringForRegex(string str)
        {
            var sb = new StringBuilder();
            foreach (var c in str)
            {
                switch (c)
                {
                case '(':
                    goto case '?';
                case ')':
                    goto case '?';
                case '[':
                    goto case '?';
                case '{':
                    goto case '?';
                case '*':
                    goto case '?';
                case '+':
                    goto case '?';
                case '.':
                    goto case '?';
                case '$':
                    goto case '?';
                case '^':
                    goto case '?';
                case '\\':
                    goto case '?';
                case '|':
                    goto case '?';
                case '?':
                    sb.Append('\\');
                    break;
                }

                sb.Append(c);
            }

            return sb.ToString();
        }
    }
}