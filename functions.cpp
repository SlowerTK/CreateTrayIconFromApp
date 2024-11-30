#pragma once
#include "functions.hpp"
bool isDebugMode = false;
std::vector<HiddenWindow> hiddenWindows = {};
std::vector<FullscreenBorderlessWindow> fullscreenBorderlessWindows = {};
ProgramVariable pv = {0};
WNDCLASS RegisterNewClass(LPCWSTR className, WNDPROC wndproc) {
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpszClassName = className;
	wc.hInstance = pv.hInstance;
	wc.lpfnWndProc = wndproc;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	RegisterClass(&wc);
	return wc;
}
bool RunAsAdmin() {
	BOOL isAdmin = FALSE;
	PSID adminGroup = NULL;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
		if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
			isAdmin = FALSE;
		}
		FreeSid(adminGroup);
	}
	return isAdmin;
}
void DebugModCheck(WCHAR* lpCmdLine) {
	int argsCount;
	wchar_t** cmdArpvLine;
	if (cmdArpvLine = CommandLineToArgvW(lpCmdLine, &argsCount))
		for (int i = 0; i != argsCount; i++)
			if (!lstrcmpW(cmdArpvLine[i], L"debug")) {
				isDebugMode = true;
				break;
			}
}
void OpenSettings() {
	if (!pv.settWin)
		pv.settWin = CreateWindowExW(0, pv.wc2.lpszClassName, pv.wc2.lpszClassName, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX | WS_VISIBLE,
									 (GetSystemMetrics(SM_CXSCREEN) - CX) >> 1, (GetSystemMetrics(SM_CYSCREEN) - CY) >> 1, CX, CY, 0, 0, pv.hInstance, 0);
	else SetForegroundWindow(pv.settWin);
}
static HBITMAP IconToBitmap(HICON hIcon) {
	const int width = 16;
	const int height = 16;
	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
	{
		HBRUSH hBrush = CreateSolidBrush({0xf9f9f9});
		RECT rect = {0, 0, width, height};
		FillRect(hdcMem, &rect, hBrush);
		DeleteObject(hBrush);
	}
	DrawIconEx(hdcMem, 0, 0, hIcon, width, height, 0, NULL, DI_NORMAL);
	SelectObject(hdcMem, hOldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
	return hBitmap;
}
void UpdateTrayMenu() {
	if (pv.hMenu) {
		DestroyMenu(pv.hMenu);
	}
	pv.hMenu = CreatePopupMenu();
	for (UINT i = 0; i < hiddenWindows.size(); ++i) {
		UINT id = 1000 + i;
		AppendMenu(pv.hMenu, MF_STRING, id, hiddenWindows[i].windowTitle.c_str());
		HBITMAP hBitmap = IconToBitmap(hiddenWindows[i].hIcon);
		if (hBitmap) {
			SetMenuItemBitmaps(pv.hMenu, id, MF_BYCOMMAND, hBitmap, hBitmap);
		}
	}
	if (!hiddenWindows.empty()) {
		AppendMenu(pv.hMenu, MF_SEPARATOR, 0, NULL);
	}
	if (isDebugMode)
		AppendMenu(pv.hMenu, MF_STRING | MF_DISABLED, 0, L"Debug mode is enabled");
	else
		AppendMenu(pv.hMenu, MF_STRING | MF_DISABLED, 0, L"Ctrl + Alt + H");
	AppendMenu(pv.hMenu, MF_STRING, 1, L"Настройки");
	AppendMenu(pv.hMenu, MF_STRING, 9999, L"Выход");
}
void CloseApp() {
	for (const auto& hiddenWindow : hiddenWindows) {
		ShowWindow(hiddenWindow.hwnd, SW_SHOW);
	}
	hiddenWindows.clear();
	if (pv.settWin) {
		DestroyWindow(pv.settWin);
	}
	PostQuitMessage(0);
}
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	HWND hApplicationsList = (HWND)lParam;
	WCHAR windowTitle[256] = {0};
	DWORD processID;
	if (IsWindowVisible(hwnd) && GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(WCHAR)) > 0) {
		GetWindowThreadProcessId(hwnd, &processID);
		int count = (int)SendMessage(pv.hFavoritesList, LB_GETCOUNT, 0, 0);
		if (count)
			for (int i = 0; i < count; i++) {
				if (((HiddenWindow*)SendMessage(pv.hFavoritesList, LB_GETITEMDATA, i, 0))->processID == processID) {
					return true;
				}
			}
		HiddenWindow* HW = new HiddenWindow;
		HW->hwnd = hwnd;
		HW->windowTitle = windowTitle;
		HW->processID = processID;
		int index = (int)SendMessage(hApplicationsList, LB_ADDSTRING, 0, (LPARAM)windowTitle);
		SendMessage(hApplicationsList, LB_SETITEMDATA, index, (LPARAM)HW);
	}
	return true;
}

void UpdateApplicationsList() {
	if (!pv.hApplicationsList) {
		return;
	}
	int count = (int)SendMessage(pv.hApplicationsList, LB_GETCOUNT, 0, 0);
	if (count)
		for (int i = 0; i < count; i++) {
			HiddenWindow* HW = (HiddenWindow*)SendMessage(pv.hApplicationsList, LB_GETITEMDATA, i, 0);
			delete HW;
		}
	SendMessage(pv.hApplicationsList, LB_RESETCONTENT, 0, 0);
	EnumWindows(EnumWindowsProc, (LPARAM)pv.hApplicationsList);
}
void CollapseToTray(HWND activeWnd, std::wstring className) {
	wchar_t windowTitle[256];
	ZeroMemory(&windowTitle, (sizeof(windowTitle) / sizeof(windowTitle[0])));
	GetWindowText(activeWnd, windowTitle, (sizeof(windowTitle) / sizeof(windowTitle[0])));
	if (wcslen(windowTitle) == 0 || isDebugMode) {
		wcscpy_s(windowTitle, className.c_str());
	}

	HICON hIcon = (HICON)SendMessage(activeWnd, WM_GETICON, ICON_SMALL, 0);
	if (!hIcon) {
		hIcon = (HICON)GetClassLongPtr(activeWnd, GCLP_HICONSM);
	}
	if (!hIcon) {
		hIcon = LoadIcon(NULL, IDI_APPLICATION);
	}

	HiddenWindow hiddenWindow = {activeWnd, windowTitle, hIcon};
	hiddenWindows.push_back(hiddenWindow);
	ShowWindow(activeWnd, SW_HIDE);
	UpdateTrayMenu();
}
void CollapseToTrayFromFavorite() {
	int count = (int)SendMessage(pv.hFavoritesList, LB_GETCOUNT, 0, 0);
	if (count == LB_ERR)
		return;
	HiddenWindow* HW;
	for (int i = 0; i < count; i++) {
		HW = (HiddenWindow*)SendMessage(pv.hFavoritesList, LB_GETITEMDATA, i, 0);
		int j = 0;
		do {
			if (hiddenWindows.size())
				if (hiddenWindows[j++].processID == HW->processID) {
					break;
				}
			hiddenWindows.push_back(*HW);
			CollapseToTray(HW->hwnd, HW->windowTitle);
		} while (j != hiddenWindows.size());
	}
}
