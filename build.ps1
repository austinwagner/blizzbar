Push-Location "$PSScriptRoot\build"


if ((Get-Item x86 -ErrorAction Ignore) -eq $null) {
    New-Item x86 -ItemType Directory
}
Push-Location x86 -ErrorAction Stop
&cmake.exe -G 'Visual Studio 15 2017' '..\..'
Pop-Location


if ((Get-Item x64 -ErrorAction Ignore) -eq $null) {
    New-Item x64 -ItemType Directory
}
Push-Location x64 -ErrorAction Stop
&cmake.exe -G 'Visual Studio 15 2017 Win64' '..\..'
Pop-Location


Pop-Location