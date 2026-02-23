#include "SessionUtils.h"
#include <windows.h>
#include <userenv.h>
#include <wtsapi32.h>
#pragma comment(lib, "wtsapi32.lib")

bool GetActiveSessionId(DWORD& sessionId) {
	sessionId = 0;
	DWORD count = 0;
	PWTS_SESSION_INFO pInfo = nullptr;
	if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pInfo, &count))
		return false;
	UniqueWtsMemory mem(pInfo);
	for (DWORD i = 0; i < count; ++i) {
		if (pInfo[i].State == WTSActive) {
			sessionId = pInfo[i].SessionId;
			return true;
		}
	}
	return false;
}

bool GetUserTokenFromSessionId(DWORD sessionId, UniqueHandle& token) {
	HANDLE raw = nullptr;
	if (!WTSQueryUserToken(sessionId, &raw)) return false;
	token.reset(raw);
	return true;
}

DWORD StartProcessInUserSession(const std::wstring& appPath, const std::wstring& args,
	const std::wstring& workingDir, int showWnd) {
	DWORD sessionId = 0;
	if (!GetActiveSessionId(sessionId)) return false;

	UniqueHandle userToken;
	if (!GetUserTokenFromSessionId(sessionId, userToken)) return false;

	HANDLE dupToken = nullptr;
	if (!DuplicateTokenEx(userToken.get(), MAXIMUM_ALLOWED, nullptr,
		SecurityIdentification, TokenPrimary, &dupToken))
		return false;
	UniqueHandle token(dupToken);

	std::wstring cmdLine;
	if (appPath.find(L' ') != std::wstring::npos)
		cmdLine = L"\"" + appPath + L"\"";
	else
		cmdLine = appPath;
	if (!args.empty())
		cmdLine += L" " + args;

	STARTUPINFOW si = { sizeof(si) };
	si.lpDesktop = const_cast<LPWSTR>(L"winsta0\\default");
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = static_cast<WORD>(showWnd);

	std::wstring effectiveWorkDir = workingDir;
	if (effectiveWorkDir.empty()) {
		size_t pos = appPath.find_last_of(L"\\/");
		if (pos != std::wstring::npos)
			effectiveWorkDir = appPath.substr(0, pos + 1);
	}

	PROCESS_INFORMATION pi = {};
	BOOL success = CreateProcessAsUserW(token.get(), nullptr, &cmdLine[0],
		nullptr, nullptr, FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,
		nullptr,
		effectiveWorkDir.empty() ? nullptr : effectiveWorkDir.c_str(),
		&si, &pi);
	if (success) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	else {
		OutputDebugStringW((L"Æô¶¯½ø³ÌÊ§°Ü£¬´íÎóÂë: " + std::to_wstring(GetLastError()) + L"\n").c_str());
	}
	return success == TRUE ? pi.dwProcessId : 0;
}