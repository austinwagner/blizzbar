#pragma once
#include <string>
#include <array>
#include <stdlib.h>

#define DWORD_MAX 4294967295

//#ifdef _WIN64
//#define BIT_MODIFIER L"64"
//#else
//#define BIT_MODIFIER L"32"
//#endif
//
#define APP_GUID L"84fed2b1-9493-4e53-a569-9e5e5abe7da2"

#define MMAP_BASE_NAME L"Local\\" APP_GUID

#define MMAP_SYNC_NAME MMAP_BASE_NAME L".sync"

#define MMAP_NAME_1 MMAP_BASE_NAME L".1"

#define MMAP_NAME_2 MMAP_BASE_NAME L".2"

#define MMAP_WRITE_MUTEX_NAME APP_GUID L".write_mutex"

#define MMAP_READ_MUTEX_NAME APP_GUID L".read_mutex"

//static const std::wstring AppGuid = L"84fed2b1-9493-4e53-a569-9e5e5abe7da2";
//static const std::wstring MmapBaseName = L"Local\\" + AppGuid;
//static const std::wstring MmapSyncName = MmapBaseName + L".sync";
//static const std::wstring MmapName1 = MmapBaseName + L".1";
//static const std::wstring MmapName2 = MmapBaseName + L".2";
//static const std::wstring MmapWriteMutexName = AppGuid + L".write_mutex";
//static const std::wstring MmapReadMutexName = AppGuid + L"read_mutex";

struct GameInfo
{
	std::array<wchar_t, 256> installPath;
	std::array<wchar_t, 64> exeRegex;
	std::array<wchar_t, 64> appUserModelId;
	std::array<wchar_t, 64> relaunchCommand;
	std::array<wchar_t, 256> iconPath;
};

struct Config
{
	uint32_t elemCount;

#pragma warning(suppress: 4200) // Doesn't matter that this is non-standard
	GameInfo gameInfoArr[];
};

struct MmapSyncData 
{
	uint16_t numReaders;
	std::array<wchar_t, _countof(MMAP_NAME_1)> mmapName;
};

static_assert(sizeof(MMAP_NAME_1) == sizeof(MMAP_NAME_2), "Length of memory mapped file names must be exactly equal.");

namespace detail {
	// https://gist.github.com/mrts/5890888
	template <class Function>
	class ScopeGuard
	{
	public:
		ScopeGuard(Function f) :
			_guardFunction(std::move(f)),
			_active(true)
		{ }

		~ScopeGuard() {
			if (_active) {
				_guardFunction();
			}
		}

		ScopeGuard(ScopeGuard&& rhs) :
			_guardFunction(std::move(rhs._guardFunction)),
			_active(rhs._active) {
			rhs.dismiss();
		}

		void dismiss() {
			_active = false;
		}

	private:
		Function _guardFunction;
		bool _active;

		ScopeGuard() = delete;
		ScopeGuard(const ScopeGuard&) = delete;
		ScopeGuard& operator=(const ScopeGuard&) = delete;
	};

	enum class ScopeGuardOnExit {};

	template <typename Fun>
	ScopeGuard<Fun> operator+(ScopeGuardOnExit, Fun&& fn) {
		return ScopeGuard<Fun>(std::forward<Fun>(fn));
	}
}

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __COUNTER__)
#define SCOPE_EXIT auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) = ::detail::ScopeGuardOnExit() + [&]()
