#include "functions.hpp"

void ScaleToFS(HWND activeWnd) {

	auto it = std::find_if(fullscreenBorderlessWindows.begin(), fullscreenBorderlessWindows.end(),
						   [&](const FullscreenBorderlessWindow& wnd) { return wnd.hwnd == activeWnd; });
	if (it != fullscreenBorderlessWindows.end()) {
		SetWindowLong(it->hwnd, GWL_STYLE, it->dwStyle);
		SetWindowLong(it->hwnd, GWL_EXSTYLE, it->dwExStyle);
		SetWindowPos(it->hwnd, NULL, POINT2CORD(it->rect), SWP_FRAMECHANGED | SWP_NOZORDER);
		fullscreenBorderlessWindows.erase(it);
		return;
	}

	DWORD dwStyle = GetWindowLong(activeWnd, GWL_STYLE);
	DWORD dwExStyle = GetWindowLong(activeWnd, GWL_EXSTYLE);
	RECT rect;
	GetWindowRect(activeWnd, &rect);
	POINT pt;
	GetCursorPos(&pt);
	HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi{sizeof(mi)};
	if (!GetMonitorInfo(hMonitor, &mi)) {
		MessageBox(NULL, L"Не удалось определить монитор", L"Ошибка", MB_ICONERROR);
		return;
	}
	SetWindowLong(activeWnd, GWL_STYLE, dwStyle &
				  ~(WS_CAPTION | WS_THICKFRAME)
	);
	SetWindowLong(activeWnd, GWL_EXSTYLE, dwExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
	SetWindowPos(activeWnd, HWND_TOP, POINT2CORD(mi.rcMonitor), SWP_FRAMECHANGED | SWP_NOZORDER);
	SendMessage(activeWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
	fullscreenBorderlessWindows.push_back({activeWnd, dwStyle, dwExStyle, rect});
}

static LRESULT CALLBACK SettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		pv.settWin = hwnd;
		SetTimer(pv.settWin, TID_UPDATE, 5000, NULL);
		CreateWindow(L"STATIC", L"Applications:", WS_VISIBLE | WS_CHILD, 10, 10, 200, 20, hwnd, NULL, pv.hInstance, NULL);
		pv.hApplicationsList = CreateWindow(L"LISTBOX", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 10, 30, 200, 300, hwnd, (HMENU)ID_LIST_APPLICATIONS, pv.hInstance, NULL);
		pv.hText = CreateWindow(L"STATIC", L"Favorites (automatic):", WS_VISIBLE | WS_CHILD, 380, 10, 200, 20, hwnd, NULL, pv.hInstance, NULL);
		pv.hFavoritesList = CreateWindow(L"LISTBOX", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 380, 30, 200, 300, hwnd, (HMENU)ID_LIST_FAVORITES, pv.hInstance, NULL);
		pv.hAddButton = CreateWindow(L"BUTTON", L"▶", WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, 300, 100, 40, 30, hwnd, (HMENU)ID_BUTTON_ADD, pv.hInstance, NULL);
		pv.hRemoveButton = CreateWindow(L"BUTTON", L"◀", WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, 300, 140, 40, 30, hwnd, (HMENU)ID_BUTTON_REMOVE, pv.hInstance, NULL);
		UpdateApplicationsList();
		break;
	case WM_SIZE: {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		MoveWindow(pv.hApplicationsList, 10, 30, width / 2 - 40, height - 30, TRUE);
		MoveWindow(pv.hText, width / 2 + 30, 10, 200, 20, TRUE);
		MoveWindow(pv.hFavoritesList, width / 2 + 30, 30, width / 2 - 40, height - 30, TRUE);
		MoveWindow(pv.hAddButton, width / 2 - 20, height / 2 - 40, 40, 30, TRUE);
		MoveWindow(pv.hRemoveButton, width / 2 - 20, height / 2, 40, 30, TRUE);
		InvalidateRect(pv.hApplicationsList, NULL, TRUE);
		InvalidateRect(pv.hText, NULL, TRUE);
		InvalidateRect(pv.hFavoritesList, NULL, TRUE);
		InvalidateRect(pv.hAddButton, NULL, TRUE);
		InvalidateRect(pv.hRemoveButton, NULL, TRUE);
		break;
	}
	case WM_GETMINMAXINFO: {
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = 600;
		mmi->ptMinTrackSize.y = 375;
		break;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_BUTTON_ADD) {
			LRESULT sel = SendMessage(pv.hApplicationsList, LB_GETCURSEL, 0, 0);
			if (sel != LB_ERR) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(pv.hApplicationsList, LB_GETITEMDATA, sel, 0);
				int i = (int)SendMessage(pv.hFavoritesList, LB_ADDSTRING, 0, (LPARAM)HW->windowTitle.c_str());
				SendMessage(pv.hFavoritesList, LB_SETITEMDATA, i, (LPARAM)HW);
				SendMessage(pv.hApplicationsList, LB_DELETESTRING, sel, 0);
			}
		} else if (LOWORD(wParam) == ID_BUTTON_REMOVE) {
			LRESULT sel = SendMessage(pv.hFavoritesList, LB_GETCURSEL, 0, 0);
			if (sel != LB_ERR) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(pv.hFavoritesList, LB_GETITEMDATA, sel, 0);
				//ФАЙЛ ГДЕ?!!??!?!?!?!?!?!?!?
				SendMessage(pv.hFavoritesList, LB_DELETESTRING, sel, 0);
				UpdateApplicationsList();
				delete HW;
			}
		}
		break;
	case WM_TIMER:
		UpdateApplicationsList();
		CollapseToTrayFromFavorite();
		break;
	case WM_CLOSE:
		KillTimer(pv.settWin, TID_UPDATE);
		pv.settWin = NULL;
		DestroyWindow(hwnd);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}
static LRESULT CALLBACK TrayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_HOTKEY: {
		HWND activeWnd = GetForegroundWindow();
		if (!activeWnd)
			return 0;
		std::wstring className;
		{
			wchar_t classNam[256];
			ZeroMemory(&classNam, (sizeof(classNam) / sizeof(classNam[0])));
			GetClassName(activeWnd, classNam, sizeof(classNam) / sizeof(classNam[0]));
			className = classNam;
		}
		for (const auto& t : wstr) {
			if (t == className)
				return 0;
		}
		switch (wParam) {
		case HK_CTIFA_ID:
			CollapseToTray(activeWnd, className);
			break;
		case HK_FBL_ID:
			ScaleToFS(activeWnd);
			break;
		default:
			break;
		}
		break;
	}
	case TRAY_ICON_MESSAGE:
		switch (lParam) {
		case WM_RBUTTONUP: {
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			SetForegroundWindow(hwnd);
			UpdateTrayMenu();
			int cmd = TrackPopupMenu(pv.hMenu, TPM_RETURNCMD | TPM_NONOTIFY, cursorPos.x, cursorPos.y, 0, hwnd, NULL);
			if (cmd >= 1000 && cmd < 1000 + hiddenWindows.size()) {
				int index = cmd - 1000;
				ShowWindow(hiddenWindows[index].hwnd, SW_SHOW);
				SetForegroundWindow(hiddenWindows[index].hwnd);
				hiddenWindows.erase(hiddenWindows.begin() + index);
			} else if (cmd == 9999) {
				CloseApp();
			} else if (cmd == 1) {
				OpenSettings();
			}
			break;
		}
		case WM_LBUTTONUP:
			OpenSettings();
			break;
		default:
			break;
		}
		break;
	case WM_DESTROY:
		CloseApp();
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
	pv.hInstance = hInstance;
	if (!RunAsAdmin())
		MessageBox(NULL, L"Для корректной работы программы рекомендуется запускать её с правами Администратора", L"Внимание", MB_ICONWARNING);

	DebugModCheck(lpCmdLine);

	pv.wc1 = RegisterNewClass(L"CTRIFATrayApp", TrayProc);
	pv.wc2 = RegisterNewClass(L"CTIFA Settings", SettingsProc);
	HWND hwnd = CreateWindowExW(0, pv.wc1.lpszClassName, pv.wc1.lpszClassName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, pv.hInstance, NULL);

	bool isCanCTIFA = RegisterHotKey(hwnd, HK_CTIFA_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'H');
	bool isCanFBL = RegisterHotKey(hwnd, HK_FBL_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_F6);
	if (!isCanCTIFA || !isCanFBL) {
		MessageBox(NULL, L"Не удалось зарегистрировать горячие клавиши", L"Ошибка", MB_ICONERROR);
		UnregisterHotKey(hwnd, HK_CTIFA_ID);
		UnregisterHotKey(hwnd, HK_FBL_ID);
		ReleaseMutex(hMutex);
		return -1;
	}
	NOTIFYICONDATA nid = {};
	{
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hwnd;
		nid.uID = 1;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.uCallbackMessage = TRAY_ICON_MESSAGE;
		nid.hIcon = LoadIcon(pv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wcscpy_s(nid.szTip, L"Скрытые окна");
	}
	Shell_NotifyIcon(NIM_ADD, &nid);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &nid);
	UnregisterHotKey(hwnd, HK_CTIFA_ID);
	UnregisterHotKey(hwnd, HK_FBL_ID);
	UnregisterClass(pv.wc1.lpszClassName, pv.wc1.hInstance);
	UnregisterClass(pv.wc2.lpszClassName, pv.wc2.hInstance);
	ReleaseMutex(hMutex);
	return 0;
}