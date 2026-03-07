#include "pch.h"

#include "HotspotDialog.h"
#include "HotspotHelper.h"

#include <WinUtils/Console.h>
#include <WinUtils/CmdParser.h>
#include <WinUtils/StrConvert.h>
#pragma comment(lib, "WindowsApp.lib")
using namespace WinUtils;
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
	Console console;
	console.attach();

	CmdParser parser;
	(void)parser.parse(lpCmdLine);

	try {
		if (parser.hasCommand(L"-start")) {
			auto args = parser.result()[L"-start"];
			if (args.size() == 2) {
				EasyStart(
					CmdParser::removeQuotation(args[0]),
					CmdParser::removeQuotation(args[1])
				);
			}
			else EasyStart();
		}
		else if (parser.hasCommand(L"-stop")) StopHotspot();
		else {
			HotspotDialog dlg(hInstance);
			return dlg.DoModal();
		}
	}
	catch (const std::exception& e) {
		std::wcerr << L"Unhandled exception: " << AnsiToWideString(e.what()) << std::endl;
		return 1;
	}
	catch (const winrt::hresult_error& e) {
		std::wcerr << L"Unhandled WinRT error: " << e.message().c_str() << std::endl;
		return 1;
	}
	return 0;
}