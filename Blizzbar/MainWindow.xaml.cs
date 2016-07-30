namespace Blizzbar
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Windows;

    using Microsoft.Win32;

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private AllGameInfo allGameInfo = new AllGameInfo();

        private AgentClient agentClient = new AgentClient();

        public MainWindow()
        {
            this.InitializeComponent();
            this.Grid.DataContext = this.allGameInfo;
            this.LinkFolderPath.Text = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
            this.allGameInfo.SelectionChanged += this.AllGameInfoOnSelectionChanged;
        }

        private async void AllGameInfoOnSelectionChanged(GameInfo gameInfo)
        {
            var currentGameExePath = this.GameExePath.Text;
            var installPath = await this.agentClient.GetInstallPath(gameInfo);

            if (currentGameExePath == this.GameExePath.Text)
            {
                this.GameExePath.Text = installPath;
            }
        }

        private static string GetParentPath(string path)
        {
            try
            {
                return Path.GetDirectoryName(path);
            }
            catch (ArgumentException)
            {
                return null;
            }
            catch (PathTooLongException)
            {
                return null;
            }
        }

        private void GameExeBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Filter = "Executable Files (*.exe)|*.exe",
                InitialDirectory = GetParentPath(this.GameExePath.Text)
            };

            if (dialog.ShowDialog() == true)
            {
                this.GameExePath.Text = dialog.FileName;
            }
        }

        private void LinkFolderBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new Ookii.Dialogs.Wpf.VistaFolderBrowserDialog { SelectedPath = this.LinkFolderPath.Text };

            if (dialog.ShowDialog() == true)
            {
                this.LinkFolderPath.Text = dialog.SelectedPath;
            }
        }

        private void CreateLink_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var gameInfo = this.allGameInfo.Selected;
                if (gameInfo == null)
                {
                    MessageBox.Show("No game selected.", "Blizzbar", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                var linkFolderPath = this.LinkFolderPath.Text.Trim();
                if (linkFolderPath.Length == 0)
                {
                    MessageBox.Show("No link path set.", "Blizzbar", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                if (!Directory.Exists(linkFolderPath))
                {
                    if (MessageBox.Show("Target directory does not exist. Create it?", "Blizzbar", MessageBoxButton.YesNo, MessageBoxImage.Information) == MessageBoxResult.No)
                    {
                        return;
                    }

                    Directory.CreateDirectory(linkFolderPath);
                }

                var gameExePath = this.GameExePath.Text.Trim();
                if (gameExePath.Length == 0)
                {
                    if (MessageBox.Show("No game executable selected. This will create a link with no icon. Is this okay?", "Blizzbar", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
                    {
                        return;
                    }
                }

                var link = new ShellLink();
                var shellLink = (IShellLinkW)link;
                shellLink.SetPath(@"C:\Windows\Explorer.exe");
                shellLink.SetArguments("battlenet://" + gameInfo.UrlName);
                if (gameExePath.Length > 0)
                {
                    shellLink.SetIconLocation(gameExePath, 0);
                }
                shellLink.SetDescription(gameInfo.FullName);

                var propStore = (IPropertyStore)link;
                propStore.SetValue(ref PropertyKey.PKEY_AppUserModel_ID, gameInfo.AppUserModelId);
                propStore.Commit();

                var persistFile = (IPersistFile)link;
                persistFile.Save(Path.Combine(linkFolderPath, gameInfo.FullName + ".lnk"), true);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Unable to create link. Reason:\n" + ex.Message, "Blizzbar", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void RegisterHelper_Click(object sender, RoutedEventArgs e)
        {
            try
            { 
                var runKey = Registry.CurrentUser.CreateSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run");
                var helperPath = Path.Combine(
                    Path.GetDirectoryName(Assembly.GetEntryAssembly().Location),
                    "BlizzbarHelper.exe");
                runKey.SetValue("Blizzbar", helperPath);
                Process.Start(helperPath);
                MessageBox.Show("Helper executable registered.", "Blizzbar", MessageBoxButton.OK);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Unable to register helper.\n" + ex.Message, "Blizzbar", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            try
            {
                this.allGameInfo.LoadGameInfo();
            }
            catch (Exception ex)
            {
                MessageBox.Show(string.Format("Fatal error: Unable to load game information file.\n{0}", ex.Message), "Blizzbar", MessageBoxButton.OK, MessageBoxImage.Error);
                this.Close();
            }
        }
    }
}
