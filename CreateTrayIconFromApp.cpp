#include <windows.h>
#include <vector>
#include <string>

#define HOTKEY_ID 1
#define HOTKEY_ID1 2
#define TRAY_ICON_MESSAGE (WM_USER + 1)

struct TrayIcon {
	HWND hwnd;
	NOTIFYICONDATA nid;
};

std::vector<TrayIcon> trayIcons;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_HOTKEY:
		if (wParam == HOTKEY_ID) {
			HWND activeWnd = GetForegroundWindow();
			if (activeWnd) {
				TrayIcon trayIcon = {};
				HICON hIcon = (HICON)SendMessage(activeWnd, WM_GETICON, ICON_SMALL, 0);
				{
					if (!hIcon) {
						hIcon = (HICON)GetClassLongPtr(activeWnd, GCLP_HICONSM);
					}
					if (!hIcon) {
						hIcon = LoadIcon(NULL, IDI_APPLICATION); // fallback если иконка не найдена
					}
					trayIcon.hwnd = activeWnd;
					trayIcon.nid.cbSize = sizeof(NOTIFYICONDATA);
					trayIcon.nid.hWnd = hwnd;
					trayIcon.nid.uID = static_cast<UINT>(trayIcons.size() + 1);
					trayIcon.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
					trayIcon.nid.uCallbackMessage = TRAY_ICON_MESSAGE;
					trayIcon.nid.hIcon = hIcon;
				}
				GetWindowText(activeWnd, trayIcon.nid.szTip, sizeof(trayIcon.nid.szTip) / sizeof(TCHAR));
				Shell_NotifyIcon(NIM_ADD, &trayIcon.nid);
				trayIcons.push_back(trayIcon);
				ShowWindow(activeWnd, SW_HIDE);
			}
		}
		if (wParam == HOTKEY_ID1) {
			for (auto& icon : trayIcons) {
				Shell_NotifyIcon(NIM_DELETE, &icon.nid);
			}
			trayIcons.clear();
			PostQuitMessage(0);
		}
		break;

	case TRAY_ICON_MESSAGE:
		if (lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP) {
			auto it = trayIcons.end();
			for (auto iter = trayIcons.begin(); iter != trayIcons.end(); ++iter) {
				if (iter->nid.uID == wParam) {
					it = iter;
					break;
				}
			}

			if (it != trayIcons.end()) {
				ShowWindow(it->hwnd, SW_SHOW);
				SetForegroundWindow(it->hwnd);
				Shell_NotifyIcon(NIM_DELETE, &it->nid);
				trayIcons.erase(it);
			}
		}
		break;

	case WM_DESTROY:
		for (auto& icon : trayIcons) {
			Shell_NotifyIcon(NIM_DELETE, &icon.nid);
		}
		trayIcons.clear();
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	WNDCLASS wc = {};
	{
		wc.lpfnWndProc = WndProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = L"TrayAppClass";
	}
	RegisterClass(&wc);
	HWND hwnd = CreateWindowEx(0, wc.lpszClassName, L"TrayApp", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	RegisterHotKey(hwnd, HOTKEY_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'T');
	RegisterHotKey(hwnd, HOTKEY_ID1, MOD_CONTROL | MOD_SHIFT | MOD_ALT | MOD_NOREPEAT, 'T');

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UnregisterHotKey(hwnd, HOTKEY_ID);
	UnregisterHotKey(hwnd, HOTKEY_ID1);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}