#pragma once
#include <Windows.h>

#include <functional>

template <typename NativeHandle, typename Deleter>
class Handle
{
public:
	Handle() :
		m_handle(nullptr)
	{
	}

	explicit Handle(NativeHandle handle)
		: m_handle(handle)
	{
	}

	~Handle()
	{
		if (!empty())
		{
			Deleter()(m_handle);
		}
	}

	Handle(Handle&& rhs) noexcept :
		m_handle(rhs.m_handle)
	{
		rhs.m_handle = nullptr;
	}

	Handle& operator=(Handle&& rhs) noexcept
	{
		if (this != &rhs)
		{
			m_handle = rhs.m_handle;
			rhs.m_handle = nullptr;
		}

		return *this;
	}

	operator NativeHandle() const
	{
		return m_handle;
	}

	NativeHandle get() const
	{
		return m_handle;
	}

	bool empty() const
	{
		return m_handle == nullptr;
	}

	Handle(const Handle&) = delete;
	Handle& operator=(const Handle&) = delete;

private:
	NativeHandle m_handle;
};


struct HookDeleter
{
	void operator()(HHOOK h) const;
};

struct Hook : Handle<HHOOK, HookDeleter>
{
	Hook(int hookId, HOOKPROC hookProc, HINSTANCE hookModule, DWORD threadId);
};


struct WindowDeleter
{
	void operator()(HWND h) const;
};

struct Window : Handle<HWND, WindowDeleter>
{
	Window(DWORD extendedStyle, const std::wstring& className, const std::wstring& windowName, 
		DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, 
		HINSTANCE module, void* param);
};


struct LibraryDeleter
{
	void operator()(HINSTANCE h) const;
};

struct Library : Handle<HINSTANCE, LibraryDeleter>
{
	explicit Library(const std::wstring& dllName);
};


struct FileMappingDeleter
{
	void operator()(HANDLE h) const;
};

struct FileMapping : Handle<HANDLE, FileMappingDeleter>
{
	FileMapping(HANDLE file, LPSECURITY_ATTRIBUTES attributes, DWORD protection, DWORD maxSize, const std::wstring& name);
};


struct FileMappingViewDeleter
{
	void operator()(void* h) const;
};

struct FileMappingView : Handle<void*, FileMappingViewDeleter>
{
	FileMappingView(HANDLE fileMapping, DWORD access, DWORD offset, size_t length);

	template<typename T>
	const T* as() const
	{
		return reinterpret_cast<const T*>(get());
	}

	template<typename T>
	T* as()
	{
		return reinterpret_cast<T*>(get());
	}
};


struct MutexDeleter
{
	void operator()(HANDLE h) const;
};

struct Mutex : Handle<HANDLE, MutexDeleter>
{
	static Mutex open(DWORD access, bool inherit, const std::wstring& name);
	static Mutex create(LPSECURITY_ATTRIBUTES attrs, bool initialOwner, const std::wstring& name);

	void wait(DWORD timeout) const;

private:
	explicit Mutex(HANDLE h);
};
