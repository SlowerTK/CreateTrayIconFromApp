#include "functions.hpp"

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

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP)) {
		KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;
		wchar_t vk = static_cast<wchar_t>(kbd->vkCode);
		byte oldModKey = pv.hk.modKey;
		byte oldOtherKey = pv.hk.otherKey;
		
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			if (!pv.hk.isActive) return CallNextHookEx(NULL, nCode, wParam, lParam);
			switch (vk) {
			case VK_LMENU:
			case VK_RMENU:
				pv.hk.modKey |= MOD_ALT;
				break;
			case VK_LCONTROL:
			case VK_RCONTROL:
				pv.hk.modKey |= MOD_CONTROL;
				break;
			case VK_LSHIFT:
			case VK_RSHIFT:
				pv.hk.modKey |= MOD_SHIFT;
				break;
			case VK_LWIN:
			case VK_RWIN:
				pv.hk.modKey |= MOD_WIN;
				break;
			default:
				pv.hk.otherKey = (byte)vk;
				//OutputDebugStringW(std::to_wstring(pv.hk.otherKey).c_str());
				break;
			}
			if (oldModKey == pv.hk.modKey && oldOtherKey == pv.hk.otherKey) {
				return 1;
			}
			pv.hk.isFixed = pv.hk.canSet = 0;
			if (pv.hk.modKey && pv.hk.otherKey) {
				pv.hk.isFixed = 1;
				if (pv.hk.canSet = RegisterHotKey(pv.settHK, 0, pv.hk.modKey, pv.hk.otherKey)) {
					pv.hk.nameArr = convertKeysToWstring(pv.hk.modKey, pv.hk.otherKey);
					pv.hk.newModKey = pv.hk.modKey;
					pv.hk.newOtherKey = pv.hk.otherKey;
					//OutputDebugStringW((std::wstring(L"Подходящие сочетание ") + pv.hk.nameArr + L"\n").c_str());
					UnregisterHotKey(pv.settHK, 0);
				}
			}
		} else {
			switch (vk) {
			case VK_LMENU:
			case VK_RMENU:
				pv.hk.modKey &= ~MOD_ALT;
				break;
			case VK_LCONTROL:
			case VK_RCONTROL:
				pv.hk.modKey &= ~MOD_CONTROL;
				break;
			case VK_LSHIFT:
			case VK_RSHIFT:
				pv.hk.modKey &= ~MOD_SHIFT;
				break;
			case VK_LWIN:
			case VK_RWIN:
				pv.hk.modKey &= ~MOD_WIN;
				break;
			default:
				if (vk == pv.hk.otherKey)
					pv.hk.otherKey = 0;
				else {
					return CallNextHookEx(NULL, nCode, wParam, lParam);
				}
				break;
			}
			if (pv.hk.isFixed) {
				return CallNextHookEx(NULL, nCode, wParam, lParam);
			}
		}
		pv.hk.nameArr = convertKeysToWstring(pv.hk.modKey, pv.hk.otherKey);
		InvalidateRect(pv.settHK, &pv.hk.textRect, TRUE);
		return (pv.hk.isActive) ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam);
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}


static LRESULT CALLBACK HKSettProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	////static HWND parent = NULL;
	//switch (uMsg) {
	//case WM_CREATE: {
	//	//parent = (HWND)GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT);
	static int cx, cy;
	static std::wstring oldNameArr;
	switch (uMsg) {
	case WM_CREATE: {
		RECT rect = {};
		GetWindowRect(hwnd, &rect);
		cx = rect.right - rect.left;
		cy = rect.bottom - rect.top;
		int dx = (cx - 106 * 3) / 4;
		//OutputDebugString(std::wstring(L"cx = " + std::to_wstring(cx) + L" cy = " + std::to_wstring(cy) + L"\n").c_str());
		oldNameArr = pv.hk.nameArr;
		pv.hk.modKey = pv.hk.otherKey = pv.hk.isFixed = pv.hk.canSet = 0;
		pv.hHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleW(NULL), 0);
		CreateWindowExW(NULL, STATIC, NULL, WS_VISIBLE | WS_CHILD | SS_BLACKFRAME, 0, 0, cx, cy, hwnd, NULL, pv.hInstance, NULL);
		pv.hButton1 = CreateWindowExW(0, BUTTON, L"Сохранить", WS_VISIBLE | WS_DISABLED | WS_CHILD | BS_CENTER | BS_VCENTER, (cx - (106 + dx) * 3), (cy-45), 106, 30, hwnd, (HMENU)INT_PTR(ID_OK_BUTTON), pv.hInstance, NULL);
		SendMessage(pv.hButton1, WM_SETFONT, (WPARAM)pv.hFont, TRUE);
		pv.hButton2 = CreateWindowExW(0, BUTTON, L"Сброс", WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, (cx - (106 + dx) * 2), (cy - 45), 106, 30, hwnd, (HMENU)INT_PTR(ID_RESET_BUTTON), pv.hInstance, NULL);
		SendMessage(pv.hButton2, WM_SETFONT, (WPARAM)pv.hFont, TRUE);
		pv.hButton3 = CreateWindowExW(0, BUTTON, L"Отмена", WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, (cx - (106 + dx)), (cy - 45), 106, 30, hwnd, (HMENU)INT_PTR(ID_CANCEL_BUTTON), pv.hInstance, NULL);
		SendMessage(pv.hButton3, WM_SETFONT, (WPARAM)pv.hFont, TRUE);
		return 0;
	}
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN: {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}
	case WM_PAINT: {
		EnableWindow(pv.hButton1, pv.hk.canSet);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
		SelectObject(hdc, pv.hFont);
		{//нижняя тёмная полоса
			RECT rect = {0, cy-60, cx, cy};
			FillRect(hdc, &rect, hBrush);
		}
		{//Текст и верхняя полоса
			RECT rect = {0, 0, cx, 50};
			FillRect(hdc, &rect, hBrush);
			rect.left += 20;
			SetBkMode(hdc, TRANSPARENT);
			DrawTextW(hdc, L"Введите новое сочетание клавиш", -1, &rect, DT_VCENTER | DT_SINGLELINE);
		}
		{//текст над нижней полосой
			LOGFONT lf = {};
			GetObjectW(pv.hFont, sizeof(LOGFONT), &lf);
			lf.lfHeight += 2;
			HFONT hSmallFont = CreateFontIndirect(&lf);
			HFONT hOldFont = (HFONT)SelectObject(hdc, hSmallFont);
			RECT rect = {0,cy-100,cx,cy - 60};
			DrawTextW(hdc, L"Допустимы только сочетания клавиш, имеющие не менее одной модальной клавиши,\n такой как Ctrl, Alt, Shift или Win.", -1, &rect, /*DT_VCENTER |*/ DT_CENTER);
			SelectObject(hdc, hOldFont);
			DeleteObject(hSmallFont);
		}
		{//Текст сочетания клавиш
			LOGFONT lf = {};
			GetObject(pv.hFont, sizeof(LOGFONT), &lf);
			lf.lfHeight -= 18;
			HFONT hSmallFont = CreateFontIndirect(&lf);
			HFONT hOldFont = (HFONT)SelectObject(hdc, hSmallFont);
			pv.hk.textRect = {20, 50, cx-20, cy - 100};
			SIZE textSize;
			GetTextExtentPoint32W(hdc, pv.hk.nameArr.c_str(), static_cast<int>(pv.hk.nameArr.length()), &textSize);
			int textX = pv.hk.textRect.left + (pv.hk.textRect.right - pv.hk.textRect.left - textSize.cx) / 2;
			int textY = pv.hk.textRect.top + (pv.hk.textRect.bottom - pv.hk.textRect.top - textSize.cy) / 2;
			SetBkMode(hdc, TRANSPARENT);
			TextOutW(hdc, textX, textY, pv.hk.nameArr.c_str(), static_cast<int>(pv.hk.nameArr.length()));
			SelectObject(hdc, hOldFont);
			DeleteObject(hSmallFont);
		}
		DeleteObject(hBrush);
		EndPaint(hwnd, &ps);
	} break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_OK_BUTTON:
			RegHotKey(pv.hk.newModKey, pv.hk.newOtherKey, HK_CTIFA_ID);
			SetZeroModKeysState();
			DestroyWindow(hwnd);
			break;
		case ID_RESET_BUTTON:
			pv.hk.nameArr = TB_HOTKEY_TEXT;
			RegHotKey(MOD_CONTROL | MOD_ALT, 'H', HK_CTIFA_ID);
			SetZeroModKeysState();
			DestroyWindow(hwnd);
			break;
		case ID_CANCEL_BUTTON:
			//не изменять ничего
			pv.hk.nameArr = oldNameArr;
			SetZeroModKeysState();
			DestroyWindow(hwnd);
			break;
		}
		break;
	case WM_KILLFOCUS:
		pv.hk.isActive = false;
		SetZeroModKeysState();
		return 0;
	case WM_SETFOCUS:
		pv.hk.isActive = true;
		return 0;
	//case WM_NCCALCSIZE:
	//	return 0;
	case WM_NCHITTEST:
		return HTCAPTION;
	//case WM_ACTIVATE:
	//	if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) {
	//		if (IsWindow(pv.settHK)) {
	//			SetForegroundWindow(pv.settHK);
	//			SetFocus(pv.settHK);
	//		}
	//	}
	//	break;
	case WM_CLOSE:
		break;
	case WM_DESTROY: 
		if (pv.hHook) UnhookWindowsHookEx(pv.hHook);
		EnableWindow(pv.settWin/*parent*/, TRUE);
		SetWindowTextW(pv.hHotKeys, pv.hk.nameArr.c_str());
		SetActiveWindow(pv.settWin);
		//SetForegroundWindow(pv.settWin/*parent*/);

		pv.settHK = NULL;
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

static LRESULT CALLBACK SettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static unsigned int prevValue[2];
	switch (uMsg) {
	case WM_CREATE: {
		LogAdd(L"Открыты настройки");
		prevValue[0] = static_cast<unsigned int>(pv.isHideOn);
		prevValue[1] = pv.timerToHide;

		HICON hIcon = LoadIcon(pv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		NONCLIENTMETRICS ncm = {sizeof(NONCLIENTMETRICS)};
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
		pv.hFont = CreateFontIndirect(&ncm.lfMessageFont);

		struct ControlInfo {
			LPCWSTR className;
			LPCWSTR text;
			DWORD style;
			HMENU id;
			HWND* hWnd;
		} controls[] = {
			{ STATIC,    WND_NAME_TEXT,        NULL,  NULL,  &pv.hAppText },
			{ LISTBOX,   NULL,                 WS_BORDER | LBS_SORT | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, (HMENU)INT_PTR(ID_LIST_APPLICATIONS), &pv.hApplicationsList },
			{ STATIC,    WND_NAME_TEXT2,       NULL,  NULL,  &pv.hFavText },
			{ LISTBOX,   NULL,                 WS_BORDER | LBS_SORT | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, (HMENU)INT_PTR(ID_LIST_FAVORITES), &pv.hFavoritesList },
			{ BUTTON,    ADDBUTTON_TEXT,       BS_CENTER | BS_VCENTER, (HMENU)INT_PTR(ID_BUTTON_ADD), &pv.hAddButton },
			{ BUTTON,    REMOVEBUTTON_TEXT,    BS_CENTER | BS_VCENTER, (HMENU)INT_PTR(ID_BUTTON_REMOVE), &pv.hRemoveButton },
			{ BUTTON,    RELOADBUTTON_TEXT,    BS_CENTER | BS_VCENTER, (HMENU)INT_PTR(ID_BUTTON_RELOAD), &pv.hReloadButton },
			{ BUTTON,    STARTUP_TEXT,         BS_AUTOCHECKBOX | WS_DISABLED, (HMENU)INT_PTR(ID_BUTTON_AUTOSTART), &pv.hCheckBoxStartUp },
			{ BUTTON,    HINT_TEXT,            NULL, (HMENU)INT_PTR(ID_BUTTON_HINT), &pv.hCheckBoxHint },
			{ BUTTON,    CHECKBOX_TEXT,        BS_AUTOCHECKBOX, (HMENU)INT_PTR(ID_BUTTON_TIMEAUTOHIDE), &pv.hCheckBoxButton },
			{ STATIC,    EDITBOX_TEXT,         NULL, NULL, &pv.hEditBoxText },
			{ STATIC,    HOTKEY_TEXT,          NULL, NULL, &pv.hHotKeysText },
			{ STATIC,    pv.hk.nameArr.c_str(),WS_BORDER | SS_CENTER | SS_CENTERIMAGE, NULL, &pv.hHotKeys},
			{ BUTTON,    HOTKEYBUTTON_TEXT,    BS_CENTER | BS_VCENTER, (HMENU)INT_PTR(ID_BUTTON_HK), &pv.hHotKeysButton }
		};

		for (auto& ctrl : controls) {
			HWND hWnd = CreateWindowW(ctrl.className, ctrl.text, ctrl.style | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, ctrl.id, pv.hInstance, NULL);
			SendMessage(hWnd, WM_SETFONT, (WPARAM)pv.hFont, TRUE);
			*ctrl.hWnd = hWnd;
		}

		if (IsTaskScheduled(appName))SendMessage(pv.hCheckBoxStartUp, BM_SETCHECK, BST_CHECKED, 0);
		else SendMessage(pv.hCheckBoxStartUp, BM_SETCHECK, BST_UNCHECKED, 0);
		if (pv.isAdminMode) EnableWindow(pv.hCheckBoxStartUp, TRUE);

		std::wstring timerStr = std::to_wstring(pv.timerToHide);
		pv.hEditBox = CreateWindowExW(WS_EX_CLIENTEDGE, EDIT, timerStr.c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_LEFT, 0, 0, 0, 0, hwnd, (HMENU)INT_PTR(ID_EDIT_FIELD), pv.hInstance, NULL);
		SendMessage(pv.hEditBox, WM_SETFONT, (WPARAM)pv.hFont, TRUE);

		if (pv.isHideOn) {
			SendMessage(pv.hCheckBoxButton, BM_SETCHECK, BST_CHECKED, 0);
			EnableWindow(pv.hEditBoxText, TRUE);
			EnableWindow(pv.hEditBox, TRUE);
		} else {
			SendMessage(pv.hCheckBoxButton, BM_SETCHECK, BST_UNCHECKED, 0);
			EnableWindow(pv.hEditBoxText, FALSE);
			EnableWindow(pv.hEditBox, FALSE);
		}

		LOGFONT lf = ncm.lfMessageFont;
		lf.lfHeight -= 8;
		pv.hFont = CreateFontIndirect(&lf);
		SendMessage(pv.hHotKeys, WM_SETFONT, (WPARAM)pv.hFont, TRUE);
		lf.lfHeight += 8;
		pv.hFont = CreateFontIndirect(&lf);
		SendMessage(pv.hHotKeysButton, WM_SETFONT, (WPARAM)pv.hFont, TRUE);

		UpdateFavoriteList();
		UpdateApplicationsList();
	}
				  break;
	case WM_CTLCOLORBTN:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wParam;
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LRESULT)GetStockObject(WHITE_PEN);
	}
	//case WM_PAINT:

	//	break;
	case WM_SIZE: {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);
		const int fixedWidth = 400;
		int rightCorner = width - (fixedWidth + 10);

		MoveWindow(pv.hAppText, 10, 5, 300, 30, TRUE);
		MoveWindow(pv.hApplicationsList, 10, 30, width / 2 - 250, height - 30, TRUE);
		MoveWindow(pv.hFavText, width / 2 - 170, 5, 300, 30, TRUE);
		MoveWindow(pv.hFavoritesList, width / 2 - 170, 30, width / 2 - 250, height - 30, TRUE);
		MoveWindow(pv.hAddButton, width / 2 - 225, height / 2 - 40, 40, 30, TRUE);
		MoveWindow(pv.hRemoveButton, width / 2 - 225, height / 2, 40, 30, TRUE);
		MoveWindow(pv.hReloadButton, width / 2 - 225, height / 2 - 120, 40, 30, TRUE);
		MoveWindow(pv.hCheckBoxStartUp, rightCorner, height / 2 - 140, fixedWidth - 50, 30, TRUE);
		MoveWindow(pv.hCheckBoxButton, rightCorner, height / 2 - 110, fixedWidth - 50, 30, TRUE);
		MoveWindow(pv.hCheckBoxHint, width - 40, height / 2 - 110, 30, 30, TRUE);
		MoveWindow(pv.hEditBoxText, rightCorner, height / 2 - 80, fixedWidth, 30, TRUE);
		MoveWindow(pv.hEditBox, rightCorner, height / 2 - 50, fixedWidth, 30, TRUE);
		MoveWindow(pv.hHotKeysText, rightCorner, height / 2, fixedWidth, 30, TRUE);
		MoveWindow(pv.hHotKeys, rightCorner, height / 2 + 30, fixedWidth, height / 6 + 20, TRUE);
		MoveWindow(pv.hHotKeysButton, rightCorner, height - height / 3 + 60, fixedWidth, 30, TRUE);

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
				SendMessage(pv.hFavoritesList, LB_SETHORIZONTALEXTENT, GetMaxTextWidth(pv.hFavoritesList) + 10, 0);
				SendMessage(pv.hApplicationsList, LB_SETHORIZONTALEXTENT, GetMaxTextWidth(pv.hApplicationsList) + 10, 0);
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
				SendMessage(pv.hApplicationsList, LB_SETHORIZONTALEXTENT, GetMaxTextWidth(pv.hApplicationsList) + 10, 0);
				SendMessage(pv.hFavoritesList, LB_SETHORIZONTALEXTENT, GetMaxTextWidth(pv.hFavoritesList) + 10, 0);
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
		case ID_BUTTON_TIMEAUTOHIDE:
			if (SendMessage(pv.hCheckBoxButton, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				EnableWindow(pv.hEditBoxText, TRUE);
				EnableWindow(pv.hEditBox, TRUE);
			} else {
				EnableWindow(pv.hEditBoxText, FALSE);
				EnableWindow(pv.hEditBox, FALSE);
			}
			break;
		case ID_BUTTON_HINT:
			MessageBox(hwnd, HINT_MSG, L"Инфо", MB_OK | MB_ICONINFORMATION);
			break;
		case ID_EDIT_FIELD: {
			if (HIWORD(wParam) != EN_UPDATE)
				break;
			HWND hEdit = (HWND)lParam;
			wchar_t buffer[16] = {0};
			GetWindowTextW(hEdit, buffer, 16);
			unsigned int value = wcstoul(buffer, NULL, 10);
			unsigned int prevValue = (unsigned int)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
			if (value < 1 || value > INT_MAX) {
				SetWindowTextW(hEdit, std::to_wstring(prevValue).c_str());
				SendMessageW(hEdit, EM_SETSEL, 0, -1);
			} else {
				SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)value);
				pv.timerToHide = value;
			}
		}
		break;
		case ID_BUTTON_HK:
			EnableWindow(hwnd, FALSE);
			CreateHKSettWnd();
			break;
		}
		break;
	//case WM_ACTIVATE:
	//	if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) {
	//		if (IsWindow(pv.settHK)) {
	//			SetForegroundWindow(pv.settHK);
	//			SetFocus(pv.settHK);
	//		}
	//	}
	//	break;
	case WM_CLOSE:
		CollapseToTrayFromFavorite();
		CheckFolderAndFile(FILEPATHNAME);
		{
			std::wstring favorite{};
			for (const auto& vec : favoriteWindows)
				favorite += serializeToWstring(vec);
			WriteMyFile(FILEPATHNAME, favorite);
			pv.isHideOn = IsDlgButtonChecked(hwnd, ID_BUTTON_TIMEAUTOHIDE);
			bool first = (prevValue[0] != static_cast<unsigned int>(pv.isHideOn)),
				second = (prevValue[1] != pv.timerToHide);
			if (first || second)
				SaveToRegistry(first, second);

			if (pv.isAdminMode)
				if (IsDlgButtonChecked(hwnd, ID_BUTTON_AUTOSTART)) {
					StartupChanging(true);
					LogAdd(L"Добавлено в автозагрузку");
					SendMessage(pv.hCheckBoxStartUp, BM_SETCHECK, BST_CHECKED, 0);
				} else {
					StartupChanging(false);
					LogAdd(L"Удалено из автозагрузки");
					SendMessage(pv.hCheckBoxStartUp, BM_SETCHECK, BST_UNCHECKED, 0);
				}

			LogAdd(L"Файл настроек переписан");
			CheckAndDeleteOldLogs(GetCurrentDate() + L".log");
		}
		DeleteList(pv.hApplicationsList);
		DeleteList(pv.hFavoritesList);
		DeleteObject(pv.hFont);
		pv.settWin = NULL;
		pv.settHK = NULL;
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
	case WM_CTLCOLORSTATIC:
		return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
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
			int cmd = TrackPopupMenu(pv.hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_BOTTOMALIGN | TPM_CENTERALIGN, cursorPos.x, cursorPos.y, NULL, hwnd, NULL);
			if (cmd >= 1000 && cmd < 1000 + hiddenWindows.size()) {
				int index = cmd - 1000;
				ShowWindow(hiddenWindows[index].hwnd, SW_SHOW);
				SetForegroundWindow(hiddenWindows[index].hwnd);
				auto it = std::find_if(favoriteWindows.begin(), favoriteWindows.end(),
									   [&](const HiddenWindow& wnd) { return wnd.hwnd == hiddenWindows[index].hwnd; });
				if (it != favoriteWindows.end() && pv.isHideOn && (GetKeyState(VK_SHIFT) & 0x8000) == 0) {
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

	INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES};
	InitCommonControlsEx(&icc);

	pv.wc1 = RegisterNewClass(L"CTRIFATrayApp", TrayProc);
	pv.wc2 = RegisterNewClass(L"CTIFA Settings", SettingsProc);
	pv.wc3 = RegisterNewClass(L"CTIFA Timer Settings", HKSettProc);
	pv.trayWnd = CreateWindowExW(0, pv.wc1, pv.wc1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, pv.hInstance, NULL);

	ReadHotKeys();
	bool isCanCTIFA = RegisterHotKey(pv.trayWnd, HK_CTIFA_ID, pv.hk.modKey, pv.hk.otherKey);
	if (!isCanCTIFA) {
		MBERROR(ET_HOTKEY);
		LogAdd(ET_HOTKEY);
		UnregisterHotKey(pv.trayWnd, HK_CTIFA_ID);
		//ReleaseMutex(pv.hMutex);
		//CloseHandle(pv.hMutex);
		//return -1;
	}
	AddTrayIcon(pv.trayWnd);
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

	RemoveTrayIcon(pv.trayWnd);
	UnregisterHotKey(pv.trayWnd, HK_CTIFA_ID);
	UnregisterClassW(pv.wc1, pv.hInstance);
	UnregisterClassW(pv.wc2, pv.hInstance);
	UnregisterClassW(pv.wc3, pv.hInstance);
	ReleaseMutex(pv.hMutex);
	CloseHandle(pv.hMutex);
	LogAdd(L"Завершение работы\n");
	return 0;
}