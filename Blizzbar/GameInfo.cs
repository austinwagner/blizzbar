namespace Blizzbar
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;

    using CsvHelper;
    using System.ComponentModel;
    using System.Reflection;

    internal class GameInfo
    {
        private readonly Action<GameInfo> onSelected;

        public string UrlName { get; private set; }

        public string FullName { get; private set; }

        public string AgentName { get; private set; }

        public string LauncherExe { get; private set; }

        private bool isSelected;

        public bool IsSelected
        {
            get { return this.isSelected; }
            set
            {
                this.isSelected = value;
                if (value)
                {
                    this.onSelected(this);
                }
            }
        }

        public string AppUserModelId => "84fed2b1-9493-4e53-a569-9e5e5abe7da2." + this.UrlName;

        internal GameInfo(Action<GameInfo> onSelected, string shortName, string longName, string agentName, string launcherExe)
        {
            this.onSelected = onSelected;
            this.UrlName = shortName;
            this.FullName = longName;
            this.AgentName = agentName;
            this.LauncherExe = launcherExe;
            this.isSelected = false;
        }
    }

    internal class AllGameInfo : INotifyPropertyChanged
    {
        private List<GameInfo> gameInfo;

        public IReadOnlyCollection<GameInfo> GameInfoCollection => this.gameInfo;

        private GameInfo selected;

        public GameInfo Selected
        {
            get { return this.selected; }
            private set
            {
                this.selected = value;
                this.SelectionChanged?.Invoke(value);
            }
        }

        private void OnSelected(GameInfo info) { this.Selected = info; }

        public event Action<GameInfo> SelectionChanged;
        public event PropertyChangedEventHandler PropertyChanged;

        // The order of these fields is important!
        private class GameInfoCsv
        {
            public string FullName { get; set; }
            public string UrlName { get; set; }
            public string AgentName { get; set; }
            public string LauncherExe { get; set; }
            public string Exe32Bit { get; set; }
            public string Exe64Bit { get; set; }
        }

        public void LoadGameInfo()
        {
            var path = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "games.txt");

            using (var fs = new FileStream(path, FileMode.Open, FileAccess.Read))
            using (var sr = new StreamReader(fs))
            using (var cr = new CsvReader(sr))
            {
                cr.Configuration.Delimiter = "|";
                cr.Configuration.HasHeaderRecord = false;
                gameInfo = cr.GetRecords<GameInfoCsv>()
                    .Select(x => new GameInfo(this.OnSelected, x.UrlName, x.FullName, x.AgentName, x.LauncherExe))
                    .ToList();
            }

            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(GameInfoCollection)));
        }
    }
}
