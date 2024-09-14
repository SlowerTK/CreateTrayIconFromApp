#include <windows.h>
#include <vector>
#include <string>
#include <time.h>
#include "resource.h"

#define HOTKEY_ID 1
#define VERY_SECRET_BUTTON 10000
#define TRAY_ICON_MESSAGE (WM_USER + 1)
bool isDebugMode = false;
struct HiddenWindow {
	HWND hwnd;
	std::wstring windowTitle;
	HICON hIcon;
};

std::vector<HiddenWindow> hiddenWindows;

HMENU hMenu = NULL;

static std::wstring wstr[] = {
			L"Shell_TrayWnd", //Панель задач
			L"Progman", //Рабочий стол
			L"TaskManagerWindow", //Диспетчер задач
			L"TopLevelWindowForOverflowXamlIsland", //Скрытая панель трея
			L"WinUIDesktopWin32WindowClass", //Всплывающие окно трея у приложений использующих WinUI3
			L"SystemTray_Main", //Всплывающие окно трея у системных приложений
			L"Windows.UI.Core.CoreWindow",//Другие окна UI Windows
			L"WindowsDashboard", //Панель слева
			L"CTRIFATrayApp" //myself
};

bool RunAsAdmin();
void DebugModCheck(WCHAR* lpCmdLine);
static HBITMAP IconToBitmap(HICON hIcon);

static void UpdateTrayMenu() {
	if (hMenu) {
		DestroyMenu(hMenu);
	}
	hMenu = CreatePopupMenu();
	for (UINT i = 0; i < hiddenWindows.size(); ++i) {
		UINT id = 1000 + i;
		AppendMenu(hMenu, MF_STRING, id, hiddenWindows[i].windowTitle.c_str());
		HBITMAP hBitmap = IconToBitmap(hiddenWindows[i].hIcon);
		if (hBitmap) {
			SetMenuItemBitmaps(hMenu, id, MF_BYCOMMAND, hBitmap, hBitmap);
		}
	}
	if (!hiddenWindows.empty()) {
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	}
	if (isDebugMode)
		AppendMenu(hMenu, MF_STRING | MF_DISABLED, 0, L"Debug mode is enabled");
	else
		AppendMenu(hMenu, MF_STRING | MF_DISABLED, 0, L"Ctrl + Alt + H");
	AppendMenu(hMenu, MF_STRING, 9999, L"Выход");
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_HOTKEY:
		if (wParam == HOTKEY_ID) {
			HWND activeWnd = GetForegroundWindow();
			if (activeWnd) {
				wchar_t className[256];
				ZeroMemory(&className, (sizeof(className) / sizeof(className[0])));
				GetClassName(activeWnd, className, sizeof(className) / sizeof(className[0]));

				for (auto& t : wstr) {
					if (t == className)
						return 0;
				}

				wchar_t windowTitle[256];
				ZeroMemory(&windowTitle, (sizeof(windowTitle) / sizeof(windowTitle[0])));
				GetWindowText(activeWnd, windowTitle, (sizeof(windowTitle) / sizeof(windowTitle[0])));
				if (wcslen(windowTitle) == 0 || isDebugMode) {
					wcscpy_s(windowTitle, className);
				}

				HICON hIcon = (HICON)SendMessage(activeWnd, WM_GETICON, ICON_SMALL, 0);
				if (!hIcon) {
					hIcon = (HICON)GetClassLongPtr(activeWnd, GCLP_HICONSM);
				}
				if (!hIcon) {
					hIcon = LoadIcon(NULL, IDI_APPLICATION);
				}

				HiddenWindow hiddenWindow = { activeWnd, windowTitle, hIcon };
				hiddenWindows.push_back(hiddenWindow);
				ShowWindow(activeWnd, SW_HIDE);
				UpdateTrayMenu();
			}
		}
		break;

	case TRAY_ICON_MESSAGE:
		if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			SetForegroundWindow(hwnd);
			UpdateTrayMenu();
			int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, cursorPos.x, cursorPos.y, 0, hwnd, NULL);
			if (cmd >= 1000 && cmd < 1000 + hiddenWindows.size()) {
				int index = cmd - 1000;
				ShowWindow(hiddenWindows[index].hwnd, SW_SHOW);
				SetForegroundWindow(hiddenWindows[index].hwnd);
				hiddenWindows.erase(hiddenWindows.begin() + index);
			}
			else if (cmd == 9999) {
				for (const auto& hiddenWindow : hiddenWindows) {
					ShowWindow(hiddenWindow.hwnd, SW_SHOW);
				}
				hiddenWindows.clear();
				PostQuitMessage(0);
			}
		}
		break;

	case WM_DESTROY:
		for (const auto& hiddenWindow : hiddenWindows) {
			ShowWindow(hiddenWindow.hwnd, SW_SHOW);
		}
		hiddenWindows.clear();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ WCHAR* lpCmdLine, _In_ int nCmdShow) {
	HANDLE hMutex = CreateMutexW(0, TRUE, L"CTIFA");
	if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS)
		return 1;

	if (!RunAsAdmin())
		MessageBox(NULL, L"Для корректной работы программы рекамендуется запускать её с правами Администратора", L"Внимание", MB_ICONWARNING);

	DebugModCheck(lpCmdLine);

	WNDCLASS wc = {};
	{
		wc.lpfnWndProc = WndProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = L"CTRIFATrayApp";
	}
	RegisterClass(&wc);
	HWND hwnd = CreateWindowEx(0, wc.lpszClassName, wc.lpszClassName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, hInstance, NULL);

	RegisterHotKey(hwnd, HOTKEY_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'H');
	NOTIFYICONDATA nid = {};
	{
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hwnd;
		nid.uID = 1;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.uCallbackMessage = TRAY_ICON_MESSAGE;
		nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wcscpy_s(nid.szTip, L"Скрытые окна");
	}
	Shell_NotifyIcon(NIM_ADD, &nid);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &nid);
	UnregisterHotKey(hwnd, HOTKEY_ID);
	UnregisterClass(wc.lpszClassName, wc.hInstance);
	ReleaseMutex(hMutex);
	return 0;
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
	wchar_t** cmdArgvLine;
	if (cmdArgvLine = CommandLineToArgvW(lpCmdLine, &argsCount))
		for (int i = 0; i != argsCount; i++)
			if (!lstrcmpW(cmdArgvLine[i], L"debug")) {
				isDebugMode = true;
				break;
			}
}

static HBITMAP IconToBitmap(HICON hIcon) {
	const int width = 16;
	const int height = 16;
	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
	{
		HBRUSH hBrush = CreateSolidBrush({ 0xf9f9f9 });
		RECT rect = { 0, 0, width, height };
		FillRect(hdcMem, &rect, hBrush);
		DeleteObject(hBrush);
	}
	DrawIconEx(hdcMem, 0, 0, hIcon, width, height, 0, NULL, DI_NORMAL);
	SelectObject(hdcMem, hOldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
	return hBitmap;
}