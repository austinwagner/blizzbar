#include "Handle.h"

#include "Win32Exception.h"

void HookDeleter::operator()(HHOOK h) const
{
	UnhookWindowsHookEx(h);

	// Wake all message loops to get Windows to release the DLL
	SendMessageTimeoutW(HWND_BROADCAST, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 1000, nullptr);
}

Hook::Hook(int hookId, HOOKPROC hookProc, HINSTANCE hookModule, DWORD threadId)
	: Handle(SetWindowsHookExW(hookId, hookProc, hookModule, threadId))
{
	if (empty())
	{
		throw Win32Exception("Failed to install Windows hook.");
	}
}


void WindowDeleter::operator()(HWND h) const
{
	DestroyWindow(h);
}

Window::Window(DWORD extendedStyle, const std::wstring& className, const std::wstring& windowName, 
	DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE module, void* param)
	: Handle(CreateWindowExW(extendedStyle, className.c_str(), windowName.c_str(), style, x, y, width, height, parent, menu, module, param))
{
	if (empty())
	{
		throw Win32Exception("Failed to create window.");
	}
}


void LibraryDeleter::operator()(HINSTANCE h) const
{
	FreeLibrary(h);
}

Library::Library(const std::wstring& dllName)
	: Handle(LoadLibraryW(dllName.c_str()))
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

Mutex::Mutex(HANDLE h)
	: Handle(h)
{
}

void MutexDeleter::operator()(HANDLE h) const
{
	CloseHandle(h);
}

Mutex Mutex::open(DWORD access, bool inherit, const std::wstring& name)
{
	Mutex mutex(OpenMutexW(access, inherit, name.c_str()));
	if (mutex.empty())
	{
		throw Win32Exception("Failed to open mutex.");
	}

	return mutex;
}

Mutex Mutex::create(LPSECURITY_ATTRIBUTES attrs, bool initialOwner, const std::wstring& name)
{
	Mutex mutex(CreateMutexW(attrs, initialOwner, name.c_str()));
	if (mutex.empty())
	{
		throw Win32Exception("Failed to create mutex.");
	}

	return mutex;
}

void Mutex::wait(DWORD timeout) const
{
	WaitForSingleObject(get(), timeout);
}

void RegistryKeyDeleter::operator()(HKEY h) const
{
    RegCloseKey(h);
}

std::optional<RegistryKey> RegistryKey::open(HKEY baseKey, const std::wstring& subKeyPath, REGSAM desiredSam)
{
    HKEY key;
    const auto res = RegOpenKeyExW(baseKey, subKeyPath.c_str(), 0, desiredSam, &key);
    return res == ERROR_SUCCESS
        ? std::optional{RegistryKey{key}}
        : std::nullopt;
}

RegistryKey RegistryKey::create(HKEY baseKey, const std::wstring& subKeyPath,
    std::wstring* className, DWORD options, REGSAM desiredSam, LPSECURITY_ATTRIBUTES attrs, DWORD* disposition)
{
    HKEY key;
    const auto res = RegCreateKeyExW(baseKey, subKeyPath.c_str(), 0, (className ? className->data() : nullptr),
        options, desiredSam, attrs, &key, disposition);
    if (res != ERROR_SUCCESS) throw Win32Exception(res, "Failed to create registry key.");
    return RegistryKey{key};
}

RegistryKey::RegistryKey(HKEY h)
    : Handle(h)
{
}
