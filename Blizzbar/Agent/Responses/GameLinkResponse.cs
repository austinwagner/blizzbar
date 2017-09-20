using Newtonsoft.Json;

namespace Blizzbar.Agent.Responses
{
    public sealed class GameLinkResponse
    {
        [JsonProperty("link", Required = Required.Always)]
        public string Link { get; set; }
    }
}