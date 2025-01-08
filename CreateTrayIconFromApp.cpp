#include "functions.hpp"

void AddTrayIcon(HWND hwnd){
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = TRAY_ICON_MESSAGE;
	nid.hIcon = LoadIcon(pv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcscpy_s(nid.szTip, SZ_TIP);
	Shell_NotifyIconW(NIM_ADD, &nid);
}
void RemoveTrayIcon(HWND hwnd) {
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;
	Shell_NotifyIconW(NIM_DELETE, &nid);
}


static LRESULT CALLBACK SettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		pv.settWin = hwnd;
		CreateWindow(STATIC, WND_NAME_TEXT, WS_VISIBLE | WS_CHILD, 10, 10, 200, 20, hwnd, NULL, pv.hInstance, NULL);
		pv.hApplicationsList = CreateWindow(LISTBOX, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 10, 30, 200, 300, hwnd, (HMENU)ID_LIST_APPLICATIONS, pv.hInstance, NULL);
		pv.hText = CreateWindow(STATIC, WND_NAME_TEXT2, WS_VISIBLE | WS_CHILD, 380, 10, 300, 20, hwnd, NULL, pv.hInstance, NULL);
		pv.hFavoritesList = CreateWindow(LISTBOX, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 380, 30, 200, 300, hwnd, (HMENU)ID_LIST_FAVORITES, pv.hInstance, NULL);
		pv.hAddButton = CreateWindow(BUTTON, ADDBUTTON_TEXT, WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, 300, 100, 40, 30, hwnd, (HMENU)ID_BUTTON_ADD, pv.hInstance, NULL);
		pv.hRemoveButton = CreateWindow(BUTTON, REMOVEBUTTON_TEXT, WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, 300, 140, 40, 30, hwnd, (HMENU)ID_BUTTON_REMOVE, pv.hInstance, NULL);
		pv.hReloadButton = CreateWindow(BUTTON, RELOADBUTTON_TEXT, WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, 300, 30, 40, 30, hwnd, (HMENU)ID_BUTTON_RELOAD, pv.hInstance, NULL);
		UpdateApplicationsList();
		UpdateFavoriteList();
		break;
	case WM_SIZE: {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		MoveWindow(pv.hApplicationsList, 10, 30, width / 2 - 40, height - 30, TRUE);
		MoveWindow(pv.hText, width / 2 + 30, 10, 300, 20, TRUE);
		MoveWindow(pv.hFavoritesList, width / 2 + 30, 30, width / 2 - 40, height - 30, TRUE);
		MoveWindow(pv.hAddButton, width / 2 - 20, height / 2 - 40, 40, 30, TRUE);
		MoveWindow(pv.hRemoveButton, width / 2 - 20, height / 2, 40, 30, TRUE);
		MoveWindow(pv.hReloadButton, width / 2 - 20, 30, 40, 30, TRUE);
		InvalidateRect(pv.hApplicationsList, NULL, TRUE);
		InvalidateRect(pv.hText, NULL, TRUE);
		InvalidateRect(pv.hFavoritesList, NULL, TRUE);
		InvalidateRect(pv.hAddButton, NULL, TRUE);
		InvalidateRect(pv.hRemoveButton, NULL, TRUE);
		InvalidateRect(pv.hReloadButton, NULL, TRUE);
		break;
	}
	case WM_GETMINMAXINFO: {
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = wndX;
		mmi->ptMinTrackSize.y = wndY;
		break;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_BUTTON_ADD) {
			LRESULT sel = SendMessage(pv.hApplicationsList, LB_GETCURSEL, NULL, NULL);
			if (sel != LB_ERR) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(pv.hApplicationsList, LB_GETITEMDATA, sel, NULL);
				HW->isFavorite = TRUE;
				favoriteWindows.push_back(*HW);
				int i = (int)SendMessage(pv.hFavoritesList, LB_ADDSTRING, NULL, (LPARAM)HW->windowTitle.c_str());
				SendMessage(pv.hFavoritesList, LB_SETITEMDATA, i, (LPARAM)HW);
				SendMessage(pv.hApplicationsList, LB_DELETESTRING, sel, NULL);
			}
		} else if (LOWORD(wParam) == ID_BUTTON_REMOVE) {
			LRESULT sel = SendMessage(pv.hFavoritesList, LB_GETCURSEL, NULL, NULL);
			if (sel != LB_ERR) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(pv.hFavoritesList, LB_GETITEMDATA, sel, NULL);
				auto eraceWindow = [HW](std::vector<HiddenWindow>& vec) {
					auto it = std::find_if(vec.begin(), vec.end(),
										   [&](const HiddenWindow& wnd) { return wnd.hwnd == HW->hwnd; });
					if (it != vec.end()) vec.erase(it);	};
				eraceWindow(favoriteWindows);
				eraceWindow(hiddenWindows);
				SendMessage(pv.hFavoritesList, LB_DELETESTRING, sel, NULL);
				ShowWindow(HW->hwnd, SW_SHOW);
				delete HW;
				UpdateApplicationsList();
			}
		} else if (LOWORD(wParam) == ID_BUTTON_RELOAD) {
			UpdateApplicationsList();
		}
		break;
	case WM_CLOSE:
		CollapseToTrayFromFavorite();
		{
			CheckFolderAndFile();
			std::wstring favorite{};
			for (const auto& hw : hiddenWindows) {
				auto it = std::find_if(favoriteWindows.begin(), favoriteWindows.end(), [hw](HiddenWindow HW) {return hw.isFavorite == TRUE && hw.processID == HW.processID; });
				if (it == favoriteWindows.end())
					favoriteWindows.push_back(*it);
			}
			for (const auto& vec : favoriteWindows) {
				if(vec.isFavorite)
					favorite += serializeToWstring(vec);
			}
			WriteSettingsFile(favorite);
		}
		DeleteList(pv.hApplicationsList);
		DeleteList(pv.hFavoritesList);
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
	case WM_CREATE:
		SetTimer(hwnd, TIMER_ID, 10000, NULL);
		break;
	case WM_TIMER:
		for (auto& fw : favoriteWindows) {
			FindWindowFromFile(fw, false);
		}
		break;
	case WM_HOTKEY: {
		HWND activeWnd = GetForegroundWindow();
		if (!activeWnd)	return 0;
		if(wParam == HK_CTIFA_ID) CollapseToTray(activeWnd);
		break;
	}
	case TRAY_ICON_MESSAGE:
		switch (lParam) {
		case WM_RBUTTONUP: {
			bool isDebug = false;
			if (GetKeyState(VK_SHIFT) & HSHELL_HIGHBIT) isDebug = true;
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			SetForegroundWindow(hwnd);
			UpdateTrayMenu(isDebug);
			int cmd = TrackPopupMenu(pv.hMenu, TPM_RETURNCMD | TPM_NONOTIFY, cursorPos.x, cursorPos.y, NULL, hwnd, NULL);
			if (cmd >= 1000 && cmd < 1000 + hiddenWindows.size()) {
				int index = cmd - 1000;
				ShowWindow(hiddenWindows[index].hwnd, SW_SHOW);
				SetForegroundWindow(hiddenWindows[index].hwnd);
				auto it = std::find_if(favoriteWindows.begin(), favoriteWindows.end(),
									   [&](const HiddenWindow& wnd) { return wnd.hwnd == hiddenWindows[index].hwnd; });
				if (it != favoriteWindows.end()) {
					it->isFavorite = SAVED_WINDOW;
				}
				hiddenWindows.erase(hiddenWindows.begin() + index);
			} else if (cmd == TB_EXIT)
				CloseApp();
			else if (cmd == TB_SETTINGS)
				OpenSettings();
			PostMessageW(hwnd, WM_NULL, NULL, NULL);
		}
		break;
		case WM_LBUTTONUP:
			OpenSettings();
			break;
		default:
			break;
		}
		break;
	case WM_DESTROY:
		KillTimer(hwnd, TIMER_ID);
		CloseApp();
		break;
	default:
		if (uMsg == pv.WM_TASKBAR_CREATED)
			AddTrayIcon(hwnd);
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ WCHAR* lpCmdLine, _In_ int nCmdShow) {
	HANDLE hMutex = CreateMutexW(0, TRUE, L"CTIFA");
	if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS)
		return 1;
	pv.hInstance = hInstance;
	if (!isRunAsAdmin())
		MBATTENTION(WT_ADMIN);

	DebugModCheck(lpCmdLine);
	pv.WM_TASKBAR_CREATED = RegisterWindowMessageW(L"TaskbarCreated");

	pv.wc1 = RegisterNewClass(L"CTRIFATrayApp", TrayProc);
	pv.wc2 = RegisterNewClass(L"CTIFA Settings", SettingsProc);
	HWND hwnd = CreateWindowExW(0, pv.wc1.lpszClassName, pv.wc1.lpszClassName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, pv.hInstance, NULL);

	bool isCanCTIFA = RegisterHotKey(hwnd, HK_CTIFA_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'H');
	if (!isCanCTIFA) {
		MBERROR(ET_HOTKEY);
		UnregisterHotKey(hwnd, HK_CTIFA_ID);
		ReleaseMutex(hMutex);
		return -1;
	}
	AddTrayIcon(hwnd);
	{
		CheckFolderAndFile();
		std::wstring favorites = ReadSettingsFile();
		if (!favorites.empty())
			deserializeFromWstring(favorites);
		
	}
	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	RemoveTrayIcon(hwnd);
	UnregisterHotKey(hwnd, HK_CTIFA_ID);
	UnregisterClass(pv.wc1.lpszClassName, pv.wc1.hInstance);
	UnregisterClass(pv.wc2.lpszClassName, pv.wc2.hInstance);
	ReleaseMutex(hMutex);
	return 0;
}