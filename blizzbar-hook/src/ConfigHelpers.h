#pragma once
#include <blizzbar_common.h>

class ConfigFileMapping
{
public:
	ConfigFileMapping();
	ConfigFileMapping(const wchar_t* name);
	ConfigFileMapping(ConfigFileMapping&& other) noexcept;
	ConfigFileMapping& operator=(ConfigFileMapping&& other) noexcept;

	~ConfigFileMapping();

	ConfigFileMapping(const ConfigFileMapping&) = delete;
	ConfigFileMapping& operator=(const ConfigFileMapping&) = delete;

	void* handle() const;

private:
	void* m_mmapHandle;
};

class ConfigFileView
{
public:
	ConfigFileView();
	ConfigFileView(const ConfigFileMapping& mmap);
	ConfigFileView(ConfigFileView&& other) noexcept;
	ConfigFileView& operator=(ConfigFileView&& other) noexcept;

	~ConfigFileView();

	const Config* operator->() const;
	const Config& operator*() const;

	ConfigFileView(const ConfigFileView&) = delete;
	ConfigFileView operator=(const ConfigFileView&) = delete;

private:

	Config* m_view;
};
