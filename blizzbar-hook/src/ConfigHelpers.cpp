#include "ConfigHelpers.h"

#include <Windows.h>

#include <exception>

namespace {
template <class T>
T* map(void* mmap, size_t len = 0)
{
    auto view
        = MapViewOfFile(mmap, FILE_MAP_READ, 0, 0, len == 0 ? sizeof(T) : len);
    if (view == nullptr) {
        throw std::exception();
    }
    return reinterpret_cast<T*>(view);
}
}

ConfigFileMapping::ConfigFileMapping()
    : m_mmapHandle(nullptr)
{
}

ConfigFileMapping::ConfigFileMapping(const wchar_t* name)
    : m_mmapHandle(OpenFileMappingW(FILE_MAP_READ, false, name))
{
    if (m_mmapHandle == nullptr) {
        throw std::exception();
    }
}

ConfigFileMapping::ConfigFileMapping(ConfigFileMapping&& other) noexcept
    : m_mmapHandle(other.m_mmapHandle)
{
    other.m_mmapHandle = nullptr;
}

ConfigFileMapping& ConfigFileMapping::operator=(
    ConfigFileMapping&& other) noexcept
{
    if (this != &other) {
        if (m_mmapHandle != nullptr) {
            CloseHandle(m_mmapHandle);
        }
        m_mmapHandle = other.m_mmapHandle;
        other.m_mmapHandle = nullptr;
    }

    return *this;
}

ConfigFileMapping::~ConfigFileMapping()
{
    if (m_mmapHandle != nullptr) {
        CloseHandle(m_mmapHandle);
    }
}

void* ConfigFileMapping::handle() const { return m_mmapHandle; }

ConfigFileView::ConfigFileView()
    : m_view(nullptr)
{
}

ConfigFileView::ConfigFileView(const ConfigFileMapping& mmap)
{
    auto header = map<Config::Header>(mmap.handle());
    auto len = header->sizeBytes();
    UnmapViewOfFile(header);

    m_view = map<Config>(mmap.handle(), len);
}

ConfigFileView::ConfigFileView(ConfigFileView&& other) noexcept
    : m_view(other.m_view)
{
    other.m_view = nullptr;
}

ConfigFileView& ConfigFileView::operator=(ConfigFileView&& other) noexcept
{
    if (this != &other) {
        if (m_view != nullptr) {
            UnmapViewOfFile(m_view);
        }
        m_view = other.m_view;
        other.m_view = nullptr;
    }

    return *this;
}

ConfigFileView::~ConfigFileView()
{
    if (m_view != nullptr) {
        UnmapViewOfFile(m_view);
    }
}

const Config* ConfigFileView::operator->() const
{
    return reinterpret_cast<Config*>(m_view);
}

const Config& ConfigFileView::operator*() const
{
    return *reinterpret_cast<Config*>(m_view);
}
