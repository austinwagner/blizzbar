using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading.Tasks;
using Blizzbar.Agent.Responses;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace Blizzbar.Agent
{
    internal sealed class Client
    {
        private static readonly string[] SkipGames = {"agent", "battle.net"};
        private readonly HttpClient _client = new HttpClient();
        private readonly JsonSerializer _serializer = new JsonSerializer();

        private bool _authenticated;

        public Client() => _client.BaseAddress = new Uri("http://127.0.0.1:1120");

        private async Task Authenticate()
        {
            var resp = await _client.GetAsync("agent");

            using (var streamReader = new StreamReader(await resp.Content.ReadAsStreamAsync()))
            using (var jsonReader = new JsonTextReader(streamReader))
            {
                var respObj = _serializer.Deserialize<JObject>(jsonReader);
                if (!respObj.TryGetValue("authorization", out var authorization))
                {
                    return;
                }

                _client.DefaultRequestHeaders.Authorization =
                    new AuthenticationHeaderValue(authorization.Value<string>());
            }

            _authenticated = true;
        }

        private async Task<T> TryJsonRequest<T>(string uri)
        {
            try
            {
                if (!_authenticated)
                {
                    await Authenticate();

                    if (!_authenticated)
                    {
                        throw new AgentRequestException("Could not authenticate with agent.");
                    }
                }

                var resp = await _client.GetAsync(uri);
                resp.EnsureSuccessStatusCode();
                using (var streamReader = new StreamReader(await resp.Content.ReadAsStreamAsync()))
                using (var jsonReader = new JsonTextReader(streamReader))
                {
                    return _serializer.Deserialize<T>(jsonReader);
                }
            }
            catch (HttpRequestException ex)
            {
                _authenticated = false;
                throw new AgentRequestException($"Request for {uri} failed.", ex);
            }
            catch (JsonException ex)
            {
                throw new AgentRequestException($"Deserialization of response from {uri} failed.", ex);
            }
        }

        public async Task<List<Game>> GetGames()
        {
            var resp = await TryJsonRequest<Dictionary<string, GameLinkResponse>>("/game");
            return resp.Where(x => !SkipGames.Contains(x.Key)).Select(x => new Game(x.Key, x.Value.Link)).ToList();
        }

        public async Task<List<GameDetails>> GetGameDetails(Game game)
        {
            var resp = await TryJsonRequest<GameDetailsResponse>(game.Uri);
            var result = new List<GameDetails>();
            if (!string.IsNullOrEmpty(resp.Binaries.Game.Regex))
            {
                result.Add(GameDetails.FromRegex(resp.InstallDir, resp.Binaries.Game.Regex, game.LaunchName));
            }

            if (!string.IsNullOrEmpty(resp.Binaries.Game.RelativePath))
            {
                result.Add(
                    GameDetails.FromExecutable(resp.InstallDir, resp.Binaries.Game.RelativePath, game.LaunchName));
            }

            if (!string.IsNullOrEmpty(resp.Binaries.Game.RelativePath64))
            {
                result.Add(GameDetails.FromExecutable(resp.InstallDir, resp.Binaries.Game.RelativePath64,
                    game.LaunchName));
            }

            return result;
        }

        public async Task<List<GameDetails>> GetAllGameDetails()
        {
            var games = await GetGames();
            var gameDetailsTask = games.Select(GetGameDetails);
            var gameDetails = await Task.WhenAll(gameDetailsTask);
            return gameDetails.SelectMany(x => x).ToList();
        }
    }
}