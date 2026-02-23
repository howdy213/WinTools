#include "ServiceUtils.h"
#include <WinUtils/WinSvcMgr.h>
#include <WinUtils/Console.h>
#include <iostream>
#include "Common.h"
#include <WinUtils/WinUtils.h>
using namespace WinUtils;
using namespace std;
int wmain(int argc, wchar_t* argv[]) {
	Console console;
	console.setLocale();

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