#pragma once

#define APP_NAME L"BlizzbarHelper"
#define APP_FRIENDLY_NAME L"Blizzbar Helper"

#define CONFIG_FILE_NAME L"games.txt"

#define SURROGATE_EXE APP_NAME L"64.exe"

#define DLL_NAME L"BlizzbarHooks" BIT_MODIFIER L".dll"

#define MUTEX_PREFIX L"net.austinwagner.blizzbar."

#define X64_MUTEX_NAME MUTEX_PREFIX L"sync_exit"

#define SINGLE_INSTANCE_MUTEX_NAME MUTEX_PREFIX L"single." BIT_MODIFIER