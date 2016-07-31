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

        private bool authenticated = false;

        public AgentClient()
        {
            this.client.BaseAddress = new Uri("http://127.0.0.1:1120");
        }

        private async Task Authenticate()
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

        public async Task<string> GetInstallPath(GameInfo gameInfo)
        {
            try
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
            catch (HttpRequestException)
            {
                this.authenticated = false;
                return string.Empty;
            }
        }
    }
}
