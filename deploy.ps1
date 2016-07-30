$deploy = "$PSScriptRoot\deployment"
$blizzbar = "$deploy\Blizzbar"
$bin = "$PSScriptRoot\bin\Release"
$zip = "$deploy\Blizzbar.zip"

Remove-Item $deploy -Recurse
New-Item $blizzbar -ItemType Directory | Out-Null

$files = @(
    "$bin\Blizzbar.exe",
    "$bin\Blizzbar.exe.config",
    "$bin\BlizzbarHelper.exe",
    "$bin\BlizzbarHelper64.exe",
    "$bin\BlizzbarHooks32.dll",
    "$bin\BlizzbarHooks64.dll",
    "$bin\CsvHelper.dll",
    "$bin\Newtonsoft.Json.dll",
    "$bin\Ookii.Dialogs.Wpf.dll",
    "$bin\games.txt"
)

Copy-Item $files $blizzbar

Compress-Archive $blizzbar $zip
