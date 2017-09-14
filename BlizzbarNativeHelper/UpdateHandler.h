#pragma once
#include <memory>

#include "Handle.h"
#include "../BlizzbarCommon.h"

class UpdateHandler
{
public:
	UpdateHandler();

	void SetGameInfo(const GameInfo gameInfoArr[], size_t gameInfoArrLen);

private:
	void ChangeExistingWindowsAppIds() const;

	Mutex writeLock_;
	FileMapping syncMap_;
	FileMappingView syncMapView_;
	MmapSyncData* mmapSyncData_;
	std::unique_ptr<FileMapping> mmap_;
	std::unique_ptr<FileMappingView> mmapView_;
	Config* mappedConfig_;
};
