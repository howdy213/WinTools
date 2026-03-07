#include <filesystem>
#include <iostream>

#include <WinUtils/WinSvcMgr.h>
#include <WinUtils/Console.h>
#include <WinUtils/WinUtils.h>

#include "ServiceUtils.h"
#include "Common.h"
using namespace WinUtils;
using namespace std;

static void CreateSampleIni(const wstring& path) {
	FILE* f = nullptr;
	if (_wfopen_s(&f, path.c_str(), L"w, ccs=UTF-8") == 0 && f) {
		wstring sample = L"[Config]\nProgram=..\\program.exe\nParams=arg1 arg2\nWorkDir=C:\\\nShowWnd=7\n";
		fwrite(sample.c_str(), sizeof(wchar_t), sample.length(), f);
		fclose(f);
	}
}

int wmain(int argc, wchar_t* argv[]) {
	Console console;
	console.setLocale();

	std::wstring configPath = WinUtils::GetCurrentProcessDir() + CONFIG_FILE_NAME;
	if (!filesystem::exists(configPath)) {
		CreateSampleIni(configPath);
		return 0;
	}
	if (argc == 2) {
		RequireAdminPrivilege();
		wstring arg = argv[1];
		WinSvcMgr svcMgr(SERVICE_NAME);

		if (arg == L"/install") {
			wstring modulePath = GetCurrentProcessPath();
			if (svcMgr.Install(modulePath)) {
				wcout << L"服务安装成功\n";
				return 0;
			}
			else {
				wcout << L"服务安装失败，错误码: " << svcMgr.GetLastError() << endl;
				return 1;
			}
		}

		if (arg == L"/uninstall") {
			if (svcMgr.Uninstall()) {
				wcout << L"服务卸载成功\n";
				return 0;
			}
			else {
				wcout << L"服务卸载失败，错误码: " << svcMgr.GetLastError() << endl;
				return 1;
			}
		}

		wcout << L"无效参数！支持：/install 或 /uninstall\n";
		return 1;
	}

	// 启动服务调度器
	SERVICE_TABLE_ENTRYW table[] = {
		{const_cast<LPWSTR>(SERVICE_NAME), ServiceMain},
		{nullptr, nullptr}
	};
	if (!StartServiceCtrlDispatcherW(table)) {
		wcout << L"启动服务调度器失败，错误码:" << GetLastError() << endl;
		return 1;
	}
	return 0;
}