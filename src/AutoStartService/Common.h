#pragma once

#include <windows.h>
#include <wtsapi32.h>
#include <memory>
#include <string>

constexpr auto SERVICE_NAME = L"AutoStartService";
constexpr auto CONFIG_FILE_NAME = L"asconfig.ini";

struct HandleDeleter {
	void operator()(HANDLE h) const { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); }
};
struct ScHandleDeleter {
	void operator()(SC_HANDLE h) const { if (h) CloseServiceHandle(h); }
};
struct WtsMemoryDeleter {
	void operator()(void* p) const { if (p) WTSFreeMemory(p); }
};

using UniqueHandle = std::unique_ptr<void, HandleDeleter>;
using UniqueScHandle = std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, ScHandleDeleter>;
using UniqueWtsMemory = std::unique_ptr<WTS_SESSION_INFO, WtsMemoryDeleter>;