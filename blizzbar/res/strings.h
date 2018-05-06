#pragma once

#ifdef _WIN64
#define BITNESS L"64"
#else
#define BITNESS L"32"
#endif

#define APP_NAME L"blizzbar"
#define APP_FRIENDLY_NAME L"Blizzbar"

#define CONFIG_FILE_NAME L"games.txt"

#define SURROGATE_EXE APP_NAME L"-surrogate.exe"

#define DLL_NAME L"blizzbar-hook-" BITNESS L".dll"

#define MUTEX_PREFIX L"net.austinwagner.blizzbar."

#define X64_MUTEX_NAME MUTEX_PREFIX L"sync_exit"

#define SINGLE_INSTANCE_MUTEX_NAME MUTEX_PREFIX L"single." BITNESS