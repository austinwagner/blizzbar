using Newtonsoft.Json;

namespace Blizzbar.Agent.Responses
{
    public sealed class GameDetailsResponse
    {
        [JsonProperty("binaries", Required = Required.Always)]
        public BinariesResponse Binaries { get; set; }

        [JsonProperty("install_dir", Required = Required.Always)]
        public string InstallDir { get; set; }

        public sealed class BinariesResponse
        {
            [JsonProperty("game", Required = Required.Always)]
            public GameResponse Game { get; set; }

            public sealed class GameResponse
            {
                [JsonProperty("regex")]
                public string Regex { get; set; }

                [JsonProperty("relative_path")]
                public string RelativePath { get; set; }

                [JsonProperty("relative_path_64")]
                public string RelativePath64 { get; set; }
            }
        }
    }
}