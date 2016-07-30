#pragma once
#include <wchar.h>

#define DWORD_MAX 4294967295

#ifdef _WIN64
#define BIT_MODIFIER L"64"
#else
#define BIT_MODIFIER L"32"
#endif

#define APP_GUID L"84fed2b1-9493-4e53-a569-9e5e5abe7da2"

#define MMAP_NAME L"Local\\" APP_GUID BIT_MODIFIER

// File format: GameName|UrlShortName|AgentShortName|LauncherExe|32BitExe|64BitExe
// Unneeded fields are excluded. exe contains the executable for the current architecture
// and is what the hook library uses. exeOtherArch is stored for the helper to find existing 
// windows for either architecture.
typedef struct tagGameInfo
{
	wchar_t exe32[32];
	wchar_t exe64[32];
	wchar_t appUserModelId[64];
} GameInfo;

typedef struct tagConfig
{
	size_t elemCount;

#pragma warning(suppress: 4200) // Doesn't matter that this is non-standard
	GameInfo gameInfoArr[];
} Config;