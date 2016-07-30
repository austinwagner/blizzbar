namespace Blizzbar
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Net.Http;
    using System.Net.Http.Headers;
    using System.Threading.Tasks;

    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    class AgentClient
    {
        private readonly HttpClient client = new HttpClient();
        private readonly JsonSerializer serializer = new JsonSerializer();

        private bool authenticated;

        public AgentClient()
        {
            this.client.BaseAddress = new Uri("http://127.0.0.1:1120");
        }

        private async Task Authenticate()
        {
            try
            {
                var resp = await this.client.GetAsync("agent");

                using (var streamReader = new StreamReader(await resp.Content.ReadAsStreamAsync()))
                using (var jsonReader = new JsonTextReader(streamReader))
                {
                    var respObj = this.serializer.Deserialize<JObject>(jsonReader);
                    JToken authorization;
                    if (!respObj.TryGetValue("authorization", out authorization))
                    {
                        return;
                    }

                    this.client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue(authorization.Value<string>());
                }

                this.authenticated = true;
            }
            catch (System.Net.Sockets.SocketException)
            {
                // Discard socket errors. It's not a big deal if we can't talk to the agent.
            }
        }

        public async Task<string> GetInstallPath(GameInfo gameInfo)
        {
            if (!this.authenticated)
            {
                await this.Authenticate();
            }

            if (!this.authenticated)
            {
                return string.Empty;
            }

            string gameKey;
            var resp = await this.client.GetAsync("game");
            using (var streamReader = new StreamReader(await resp.Content.ReadAsStreamAsync()))
            using (var jsonReader = new JsonTextReader(streamReader))
            {
                var respObj = this.serializer.Deserialize<JObject>(jsonReader);
                var gameJObject = respObj.AsJEnumerable()
                    .FirstOrDefault(x => x.Path.StartsWith(gameInfo.AgentName, StringComparison.OrdinalIgnoreCase));
                if (gameJObject == null)
                {
                    return string.Empty;
                }

                gameKey = gameJObject.Path;
            }

            resp = await this.client.GetAsync("game/" + gameKey);
            using (var streamReader = new StreamReader(await resp.Content.ReadAsStreamAsync()))
            using (var jsonReader = new JsonTextReader(streamReader))
            {
                var respObj = this.serializer.Deserialize<JObject>(jsonReader);
                JToken installDir;
                if (!respObj.TryGetValue("install_dir", out installDir))
                {
                    return string.Empty;
                }

                return Path.Combine(Path.GetFullPath(installDir.Value<string>()), gameInfo.LauncherExe + ".exe");
            }
        }
    }
}
