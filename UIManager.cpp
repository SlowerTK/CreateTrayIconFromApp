#include "UIManager.hpp"
#include "AppConstants.hpp"
#include "Utils.hpp"
#include "WindowManager.hpp"
#include "ProcessManager.hpp"
#include "Logger.hpp"
#include "resource.h"

void OpenSettings() {
	if (!pv.settWin)
		pv.settWin = CreateWindowExW(NULL, pv.wc2, pv.isAdminMode ? pv.wc2 : TX_UI_TITLE, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX | WS_VISIBLE,
									 (GetSystemMetrics(SM_CXSCREEN) - CONST_WND_WIDTH) >> 1, (GetSystemMetrics(SM_CYSCREEN) - CONST_WND_HEIGHT) >> 1, CONST_WND_WIDTH, CONST_WND_HEIGHT, 0, 0, pv.hInstance, 0);
	else SetForegroundWindow(pv.settWin);
}
void UpdateTrayMenu(bool isDebug) {
	if (pv.hMenu) DestroyMenu(pv.hMenu);
	pv.hMenu = CreatePopupMenu();
	for (auto it = hiddenWindows.begin(); it != hiddenWindows.end();) {
		if (!GetWindowThreadProcessId(it->hwnd, NULL)) {
			it = hiddenWindows.erase(it);
			continue;
		}
		UINT id = static_cast<UINT>(1000) + static_cast<UINT>(std::distance(hiddenWindows.begin(), it));
		std::wstring menuText = (pv.isDebugMode || isDebug) ? it->processName.c_str() : TruncateWithEllipsis(it->windowTitle);
		AppendMenuW(pv.hMenu, MF_STRING, id, menuText.c_str());
		HBITMAP hBitmap = IconToBitmap(it->hIcon);
		if (hBitmap) SetMenuItemBitmaps(pv.hMenu, id, MF_BYCOMMAND, hBitmap, hBitmap);
		++it;
	}
	if (!hiddenWindows.empty()) AppendMenuW(pv.hMenu, MF_SEPARATOR, NULL, NULL);
	if (pv.isAdminMode) AppendMenuW(pv.hMenu, MF_STRING | MF_DISABLED, ID_TRAY_RESTART, pv.hk.nameArr.c_str());
	else AppendMenuW(pv.hMenu, MF_STRING, ID_TRAY_RESTART, TX_ADMIN_RESTART);
	AppendMenuW(pv.hMenu, MF_STRING, ID_TRAY_SETTINGS, TX_TRAY_BTN_SETTINGS);
	AppendMenuW(pv.hMenu, MF_STRING, ID_TRAY_EXIT, TX_TRAY_BTN_EXIT);
}
static BOOL CALLBACK EnumWindowsUpdateAppListProc(HWND hwnd, LPARAM lParam) {
	HWND hApplicationsList = reinterpret_cast<HWND>(lParam);
	wchar_t windowTitle[MAX_PATH] = { 0 };

	if (IsWindowVisible(hwnd) && GetWindowTextW(hwnd, windowTitle, SIZEOF(windowTitle)) > 0) {
		std::wstring className = GetWstringClassName(hwnd);
		if (className.empty())
			return true;
		DWORD processID = GetProcessId(hwnd);
		std::wstring processName = GetAllowedProcessName(processID);
		if (processName.empty())
			return true;

		int count = static_cast<int>(SendMessageW(pv.hFavoritesList, LB_GETCOUNT, NULL, NULL));
		for (int i = 0; i < count; i++) {
			HiddenWindow* hw = reinterpret_cast<HiddenWindow*>(SendMessageW(pv.hFavoritesList, LB_GETITEMDATA, i, NULL));
			if (hw->processName == processName && hw->hwnd == hwnd)
				return true;
		}

		HiddenWindow* HW = new HiddenWindow;
		HW->hwnd = hwnd;
		HW->windowTitle = windowTitle;
		HW->className = className;
		HW->processID = processID;
		HW->processName = processName;
		HW->isFavorite = false;
		int index = static_cast<int>(SendMessageW(hApplicationsList, LB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(windowTitle)));
		SendMessageW(hApplicationsList, LB_SETITEMDATA, index, reinterpret_cast<LPARAM>(HW));
	}
	return true;
}
void UpdateApplicationsList() {
	if (!pv.hApplicationsList)
		return;
	DeleteList(pv.hApplicationsList);
	EnumWindows(EnumWindowsUpdateAppListProc, reinterpret_cast<LPARAM>(pv.hApplicationsList));

	SendMessageW(pv.hApplicationsList, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(GetMaxTextWidth(pv.hApplicationsList)) + 10, 0);
}
void UpdateFavoriteList() {
	if (!pv.hFavoritesList)
		return;
	DeleteList(pv.hFavoritesList);
	for (const auto& hw : favoriteWindows) {
		HiddenWindow* HW = new HiddenWindow;
		*HW = hw;
		int index = static_cast<int>(SendMessageW(pv.hFavoritesList, LB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(hw.windowTitle.c_str())));
		SendMessageW(pv.hFavoritesList, LB_SETITEMDATA, index, reinterpret_cast<LPARAM>(HW));
	}
	SendMessageW(pv.hFavoritesList, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(GetMaxTextWidth(pv.hFavoritesList)) + 10, 0);
}
void DeleteList(HWND list) {
	if (list) {
		int count = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
		for (int i = 0; i < count; ++i) {
			HiddenWindow* HW = reinterpret_cast<HiddenWindow*>(SendMessageW(list, LB_GETITEMDATA, i, 0));
			delete HW;
		}
		SendMessageW(list, LB_RESETCONTENT, 0, 0);
	}
}
int GetMaxTextWidth(HWND hwndListBox) {
	HDC hdc = GetDC(hwndListBox);
	HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, pv.hFont));
	int count = static_cast<int>(SendMessageW(hwndListBox, LB_GETCOUNT, 0, 0));
	SIZE sz = { 0 };
	int maxWidth = 0;
	for (int i = 0; i < count; ++i) {
		wchar_t buffer[1024]{};
		SendMessageW(hwndListBox, LB_GETTEXT, i, reinterpret_cast<LPARAM>(buffer));
		GetTextExtentPoint32W(hdc, buffer, lstrlenW(buffer), &sz);
		if (sz.cx > maxWidth)
			maxWidth = sz.cx;
	}
	SelectObject(hdc, hOldFont);
	ReleaseDC(hwndListBox, hdc);
	return maxWidth;
}
void AddTrayIcon(HWND hwnd) {
	static bool count = 0;
	while (!FindWindowW(L"Shell_TrayWnd", NULL)) {
		Sleep(100);
	}
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = TRAY_ICON_MESSAGE;
	nid.hIcon = LoadIconW(pv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	if (!nid.hIcon) LogAdd(ET_ICON_LOAD);
	wcscpy_s(nid.szTip, TX_TRAY_TOOLTIP);
	if (Shell_NotifyIconW(NIM_ADD, &nid)) {
		LogAdd(IT_ICON_CREATED);
	}
	else if (!count) {
		LogAdd(IT_TRAY_WAIT_RETRY);
		count = 1;
	}
	else {
		DWORD err = GetLastError();
		wchar_t buf[64];
		swprintf_s(buf, ET_ICON_CREATE, static_cast<UINT>(err));
		LogAdd(buf);
	}
}
void RemoveTrayIcon(HWND hwnd) {
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;
	if (Shell_NotifyIconW(NIM_DELETE, &nid)) {
		LogAdd(IT_ICON_DELETED);
	}
	else {
		DWORD err = GetLastError();
		wchar_t buf[64];
		swprintf_s(buf, ET_ICON_DELETE, static_cast<UINT>(err));
		LogAdd(buf);
	}
}
void CreateHKSettWnd() {
	if (!pv.settHK) {
		RECT rect;
		GetWindowRect(pv.settWin, &rect);
		int cx = 600;
		int cy = 300;
		int x = rect.left + ((rect.right - rect.left) >> 1) - (cx >> 1);
		int y = rect.top + ((rect.bottom - rect.top) >> 1) - (cy >> 1);
		pv.settHK = CreateWindowExW(NULL, pv.wc3, TX_PRESS_HOTKEY, WS_SYSMENU | WS_POPUP | WS_VISIBLE, x, y, cx, cy, pv.settWin, NULL, pv.hInstance, NULL);
		SetForegroundWindow(pv.settHK);
	}
}
std::wstring TruncateWithEllipsis(const std::wstring& text, size_t maxLen) {
	if (text.size() <= maxLen)
		return text;
	return text.substr(0, maxLen - 3) + L"...";
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