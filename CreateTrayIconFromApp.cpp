#include <windows.h>
#include <vector>
#include <string>

#define HOTKEY_ID 1
#define TRAY_ICON_MESSAGE (WM_USER + 1)

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
			L"SystemTray_Main" //Всплывающие окно трея у системных приложений
};

HBITMAP IconToBitmap(HICON hIcon) {
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

void UpdateTrayMenu() {
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
	AppendMenu(hMenu, MF_STRING, 9999, L"Выход");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_HOTKEY:
		if (wParam == HOTKEY_ID) {
			HWND activeWnd = GetForegroundWindow();
			if (activeWnd) {
				wchar_t className[256];
				GetClassName(activeWnd, className, sizeof(className) / sizeof(wchar_t));

				for (auto& t : wstr) {
					if (t == className)
						return 0;
				}

				wchar_t windowTitle[256];
				GetWindowText(activeWnd, windowTitle, sizeof(windowTitle) / sizeof(wchar_t));

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

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	WNDCLASS wc = {};
	{
		wc.lpfnWndProc = WndProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = L"TrayAppClass";
	}
	RegisterClass(&wc);
	HWND hwnd = CreateWindowEx(0, wc.lpszClassName, wc.lpszClassName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, hInstance, NULL);

	RegisterHotKey(hwnd, HOTKEY_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'T');
	NOTIFYICONDATA nid = {};
	{
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hwnd;
		nid.uID = 1;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.uCallbackMessage = TRAY_ICON_MESSAGE;
		nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
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

	return 0;
}
