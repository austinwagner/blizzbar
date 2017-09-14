using System.Collections.Generic;

namespace Blizzbar.Agent
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Net.Http;
    using System.Net.Http.Headers;
    using System.Threading.Tasks;

    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    internal sealed class Client
    {
        private readonly HttpClient client = new HttpClient();
        private readonly JsonSerializer serializer = new JsonSerializer();

        private bool authenticated = false;

        public Client()
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
                if (!respObj.TryGetValue("authorization", out var authorization))
                {
                    return;
                }

                this.client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue(authorization.Value<string>());
            }

            this.authenticated = true;
        }

        private async Task<T> TryJsonRequest<T>(string uri)
        {
            try
            {
                if (!this.authenticated)
                {
                    await this.Authenticate();

                    if (!this.authenticated)
                    {
                        throw new AgentRequestException("Could not authenticate with agent.");
                    }
                }

                var resp = await this.client.GetAsync("game");
                resp.EnsureSuccessStatusCode();
                using (var streamReader = new StreamReader(await resp.Content.ReadAsStreamAsync()))
                using (var jsonReader = new JsonTextReader(streamReader))
                {
                    return this.serializer.Deserialize<T>(jsonReader);
                }
            }
            catch (HttpRequestException ex)
            {
                this.authenticated = false;
                throw new AgentRequestException($"Request for {uri} failed.", ex);
            }
            catch (JsonException ex)
            {
                throw new AgentRequestException($"Deserialization of response from {uri} failed.", ex);
            }
        }

        private static readonly string[] SkipGames = {"agent", "battle.net"};
        public async Task<List<Game>> GetGames()
        {
            var resp = await TryJsonRequest<Dictionary<string, Responses.GameLinkResponse>>("/games");
            return resp.Where(x => !SkipGames.Contains(x.Key)).Select(x => new Game(x.Key, x.Value.Link)).ToList();
        }

        public async Task<GameDetails> GetGameDetails(Game game)
        {
            var resp = await TryJsonRequest<Responses.GameDetailsResponse>(game.Uri);
            return new GameDetails(
                resp.Binaries.Game.RelativePath ?? string.Empty,
                resp.Binaries.Game.RelativePath64 ?? string.Empty,
                resp.Product);
        }

        public async Task<List<GameDetails>> GetAllGameDetails()
        {
            var games = await GetGames();
            var gameDetailsTask = games.Select(GetGameDetails);
            var gameDetails = await Task.WhenAll(gameDetailsTask);
            return gameDetails.ToList();
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
