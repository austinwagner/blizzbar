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

        // Launch name is the URI without the leading "/game" and no trailing slash
        // (shouldn't be one, but might as well run the replace for safety)
        public string LaunchName => Uri.Substring(5).Replace("/", "");

        public override string ToString() => $"{{Name={Name}, Uri={Uri}}}";
    }
}