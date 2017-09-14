#pragma once
#include <wchar.h>
#include <stdint.h>

#define DWORD_MAX 4294967295

#ifdef _WIN64
#define BIT_MODIFIER L"64"
#else
#define BIT_MODIFIER L"32"
#endif

#define APP_GUID L"84fed2b1-9493-4e53-a569-9e5e5abe7da2"

#define MMAP_BASE_NAME L"Local\\" APP_GUID L"." BIT_MODIFIER

#define MMAP_SYNC_NAME MMAP_BASE_NAME L".sync"

#define MMAP_NAME_1 MMAP_BASE_NAME L".1"

#define MMAP_NAME_2 MMAP_BASE_NAME L".2"

#define MMAP_MUTEX_BASE_NAME APP_GUID L"." BIT_MODIFIER

#define MMAP_WRITE_MUTEX_NAME MMAP_MUTEX_BASE_NAME L".write_mutex"

#define MMAP_READ_MUTEX_NAME MMAP_MUTEX_BASE_NAME L".read_mutex"

// File format: GameName|UrlShortName|AgentShortName|LauncherExe|32BitExe|64BitExe
// Unneeded fields are excluded. exe contains the executable for the current architecture
// and is what the hook library uses. exeOtherArch is stored for the helper to find existing 
// windows for either architecture.
typedef struct tagGameInfo
{
	wchar_t exe32[32];
	wchar_t exe64[32];
	wchar_t appUserModelId[64];
	wchar_t relaunchCommand[64];
} GameInfo;

typedef struct tagConfig
{
	size_t elemCount;

#pragma warning(suppress: 4200) // Doesn't matter that this is non-standard
	GameInfo gameInfoArr[];
} Config;

typedef struct tagMmapSyncData 
{
	int32_t numReaders;
	wchar_t mmapName[sizeof(MMAP_NAME_1)];
} MmapSyncData;

#ifdef __cplusplus
#define BBCOMMON_STATIC_ASSERT static_assert
#else
#define BBCOMMON_STATIC_ASSERT(COND,MSG) typedef char static_assertion[(COND)?1:-1]
#endif

BBCOMMON_STATIC_ASSERT(sizeof(MMAP_NAME_1) == sizeof(MMAP_NAME_2), "Length of memory mapped file names must be exactly equal.");

#undef BBCOMMON_STATIC_ASSERT
