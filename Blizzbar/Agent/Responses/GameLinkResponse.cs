using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace Blizzbar.Agent.Responses
{
    public sealed class GameLinkResponse
    {
        [JsonProperty("link", Required = Required.Always)]
        public string Link { get; set; }
    }
}
