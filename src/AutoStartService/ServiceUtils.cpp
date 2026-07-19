#include <WinUtils/WinPch.h>

#include "ServiceUtils.h"
#include "SessionUtils.h"
#include <filesystem>
#include "Common.h"
#include "ConfigHelper.h"
#include <WinUtils/WinUtils.h>
#include <WinUtils/INI.h>
#include <iostream>

using namespace WinUtils;
using namespace std;

bool keepRunning = false;

namespace {
	SERVICE_STATUS g_status = {};
	SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
	UniqueHandle g_stopEvent;
}

// 从 INI 配置文件中读取启动项
static LaunchItem ReadLaunchItemFromIni(const std::wstring& iniPath) {
	LaunchItem item;
	INIFile iniFile(iniPath);
	INIStructure data;
	if (!iniFile.read(data)) {
		return item;
	}
	if (!data.has(L"Config")) return item;
	const INIMap<wstring>& kvMap = data.get(L"Config");

	item.program = kvMap.get(L"Program");
	item.params = kvMap.get(L"Params");
	item.workDir = kvMap.get(L"WorkDir");
	item.runAsAdmin = kvMap.get_as<bool>(L"RunAsAdmin", false);
	keepRunning = kvMap.get_as<bool>(L"KeepRunning", false);
	int showVal = kvMap.get_as<int>(L"ShowWnd", SW_NORMAL);
	showVal = (showVal >= 0 && showVal <= 10) ? showVal : SW_NORMAL;
	item.showWnd = showVal;

	return item;
}

DWORD WINAPI ServiceWorkerThread(LPVOID) {
	std::wstring configPath = GetConfigFilePath();
	LaunchItem item = ReadLaunchItemFromIni(configPath);

	if (item.program.empty()) {
		OutputDebugStringW(L"配置文件中 Program 为空，无法启动\n");
		return 1;
	}

	item.program = ResolvePath(item.program);
	if (item.program[1] == L':') {  // 绝对路径检查
		if (!filesystem::exists(item.program)) {
			OutputDebugStringW(L"配置文件中 Program 路径不存在，无法启动\n");
			return 1;
		}
	}

	HANDLE hProcess = nullptr;  // 用于 keepRunning 时等待进程

	// 循环重试直到成功启动或收到停止信号
	while (true) {
		DWORD pid = StartProcessInUserSession(item.program, item.params, item.workDir, item.showWnd);
		if (pid != 0) {
			OutputDebugStringW(L"进程启动成功\n");
			if (keepRunning) {
				hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
				if (!hProcess) {
					OutputDebugStringW(L"无法打开进程句柄，无法等待进程结束\n");
					break;
				}
				HANDLE waitHandles[] = { hProcess, g_stopEvent.get() };
				DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
				CloseHandle(hProcess);
			}
			else {
				break;
			}
		}

		// 启动失败，等待500ms重试或收到停止事件
		DWORD waitResult = WaitForSingleObject(g_stopEvent.get(), 500);
		if (waitResult == WAIT_OBJECT_0) {
			OutputDebugStringW(L"服务停止，退出重试循环\n");
			return 2;  // 收到停止命令，退出线程
		}
		// 超时则继续下一次尝试
	}

	return 0;
}

void WINAPI ServiceCtrlHandler(DWORD ctrlCode) {
	if (ctrlCode == SERVICE_CONTROL_STOP && g_status.dwCurrentState == SERVICE_RUNNING) {
		g_status.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(g_statusHandle, &g_status);
		SetEvent(g_stopEvent.get());  // 通知工作线程退出
	}
}

std::wstring GetServiceBinaryPath(const std::wstring& serviceName) {
	// 打开服务管理器（只需 CONNECT 权限）
	UniqueScHandle scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
	if (!scm) {
		return L"";
	}

	// 打开服务（查询配置权限）
	UniqueScHandle svc(OpenServiceW(scm.get(), serviceName.c_str(), SERVICE_QUERY_CONFIG));
	if (!svc) {
		return L"";   // 服务不存在
	}
	DWORD bytesNeeded = 0;
	if (!QueryServiceConfigW(svc.get(), nullptr, 0, &bytesNeeded) &&
		GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		return L"";
	}
	std::vector<BYTE> buffer(bytesNeeded);
	LPQUERY_SERVICE_CONFIGW pConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(buffer.data());
	if (!QueryServiceConfigW(svc.get(), pConfig, bytesNeeded, &bytesNeeded)) {
		return L"";
	}
	if (pConfig->lpBinaryPathName && wcslen(pConfig->lpBinaryPathName) > 0) {
		return std::wstring(pConfig->lpBinaryPathName);
	}
	return L"";
}

void WINAPI ServiceMain(DWORD, LPWSTR*) {
	// 获取动态服务名（必须与分发表中的名称完全一致）
	std::wstring serviceName = GetConfiguredServiceName();

	g_statusHandle = RegisterServiceCtrlHandlerW(serviceName.c_str(), ServiceCtrlHandler);
	if (!g_statusHandle) return;

	g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_status.dwCurrentState = SERVICE_START_PENDING;
	g_status.dwControlsAccepted = 0;
	SetServiceStatus(g_statusHandle, &g_status);

	g_stopEvent.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
	if (!g_stopEvent) {
		g_status.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(g_statusHandle, &g_status);
		return;
	}

	g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_statusHandle, &g_status);

	HANDLE hThread = CreateThread(nullptr, 0, ServiceWorkerThread, nullptr, 0, nullptr);
	if (hThread) {
		// 同时等待线程结束和停止事件
		HANDLE handles[] = { hThread, g_stopEvent.get() };
		WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		CloseHandle(hThread);
	}

	g_status.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(g_statusHandle, &g_status);
}