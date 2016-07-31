# Blizzbar

Blizzbar is a tool to allow pinning of games that use the Battle.net launcher to the Windows taskbar. This addresses flaws in other methods of pinning:

* Uses the Battle.net launcher's login credentials
* Doesn't create a separate icon in the taskbar for the running game

## Installation

Simply download the zip archive from the Github Releases page and extract where you'd like to keep the files.

## Usage

![Example Usage Demo](http://imgur.com/ZsdPZNw.gif "Example Usage Demo")

Blizzbar requires that the shortcut files are in a specific format, so you need to use the `Blizzbar.exe` GUI application to create the shortcuts.

The first time you run the application, you'll want to press the button labelled `Register Helper to Run at Startup`. If the helper process isn't running, then the game will split off into another icon when launched.

To create a shortcut, select the name of the game you'd like to create a shortcut for. The other fields are automatically filled out for you, but can be tweaked if required (the location of the game is only needed so the shortcut will have an icon).

Clicking `Create Link` will create the shortcut in the specified folder. Windows does not allow applications to pin themselves to the taskbar, so by default this will be your desktop.

From here, all you need to do is drag the shortcut onto the taskbar and delete the one that was placed on your desktop.

![End Rusult](http://imgur.com/XpOwTJc.png "End Rusult")

### Other Notes

If you move the folder containing the Blizzbar files, you will need to use `Register Helper to Run at Startup` again to point Windows to the new file location.

If you'd like to start the helper process manually, always launch `BlizzbarHelper.exe`, not `BlizzbarHelper64.exe`.

As long as the helper process is running, no matter how you launch a game, it will merge into the correct taskbar button.

## Building

Requires VS2015 or later with C++ tools installed.

Since the final package requires both 32-bit and 64-bit binaries, you need to build both:

```bat
msbuild Blizzbar.sln /p:Configuration=Release /p:Platform=x86
msbuild Blizzbar.sln /p:Configuration=Release /p:Platform=x64
```

The files will be placed in `bin\Release`.

## How it Works

### Background

#### Lanching a Game
The Battle.net launcher allows games to be launched without forcing you to click the `Play` button if it is invoked using the URI scheme `battlenet:`. For example creating a shortcut pointing to `C:\Windows\explorer.exe battlenet://WTCG` will allow you to launch Hearthstone. Since this method of starting a game goes through the launcher, the game takes advantage of the credentials you've used to login to the launcher to not require you to re-enter them.

The downside to launching a game this way is that Windows doesn't know that the game and the shortcut used to launch the game are related. The taskbar ends up having two buttons on it. One for the shortcut and one for the actual game window.

#### AppUserModelID

The way that Windows determines which shortcuts and windows need to be grouped together is by using the "Application User Model ID" of the shortcut and the window. Each process can have a default ID and each window can have their ID overridden.

By default, Windows automatically determines the ID for both pinned shortcuts and new processes. Windows however does not allow a process to access this automatically assigned ID.

### Blizzbar GUI

The GUI serves two main purposes. One is to make it easy to set the helper process to launch at startup. The other is to create shortcuts. The reason this is done through a custom GUI instead of just having users create a shortcut using the normal Windows GUI is that Blizzbar sets a special property on the shortcut.

The `IShellLink` COM class allows extra properties to be set through an `IPropertyStore`. One of these properties is named `PKEY_AppUserModel_ID`. When this is set on a shortcut, the process launched through the shortcut will take on that AppUserModelID and any windows later assigned that same ID will merge into the shortcut. Blizzbar assigns the links it creates an ID of the form `[App_GUID].[Game_ID]` where `App_GUID` is a constant and `Game_ID` is the same ID that is used in the `battlenet://` URL.

### Blizzbar Helper

The helper process's main purpose is to install the hook function in `BlizzbarHooks.dll`. The process needs to stay alive for the hook to remain in effect. There is both a 32-bit and a 64-bit version of the helper so hooks can be installed in both types of processes. The 32-bit version launches the 64-bit version automatically and ties their lifetimes together.

### Blizzbar Hook

Windows allows processes to set hooks that monitor window messages using the `SetWindowsHookEx` function. This is handy here because instead of polling for new windows (which includes enumerating over all of the open windows and opening process handles), Blizzbar can wait around to be notified of the message `HSHELL_WINDOWCREATED`.

Whenever the message is recieved `SHGetPropertyStoreForWindow` is called so `PKEY_AppUserModel_ID` can be set to the same value that was set on the shortcut.

One downside to this method of setting the AppUserModelID is that the hook DLL is installed into every process that creates a top-level window after the hook is registered. For that reason, there are a few optimizations in the library:

* No linkage against the C Runtime
* Configuration is loaded ahead of time by the helper and stored as a C structure in a memory-mapped file
* Library checks as soon as it's attached if it should care about the process it's in and if it doesn't then the only overhead of the hook (besides existing in the hook chain) is a null check
