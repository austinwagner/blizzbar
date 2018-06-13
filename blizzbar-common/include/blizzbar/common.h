#pragma once
#include <cstdarg>
#include <string>

#ifdef _WIN64
#define BIT_MODIFIER L"64"
#else
#define BIT_MODIFIER L"32"
#endif

#define APP_GUID L"84fed2b1-9493-4e53-a569-9e5e5abe7da2"

#define MMAP_NAME L"Local\\" APP_GUID BIT_MODIFIER

template <size_t N>
class FixedCapacityString {
    // Keep in mind that N is the size of the backing array, therefore the
    // maximum string size is N - 1.
public:
    static_assert(N > 0,
        "FixedCapacityString needs at have a size of at least 1 to hold the "
        "null terminator.");
    using value_type = wchar_t;

    FixedCapacityString()
        : m_size{ 0 }
    {
        m_data[0] = 0;
    }

    FixedCapacityString(const std::wstring& str)
        : m_size{ str.size() }
    {
        checkBounds(m_size);
        uncheckedCopy(str);
    }

    FixedCapacityString(const FixedCapacityString& other)
        : m_size{ other.m_size }
    {
        uncheckedCopy(other);
    }

    FixedCapacityString& operator=(const FixedCapacityString& other)
    {
        if (this != &other) {
            m_size = other.m_size;
            uncheckedCopy(other);
        }

        return *this;
    }

    // For this class a move and copy are one and the same
    FixedCapacityString(FixedCapacityString&&) = delete;
    FixedCapacityString operator=(FixedCapacityString&&) = delete;

    wchar_t* data() noexcept { return m_data; }
    const wchar_t* data() const noexcept { return m_data; }
    const wchar_t* c_str() const noexcept { return m_data; }
    size_t size() const noexcept { return m_size; }
    size_t capacity() const noexcept { return N; }
    void resize(size_t size)
    {
        CheckBounds(size);
        m_size = size;
		
		// We follow the null termination guarantee of an std::string
        m_data[m_size] = 0;
    }

    wchar_t operator[](size_t i) const noexcept { return m_data[i]; }
    wchar_t& operator[](size_t i) noexcept { return m_data[i]; }

    void sprintf(const wchar_t* fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        int x = __crt_countof(m_data);
        m_size = vswprintf(m_data, __crt_countof(m_data), fmt, va);
        va_end(va);
    }

private:
    template <class String>
    void uncheckedCopy(const String& src)
    {
        std::wmemcpy(m_data, src.c_str(), src.size() + 1);
    }

    void checkBounds(size_t s)
    {
        if (s >= N) throw std::out_of_range();
    }

    size_t m_size;
    wchar_t m_data[N];
};

// File format:
// GameName|UrlShortName|AgentShortName|LauncherExe|32BitExe|64BitExe 
// 
// Unneeded fields are excluded. exe contains the executable for the current
// architecture and is what the hook library uses. exeOtherArch is stored for
// the helper to find existing windows for either architecture.
struct GameInfo {
    FixedCapacityString<32> exe;
    FixedCapacityString<64> appUserModelId;
    FixedCapacityString<128> relaunchCommand;
    FixedCapacityString<64> gameName;
};

class Config {
public:
    class Header {
    public:
        Header(size_t elemCount)
            : m_elemCount(elemCount)
        {
        }
		
        size_t sizeBytes() const
        {
            return sizeof(Header) + sizeof(GameInfo) * m_elemCount;
        }

    private:
        size_t m_elemCount;
        friend class Config;
    };

    template <class Collection>
    void copyFrom(const Collection& c)
    {
        static_assert(
            std::is_same_v<typename Collection::value_type, GameInfo>);

        m_header.m_elemCount = c.size();
        std::memcpy(m_gameInfoArr, c.data(), m_header.sizeBytes());
    }

    size_t size() const { return m_header.m_elemCount; }
    size_t capacity() const { return size(); }

#pragma warning(push)
#pragma warning(disable : 26446)
    GameInfo* operator[](size_t i) { return &m_gameInfoArr[i]; }
    const GameInfo* operator[](size_t i) const { return &m_gameInfoArr[i]; }
#pragma warning(pop)

    // Since this structure contains a VLA, none of these default methods make
    // sense
    Config() = delete;
    ~Config() = delete;
    Config(const Config&) = delete;
    Config operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config operator=(Config&&) = delete;

private:
    Header m_header;
    GameInfo
        m_gameInfoArr[1]; // VLA. Size 1 bececause VLAs aren't in the C++ spec
};
