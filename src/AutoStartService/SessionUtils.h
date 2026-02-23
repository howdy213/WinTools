#ifndef SESSIONUTILS_H
#define SESSIONUTILS_H

#include "common.h"
#include <string>

bool GetActiveSessionId(DWORD& sessionId);
bool GetUserTokenFromSessionId(DWORD sessionId, UniqueHandle& token);
DWORD StartProcessInUserSession(const std::wstring& appPath, const std::wstring& args,
    const std::wstring& workingDir, int showWnd = SW_NORMAL);

#endif