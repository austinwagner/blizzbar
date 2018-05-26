#pragma once
#include <string>
#include <cstdarg>

#ifdef _WIN64
#define BIT_MODIFIER L"64"
#else
#define BIT_MODIFIER L"32"
#endif

#define APP_GUID L"84fed2b1-9493-4e53-a569-9e5e5abe7da2"

#define MMAP_NAME L"Local\\" APP_GUID BIT_MODIFIER

template <size_t N>
class FixedCapacityString {
	// Keep in mind that N is the size of the backing array, therefore the maximum
	// string size is N - 1.
public:
	using value_type = wchar_t;

	FixedCapacityString() : size_{ 0 } { data_[size_] = 0; }

	FixedCapacityString(const std::wstring& str)
		: size_{ str.size() }
	{
		CheckBounds(size_);
		UncheckedCopy(str);
	}

	FixedCapacityString(const FixedCapacityString& other)
		: size_{ other.size_ }
	{
		UncheckedCopy(other);
	}

	FixedCapacityString& operator=(const FixedCapacityString& other)
	{
		if (this != &other)
		{
			size_ = other.size_;
			UncheckedCopy(other);
		}

		return *this;
	}

	// For this class a move and copy are one and the same
	FixedCapacityString(FixedCapacityString&&) = delete;
	FixedCapacityString operator=(FixedCapacityString&&) = delete;

	wchar_t* data() noexcept { return data_; }
	const wchar_t* data() const noexcept { return data_; }
	const wchar_t* c_str() const noexcept { return data_; }
	size_t size() const noexcept { return size_; }
	size_t capacity() const noexcept { return N; }
	void resize(size_t size) 
	{
		CheckBounds(size);
		size_ = size;
		data_[size_] = 0; // We follow the null termination guarantee of an std::string
	}

	wchar_t operator[](size_t i) const noexcept { return data_[i]; }
	wchar_t& operator[](size_t i) noexcept { return data_[i]; }

	void sprintf(const wchar_t* fmt, ...)
	{
		va_list va;
		va_start(va, fmt);
		int x = __crt_countof(data_);
		size_ = vswprintf(data_, __crt_countof(data_), fmt, va);
		va_end(va);
	}

private:
	template <class String>
	void UncheckedCopy(const String& src) 
	{ 
		std::wmemcpy(data_, src.c_str(), src.size() + 1);
	}

	void CheckBounds(size_t s) { if (s >= N) throw std::out_of_range(); }

	size_t size_;
	wchar_t data_[N];
};

// File format: GameName|UrlShortName|AgentShortName|LauncherExe|32BitExe|64BitExe
// Unneeded fields are excluded. exe contains the executable for the current architecture
// and is what the hook library uses. exeOtherArch is stored for the helper to find existing 
// windows for either architecture.
struct GameInfo
{
	FixedCapacityString<32> exe;
	FixedCapacityString<64> appUserModelId;
	FixedCapacityString<128> relaunchCommand;
	FixedCapacityString<64> gameName;
};

class Config
{
public:
	class Header
	{
	public:
		Header(size_t elemCount) : elemCount_(elemCount) { }
		size_t size_bytes() const { return sizeof(Header) + sizeof(GameInfo) * elemCount_; }

	private:
		size_t elemCount_;
		friend class Config;
	};

	template <class Collection>
	void copy_from(const Collection& c)
	{
		static_assert(std::is_same_v<Collection::value_type, GameInfo>);

		header_.elemCount_ = c.size();
		std::memcpy(gameInfoArr_, c.data(), header_.size_bytes());
	}

	size_t size() const { return header_.elemCount_; }
	size_t capacity() const { return size(); }
	GameInfo* operator[](size_t i) { return &gameInfoArr_[i]; }
	const GameInfo* operator[](size_t i) const { return &gameInfoArr_[i]; }

	// Since this structure contains a VLA, none of these default methods make sense
	Config() = delete;
	~Config() = delete;
	Config(const Config&) = delete;
	Config operator=(const Config&) = delete;
	Config(Config&&) = delete;
	Config operator=(Config&&) = delete;

private:
	Header header_;
	GameInfo gameInfoArr_[1]; // VLA. Size 1 bececause VLAs aren't in the C++ spec
};
