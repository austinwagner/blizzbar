#pragma once
#include <Windows.h>

template <class T>
class ThreadLocal
{
public:
	ThreadLocal()
		: m_tlsIndex(TlsAlloc())
	{
		if (m_tlsIndex == TLS_OUT_OF_INDEXES) throw std::exception();
	}

	~ThreadLocal()
	{
		if (m_tlsIndex != TLS_OUT_OF_INDEXES) TlsFree(m_tlsIndex);
	}

	ThreadLocal(const ThreadLocal&) = delete;
	ThreadLocal& operator=(const ThreadLocal&) = delete;

	ThreadLocal(ThreadLocal&& other) noexcept
		: m_tlsIndex(other.m_tlsIndex)
	{
		other.m_tlsIndex = TLS_OUT_OF_INDEXES;
	}

	ThreadLocal& operator=(ThreadLocal&& other) noexcept
	{
		if (this != &other)
		{
			if (m_tlsIndex != TLS_OUT_OF_INDEXES) TlsFree(m_tlsIndex);
			m_tlsIndex = other.m_tlsIndex;
			other.m_tlsIndex = TLS_OUT_OF_INDEXES;
		}

		return *this;
	}

	template<class... TParams>
	void alloc(TParams&&... param)
	{
		auto p = new T(std::forward<TParams>(param)...);
		if (!TlsSetValue(m_tlsIndex, reinterpret_cast<void*>(p)))
        {
            delete p;
		    throw std::exception();
        }
	}

	void free() { delete get(); }

	const T* operator->() const { return get(); }
	const T& operator*() const { return *get(); }

	T* operator->() { return get(); }
	T& operator*() { return *get(); }

	T* get()
	{
        auto p = reinterpret_cast<T*>(TlsGetValue(m_tlsIndex));

        if (p == nullptr)
        {
            if constexpr (std::is_default_constructible_v<T>)
            {
                p = new T();
                if (!TlsSetValue(m_tlsIndex, reinterpret_cast<void*>(p)))
                {
                    delete p;
                    throw std::exception();
                }
            }
            else
            {
                throw std::exception();
            }
        }
        
        return p;
	}

	const T* get() const
	{
        return const_cast<ThreadLocal<T>>(this)->get();
	}

	bool hasValue() const
	{
		return TlsGetValue(m_tlsIndex) != nullptr;
	}

private:
	DWORD m_tlsIndex = TLS_OUT_OF_INDEXES;
};
