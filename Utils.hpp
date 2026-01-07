#pragma once
#include <windows.h>
#include <string>

LPCWSTR RegisterNewClass(LPCWSTR className, WNDPROC wndproc);
bool isRunAsAdmin();
void DebugModCheck(wchar_t* lpCmdLine);
HBITMAP IconToBitmap(HICON hIcon);
void RestartWithAdminRights();