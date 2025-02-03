#include "functions.hpp"

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
	nid.hIcon = LoadIcon(pv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	if (!nid.hIcon) LogAdd(L"Ошибка загрузки иконки");
	wcscpy_s(nid.szTip, SZ_TIP);
	if (Shell_NotifyIconW(NIM_ADD, &nid)) {
		LogAdd(L"Иконка создана");
	} else if (!count) {
		LogAdd(L"Первая попытка добавить иконку не удалась, жду TaskbarCreated...");
		count = 1;
	} else {
		DWORD err = GetLastError();
		wchar_t buf[64];
		swprintf_s(buf, L"Ошибка создания иконки: 0x%08X", static_cast<UINT>(err));
		LogAdd(buf);
	}
}
void RemoveTrayIcon(HWND hwnd) {
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;
	if (Shell_NotifyIconW(NIM_DELETE, &nid)) {
		LogAdd(L"Иконка успешно удалена");
	} else {
		DWORD err = GetLastError();
		wchar_t buf[64];
		swprintf_s(buf, L"Ошибка удаления иконки: 0x%08X", static_cast<UINT>(err));
		LogAdd(buf);
	}
}
void CALLBACK TIMER_PROC(HWND hwnd, UINT uint, UINT_PTR uintptr, DWORD dword) {
	auto it = std::find_if(favoriteWindows.begin(), favoriteWindows.end(),
						   [&](const HiddenWindow& wnd) { return wnd.isFavorite == TIMED_WINDOW; });
	if (it != favoriteWindows.end()) {
		it->isFavorite = TRUE;
		if (it->hwnd == FindWindow(it->className.c_str(), it->windowTitle.c_str())) {
			CollapseToTray(it->hwnd, &*it);
		}
	}
	KillTimer(hwnd, uintptr);
}

static LRESULT CALLBACK TimerSettProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HFONT hFont = NULL;
	static HBRUSH hBrush = NULL;
	static HBRUSH hBrushEdit = NULL;
	static HWND parent = NULL;
	switch (uMsg) {
	case WM_CREATE: {
		parent = (HWND)GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT);
		hFont = CreateFontW(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Ariel");
		hBrush = CreateSolidBrush(RGB(189, 189, 189));
		hBrushEdit = CreateSolidBrush(RGB(255, 255, 255));
		std::wstring timerStr = std::to_wstring(pv.timerToHide);
		CreateWindowExW(0, STATIC, L"Через сколько восстановленное окно должно быть скрыто", WS_VISIBLE | WS_CHILD, 10, 10, 700, 30, hwnd, NULL, pv.hInstance, NULL);
		CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", timerStr.c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_LEFT, 10, 40, 700, 30, hwnd, (HMENU)ID_EDIT_FIELD, pv.hInstance, NULL);
		HWND hButton = CreateWindowExW(0, BUTTON, L"ОК", WS_VISIBLE | WS_CHILD | BS_CENTER | BS_FLAT | BS_VCENTER, 650, 200, 50, 30, hwnd, (HMENU)ID_OK_BUTTON, pv.hInstance, NULL);
		SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
		return 0;
	}
	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wParam;
		SetBkColor(hdcStatic, RGB(189, 189, 189));
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SelectObject(hdcStatic, hFont);
		return (INT_PTR)hBrush;
	}
	case WM_CTLCOLOREDIT: {
		HDC hdcEdit = (HDC)wParam;
		SetBkColor(hdcEdit, RGB(255, 255, 255));
		SetTextColor(hdcEdit, RGB(0, 0, 0));
		SelectObject(hdcEdit, hFont);
		return (INT_PTR)hBrushEdit;
	}
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_UPDATE && LOWORD(wParam) == ID_EDIT_FIELD) {
			HWND hEdit = (HWND)lParam;
			wchar_t buffer[16] = {0};
			GetWindowTextW(hEdit, buffer, 16);
			unsigned int value = wcstoul(buffer, NULL, 10);
			unsigned int prevValue = (unsigned int)GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if (value < 1 || value > INT_MAX) {
				SetWindowTextW(hEdit, std::to_wstring(prevValue).c_str());
				SendMessageW(hEdit, EM_SETSEL, 0, -1);
			} else {
				SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)value);
				pv.timerToHide = value;
			}
		} else if (LOWORD(wParam) == ID_OK_BUTTON) {
			DestroyWindow(hwnd);
			pv.settTimer = NULL;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		pv.settTimer = NULL;
		break;
	case WM_DESTROY:
		DeleteObject(hBrush);
		DeleteObject(hBrushEdit);
		DeleteObject(hFont);
		EnableWindow(parent, TRUE);
		SetForegroundWindow(parent);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}


static LRESULT CALLBACK SettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static unsigned int prevValue;
	static HFONT hFont = NULL;
	switch (uMsg) {
	case WM_CREATE: {
		LogAdd(L"Открыты настройки");
		prevValue = pv.timerToHide;
		hFont = CreateFontW(22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Ariel");
		pv.hSettMenu = CreateMenu();
		pv.hSettSubMenu = CreatePopupMenu();
		std::vector<LPCWSTR> subMenuItems = {(IsTaskScheduled(appName)) ? L"&Убрать из автозагрузки" : L"&Добавить в автозагрузку", L"&Изменить время автоскрытия"};
		for (UINT j = 0; j < 2; ++j) {
			MENUITEMINFO mii = {sizeof(MENUITEMINFO)};
			mii.fMask = MIIM_STRING | MIIM_ID | MIIM_FTYPE | MIIM_DATA | MIIM_STATE;
			mii.fType = MFT_OWNERDRAW;
			mii.fState = (pv.isAdminMode || j ? MFS_ENABLED : MFS_DISABLED);
			mii.fState |= (!j && IsTaskScheduled(appName) ? MFS_CHECKED : MFS_UNCHECKED);
			mii.wID = 1001 + j;
			mii.dwTypeData = const_cast<LPWSTR>(subMenuItems[j]);
			mii.dwItemData = (ULONG_PTR)subMenuItems[j];

			InsertMenuItemW(pv.hSettSubMenu, j, TRUE, &mii);
		}
		AppendMenuW(pv.hSettMenu, MF_POPUP | MF_OWNERDRAW, (UINT_PTR)pv.hSettSubMenu, L"&Меню");
		SetMenu(hwnd, pv.hSettMenu);
		HWND hText = CreateWindow(STATIC, WND_NAME_TEXT, WS_VISIBLE | WS_CHILD, 10, 0, 200, 30, hwnd, NULL, pv.hInstance, NULL);
		SendMessage(hText, WM_SETFONT, (WPARAM)hFont, TRUE);
		pv.hApplicationsList = CreateWindow(LISTBOX, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 10, 30, 200, 300, hwnd, (HMENU)ID_LIST_APPLICATIONS, pv.hInstance, NULL);
		SendMessage(pv.hApplicationsList, WM_SETFONT, (WPARAM)hFont, TRUE);
		pv.hText = CreateWindow(STATIC, WND_NAME_TEXT2, WS_VISIBLE | WS_CHILD, 380, 0, 300, 30, hwnd, NULL, pv.hInstance, NULL);
		SendMessage(pv.hText, WM_SETFONT, (WPARAM)hFont, TRUE);
		pv.hFavoritesList = CreateWindow(LISTBOX, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY, 380, 30, 200, 300, hwnd, (HMENU)ID_LIST_FAVORITES, pv.hInstance, NULL);
		SendMessage(pv.hFavoritesList, WM_SETFONT, (WPARAM)hFont, TRUE);
		pv.hAddButton = CreateWindow(BUTTON, ADDBUTTON_TEXT, WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, 300, 100, 40, 30, hwnd, (HMENU)ID_BUTTON_ADD, pv.hInstance, NULL);
		SendMessage(pv.hAddButton, WM_SETFONT, (WPARAM)hFont, TRUE);
		pv.hRemoveButton = CreateWindow(BUTTON, REMOVEBUTTON_TEXT, WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, 300, 140, 40, 30, hwnd, (HMENU)ID_BUTTON_REMOVE, pv.hInstance, NULL);
		SendMessage(pv.hRemoveButton, WM_SETFONT, (WPARAM)hFont, TRUE);
		pv.hReloadButton = CreateWindow(BUTTON, RELOADBUTTON_TEXT, WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, 300, 30, 40, 30, hwnd, (HMENU)ID_BUTTON_RELOAD, pv.hInstance, NULL);
		hFont = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Ariel");
		SendMessage(pv.hReloadButton, WM_SETFONT, (WPARAM)hFont, TRUE);
		UpdateFavoriteList();
		UpdateApplicationsList();

	}
      break;
	case WM_INITMENUPOPUP: {
		MENUINFO mi = {sizeof(mi)};
		mi.fMask = MIM_BACKGROUND;
		mi.hbrBack = CreateSolidBrush(RGB(240, 240, 240));
		SetMenuInfo((HMENU)wParam, &mi);
	}
       break;
	case WM_MEASUREITEM:
	{
		MEASUREITEMSTRUCT* pMeasureItem = (MEASUREITEMSTRUCT*)lParam;
		if (pMeasureItem->CtlType == ODT_MENU) {
			HDC hdc = GetDC(hwnd);
			RECT rc = {0, 0, 0, 0};
			LPCWSTR text = (LPCWSTR)pMeasureItem->itemData;
			DrawTextW(hdc, text, -1, &rc, DT_SINGLELINE | DT_CALCRECT | DT_LEFT);
			ReleaseDC(hwnd, hdc);
			pMeasureItem->itemWidth = rc.right - rc.left;
			pMeasureItem->itemHeight = rc.bottom - rc.top;
		}
	}
	return TRUE;
	case WM_DRAWITEM: {
		DRAWITEMSTRUCT* pDrawItem = (DRAWITEMSTRUCT*)lParam;
		if (pDrawItem->CtlType == ODT_MENU) {
			HDC hdc = pDrawItem->hDC;
			RECT rcItem = pDrawItem->rcItem;
			HBRUSH hBrush;
			COLORREF textColor;

			if (pDrawItem->itemState & ODS_SELECTED) {
				hBrush = CreateSolidBrush(RGB(230, 230, 230));
			} else if (pDrawItem->itemState & ODS_SELECTED && pDrawItem->itemState & ODS_HOTLIGHT) {
				hBrush = CreateSolidBrush(RGB(233, 233, 233));
			} else {
				hBrush = CreateSolidBrush(RGB(240, 240, 240));
			}
			FillRect(hdc, &rcItem, hBrush);
			DeleteObject(hBrush);

			if (pDrawItem->itemState & ODS_DISABLED) {
				textColor = RGB(128, 128, 128);
			} else {
				textColor = RGB(0, 0, 0);
			}

			SetTextColor(hdc, textColor);
			SetBkMode(hdc, TRANSPARENT);

			LPCWSTR text = (LPCWSTR)pDrawItem->itemData;
			if (text) {
				DrawTextW(hdc, text, -1, &rcItem, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_HIDEPREFIX);
			}
		}
	}
					return TRUE;
	case WM_SIZE: {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		MoveWindow(pv.hApplicationsList, 10, 30, width / 2 - 40, height - 30, TRUE);
		MoveWindow(pv.hText, width / 2 + 30, 0, 300, 30, TRUE);
		MoveWindow(pv.hFavoritesList, width / 2 + 30, 30, width / 2 - 40, height - 30, TRUE);
		MoveWindow(pv.hAddButton, width / 2 - 20, height / 2 - 40, 40, 30, TRUE);
		MoveWindow(pv.hRemoveButton, width / 2 - 20, height / 2, 40, 30, TRUE);
		MoveWindow(pv.hReloadButton, width / 2 - 20, 30, 40, 30, TRUE);
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}
	case WM_GETMINMAXINFO: {
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = wndX;
		mmi->ptMinTrackSize.y = wndY;
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_BUTTON_ADD: {
			LRESULT sel = SendMessage(pv.hApplicationsList, LB_GETCURSEL, NULL, NULL);
			if (sel != LB_ERR) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(pv.hApplicationsList, LB_GETITEMDATA, sel, NULL);
				HW->isFavorite = TRUE;
				int i = (int)SendMessage(pv.hFavoritesList, LB_ADDSTRING, NULL, (LPARAM)HW->windowTitle.c_str());
				SendMessage(pv.hFavoritesList, LB_SETITEMDATA, i, (LPARAM)HW);
				SendMessage(pv.hApplicationsList, LB_DELETESTRING, sel, NULL);
			}
			break;
		}
		case ID_BUTTON_REMOVE: {
			LRESULT sel = SendMessage(pv.hFavoritesList, LB_GETCURSEL, NULL, NULL);
			if (sel != LB_ERR) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(pv.hFavoritesList, LB_GETITEMDATA, sel, NULL);
				auto eraseWindow = [HW](std::vector<HiddenWindow>& vec) {
					auto it = std::find_if(vec.begin(), vec.end(),
             [&](const HiddenWindow& wnd) { return wnd.hwnd == HW->hwnd; });
					if (it != vec.end()) vec.erase(it);	};
				eraseWindow(favoriteWindows);
				eraseWindow(hiddenWindows);
				SendMessage(pv.hFavoritesList, LB_DELETESTRING, sel, NULL);
				ShowWindow(HW->hwnd, SW_SHOW);
				SetFocus(hwnd);
				delete HW;
				UpdateApplicationsList();
			}
			break;
		}
		case ID_BUTTON_RELOAD:
			UpdateApplicationsList();
			break;
		case ID_AUTOSTART: {
			MENUITEMINFO mii = {sizeof(MENUITEMINFO)};
			mii.fMask = MIIM_DATA | MIIM_STATE;
			GetMenuItemInfoW(pv.hSettMenu, ID_AUTOSTART, FALSE, &mii);
			if (mii.fState & MFS_CHECKED) {
				mii.fState &= ~MFS_CHECKED;
				mii.dwItemData = (ULONG_PTR)L"Добавить в автозагрузку";
				StartupChanging(false);
				LogAdd(L"Удалено из автозагрузки");
			} else {
				mii.fState |= MFS_CHECKED;
				mii.dwItemData = (ULONG_PTR)L"Убрать из автозагрузки";
				StartupChanging(true);
				LogAdd(L"Добавлено в автозагрузку");
			}
			SetMenuItemInfoW(pv.hSettMenu, ID_AUTOSTART, FALSE, &mii);
			DrawMenuBar(pv.settWin);
		}
       break;

		case ID_TIME_AUTOHIDE:
			EnableWindow(hwnd, FALSE);
			CreateTimerSettWnd();
			break;
		}
		break;
	case WM_CLOSE:
		CollapseToTrayFromFavorite();
		CheckFolderAndFile(FILEPATHNAME);
		{
			std::wstring favorite{};
			for (const auto& vec : favoriteWindows)
				favorite += serializeToWstring(vec);
			WriteMyFile(FILEPATHNAME, favorite);
			if (prevValue != pv.timerToHide)
				SaveNumberToRegistry();
			LogAdd(L"Файл настроек переписан");
			CheckAndDeleteOldLogs(GetCurrentDate() + L".log");
		}
		DeleteList(pv.hApplicationsList);
		DeleteList(pv.hFavoritesList);
		pv.settWin = NULL;
		pv.settTimer = NULL;
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
		SetTimer(hwnd, TIMER_ID, SECOND, NULL);
		break;
	case WM_TIMER:
		for (auto& fw : favoriteWindows) {//наверное лучше сделать отдельный поток под это действие
			FindWindowFromFile(fw, false);
		}
		break;
	case WM_HOTKEY: {
		HWND activeWnd = GetForegroundWindow();
		if (!activeWnd)	return 0;
		if (wParam == HK_CTIFA_ID) CollapseToTray(activeWnd);
		break;
	}
	case TRAY_ICON_MESSAGE:
		switch (lParam) {
		case WM_RBUTTONUP: {
			bool isDebug = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
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
					it->isFavorite = TIMED_WINDOW;
					SetTimer(NULL, reinterpret_cast<UINT_PTR>(it->hwnd), pv.timerToHide, TIMER_PROC);
				}
				hiddenWindows.erase(hiddenWindows.begin() + index);
			} else if (cmd == TB_EXIT)
				CloseApp();
			else if (cmd == TB_SETTINGS)
				OpenSettings();
			else if (cmd == TB_RESTART) {
				ReleaseMutex(pv.hMutex);
				CloseHandle(pv.hMutex);
				UnregisterHotKey(hwnd, HK_CTIFA_ID);
				LogAdd(L"Перезапуск запущен");
				RestartWithAdminRights();
				PostMessage(hwnd, WM_DESTROY, NULL, NULL);
			}
			PostMessage(hwnd, WM_NULL, NULL, NULL);
		}
						 break;
		case WM_LBUTTONDBLCLK:
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
		if (uMsg == pv.WM_TASKBAR_CREATED) {
			LogAdd(L"Пересоздаю иконку");
			AddTrayIcon(hwnd);
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ WCHAR* lpCmdLine, _In_ int nCmdShow) {
	pv.hMutex = CreateMutexW(NULL, TRUE, appName);
	if (pv.hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		LogAdd(L"Экземпляр программы уже запущен\tВыключение\n");
		if (pv.hMutex)
			CloseHandle(pv.hMutex);
		return 1;
	}
	pv.hInstance = hInstance;
	pv.isAdminMode = isRunAsAdmin();
	if (!pv.isAdminMode) {
		LogAdd(L"Без прав администратора");
		MBATTENTION(WT_ADMIN);
	} else LogAdd(L"Права администратора получены");
	DebugModCheck(lpCmdLine);
	pv.WM_TASKBAR_CREATED = RegisterWindowMessageW(L"TaskbarCreated");

	pv.wc1 = RegisterNewClass(L"CTRIFATrayApp", TrayProc, RGB(240, 240, 240));
	pv.wc2 = RegisterNewClass(L"CTIFA Settings", SettingsProc, RGB(240, 240, 240));
	pv.wc3 = RegisterNewClass(L"CTIFA Timer Settings", TimerSettProc, RGB(189, 189, 189));
	HWND hwnd = CreateWindowExW(0, pv.wc1.lpszClassName, pv.wc1.lpszClassName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, pv.hInstance, NULL);

	bool isCanCTIFA = RegisterHotKey(hwnd, HK_CTIFA_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'H');
	if (!isCanCTIFA) {
		MBERROR(ET_HOTKEY);
		LogAdd(ET_HOTKEY);
		UnregisterHotKey(hwnd, HK_CTIFA_ID);
		ReleaseMutex(pv.hMutex);
		CloseHandle(pv.hMutex);
		return -1;
	}
	AddTrayIcon(hwnd);
	{
		CheckFolderAndFile(FILEPATHNAME);
		std::wstring favorites = ReadSettingsFile();
		if (!favorites.empty())
			deserializeFromWstring(favorites);
		CheckAndDeleteOldLogs(GetCurrentDate() + L".log");

		LoadNumberFromRegistry();
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
	ReleaseMutex(pv.hMutex);
	CloseHandle(pv.hMutex);
	LogAdd(L"Завершение работы\n");
	return 0;
}