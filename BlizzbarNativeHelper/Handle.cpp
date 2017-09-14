#include "Handle.h"
#include "Win32Exception.h"

void GenericHandleDeleter::operator()(HANDLE h) const
{
	CloseHandle(h);
}

GenericHandle::GenericHandle(HANDLE h)
	: Handle(h)
{
}

void FileMappingDeleter::operator()(HANDLE h) const
{
	CloseHandle(h);
}


FileMapping::FileMapping(HANDLE file, LPSECURITY_ATTRIBUTES attributes, DWORD protection, DWORD maxSize, const std::wstring& name)
	: Handle(CreateFileMappingW(file, attributes, protection, 0, maxSize, name.c_str()))
{
	if (empty())
	{
		throw Win32Exception("Failed to map file.");
	}
}


void FileMappingViewDeleter::operator()(void* h) const
{
	UnmapViewOfFile(h);
}


FileMappingView::FileMappingView(HANDLE fileMapping, DWORD access, DWORD offset, size_t length)
	: Handle(MapViewOfFile(fileMapping, access, 0, offset, length))
{
	if (empty())
	{
		throw Win32Exception("Failed to map view of file.");
	}
}

Mutex::Mutex(HANDLE h, bool isOwner) : Handle(h), isOwner_(isOwner)
{
}

void MutexDeleter::operator()(HANDLE h) const
{
	CloseHandle(h);
}

Mutex Mutex::open(DWORD access, bool inherit, const std::wstring& name)
{
	Mutex mutex(OpenMutexW(access, inherit, name.c_str()), false);
	if (mutex.empty())
	{
		throw Win32Exception("Failed to open mutex.");
	}

	return mutex;
}

Mutex Mutex::create(LPSECURITY_ATTRIBUTES attrs, bool initialOwner, const std::wstring& name)
{
	Mutex mutex(CreateMutexW(attrs, initialOwner, name.c_str()), true);
	if (mutex.empty())
	{
		throw Win32Exception("Failed to create mutex.");
	}

	return mutex;
}

Mutex Mutex::acquire() const
{
	WaitForSingleObject(get(), INFINITE);
	return Mutex(get(), false);
}

Mutex::~Mutex()
{
	if (m_handle != nullptr)
	{
		ReleaseMutex(m_handle);
	}

	if (!isOwner_)
	{
		m_handle = nullptr;
	}
}
