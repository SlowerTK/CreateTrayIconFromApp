#include "functions.hpp"
#include "DarkMod.hpp"
#include <windowsx.h>

LRESULT CALLBACK CheckBoxSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (msg) {
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT rc;
		GetClientRect(hwnd, &rc);

		UINT state = (UINT)SendMessageW(hwnd, BM_GETCHECK, 0, 0);
		int themeState;
		BOOL isActive = IsWindowEnabled(hwnd);
		if (!isActive)
			themeState = (state == BST_CHECKED) ? CBS_CHECKEDDISABLED : CBS_UNCHECKEDDISABLED;
		else if (state == BST_CHECKED)
			themeState = CBS_CHECKEDNORMAL;
		else
			themeState = CBS_UNCHECKEDNORMAL;

		wchar_t buf[256];
		GetWindowTextW(hwnd, buf, SIZEOF(buf));
		HGDIOBJ oldFont = SelectObject(hdc, pv.hFont);


		HTHEME hTheme = OpenThemeData(hwnd, L"BUTTON");
		if (hTheme) {
			SIZE checkSize;
			GetThemePartSize(hTheme, hdc, BP_CHECKBOX, themeState, NULL, TS_TRUE, &checkSize);

			RECT rcCheck = { rc.left, rc.top, rc.left + checkSize.cx, rc.top + checkSize.cy };
			int offsetY = (rc.bottom - checkSize.cy) / 2;
			OffsetRect(&rcCheck, 0, offsetY);

			DrawThemeBackground(hTheme, hdc, BP_CHECKBOX, themeState, &rcCheck, NULL);

			rc.left = rcCheck.right + 4;

			COLORREF textColor;
			if (isActive) {
				if (pv.isDark)
					textColor = ID_CLR_LIGHT;
				else
					textColor = ID_CLR_BLACK;
			}
			else {
				if (pv.isDark)
					textColor = ID_CLR_DISABLED_D;
				else
					textColor = ID_CLR_DISABLED_L;
			}

			HBRUSH bgBrush = pv.isDark ? pv.brDark : pv.brLight;
			FillRect(hdc, &rc, bgBrush);

			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, textColor);
			DrawTextW(hdc, buf, -1, &rc, DT_SINGLELINE | DT_VCENTER);

			CloseThemeData(hTheme);
		}
		else {
			DrawTextW(hdc, buf, -1, &rc, DT_SINGLELINE | DT_VCENTER);
		}
		SelectObject(hdc, oldFont);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, CheckBoxSubclassProc, uIdSubclass);
		break;
	}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK StaticSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (msg) {
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT rc;
		GetClientRect(hwnd, &rc);

		HBRUSH bgBrush = pv.isDark ? pv.brDark : pv.brLight;
		FillRect(hdc, &rc, bgBrush);

		wchar_t buf[256];
		GetWindowTextW(hwnd, buf, _countof(buf));

		HGDIOBJ oldFont = SelectObject(hdc, pv.hFont);
		COLORREF textColor = IsWindowEnabled(hwnd)
			? (pv.isDark ? ID_CLR_LIGHT : ID_CLR_BLACK)
			: (pv.isDark ? ID_CLR_DISABLED_D : ID_CLR_DISABLED_L);

		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, textColor);

		DrawTextW(hdc, buf, -1, &rc, DT_SINGLELINE);

		SelectObject(hdc, oldFont);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_ENABLE:
		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, StaticSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}


void CALLBACK TIMER_PROC(HWND hwnd, UINT uint, UINT_PTR uintptr, DWORD dword) {
	auto it = std::find_if(favoriteWindows.begin(), favoriteWindows.end(),
		[&](const HiddenWindow& wnd) { return wnd.isFavorite == ID_WND_TIMED_HIDE; });
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
		}
		else {
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
	static int cx, cy;
	static std::wstring oldNameArr;
	switch (uMsg) {
	case WM_CREATE:
	{
		EnableDarkForWindow(hwnd, pv.isDark);
		RECT rect = {};
		GetWindowRect(hwnd, &rect);
		cx = rect.right - rect.left;
		cy = rect.bottom - rect.top;
		int dx = (cx - 106 * 3) / 4;
		oldNameArr = pv.hk.nameArr;
		pv.hk.modKey = pv.hk.otherKey = pv.hk.isFixed = pv.hk.canSet = 0;
		pv.hHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleW(NULL), 0);
		CreateWindowExW(NULL, TX_UI_CONTROL_STATIC, NULL, WS_VISIBLE | WS_CHILD | SS_BLACKFRAME, 0, 0, cx, cy, hwnd, NULL, pv.hInstance, NULL);
		pv.hButton1 = CreateWindowExW(0, TX_UI_CONTROL_BUTTON, TX_BTN_SAVE, WS_VISIBLE | WS_DISABLED | WS_CHILD | BS_CENTER | BS_VCENTER, (cx - (106 + dx) * 3), (cy - 45), 106, 30, hwnd, (HMENU)INT_PTR(ID_BTN_OK), pv.hInstance, NULL);
		SendMessage(pv.hButton1, WM_SETFONT, (WPARAM)pv.hFont, TRUE);
		pv.hButton2 = CreateWindowExW(0, TX_UI_CONTROL_BUTTON, TX_BTN_RESET, WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, (cx - (106 + dx) * 2), (cy - 45), 106, 30, hwnd, (HMENU)INT_PTR(ID_BTN_RESET), pv.hInstance, NULL);
		SendMessage(pv.hButton2, WM_SETFONT, (WPARAM)pv.hFont, TRUE);
		pv.hButton3 = CreateWindowExW(0, TX_UI_CONTROL_BUTTON, TX_BTN_CANCEL, WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER, (cx - (106 + dx)), (cy - 45), 106, 30, hwnd, (HMENU)INT_PTR(ID_BTN_CANCEL), pv.hInstance, NULL);
		SendMessage(pv.hButton3, WM_SETFONT, (WPARAM)pv.hFont, TRUE);

		ApplyThemeToControls(hwnd, pv.isDark);
		return 0;
	}
	case WM_SETTINGCHANGE:
		if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) {
			bool new_dark = IsSystemInDarkMode();
			if (new_dark != pv.isDark) {
				pv.isDark = new_dark;
				EnableDarkForWindow(hwnd, pv.isDark);
				ApplyThemeToControls(hwnd, pv.isDark);
				InvalidateRect(hwnd, NULL, TRUE);
			}
		}
		break;
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, pv.isDark ? pv.brDarkGray : pv.brLight);
		return 1;
	}
	case WM_CTLCOLORSTATIC:
	{
	case WM_CTLCOLORBTN:
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, pv.isDark ? ID_CLR_LIGHT : ID_CLR_BLACK);
		return (INT_PTR)(pv.isDark ? pv.brDark : pv.brLight);
	}
	case WM_PAINT:
	{
		EnableWindow(pv.hButton1, pv.hk.canSet);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		HBRUSH hBrush = pv.isDark ? CreateSolidBrush(ID_CLR_DARK) : CreateSolidBrush(ID_CLR_LIGHTGRAY);
		SelectObject(hdc, pv.hFont);
		{//нижняя полоса
			RECT rect = { 0, cy - 60, cx, cy };
			FillRect(hdc, &rect, hBrush);
		}
		{//Текст и верхняя полоса
			RECT rect = { 0, 0, cx, 50 };
			FillRect(hdc, &rect, hBrush);
			rect.left += 20;
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, pv.isDark ? ID_CLR_LIGHT : ID_CLR_BLACK);
			DrawTextW(hdc, TX_PRESS_HOTKEY, -1, &rect, DT_VCENTER | DT_SINGLELINE);
		}
		{//текст над нижней полосой
			LOGFONT lf = {};
			GetObjectW(pv.hFont, sizeof(LOGFONT), &lf);
			lf.lfHeight += 2;
			HFONT hSmallFont = CreateFontIndirect(&lf);
			HFONT hOldFont = (HFONT)SelectObject(hdc, hSmallFont);
			RECT rect = { 0,cy - 100,cx,cy - 60 };
			DrawTextW(hdc, IX_ALLOWED_KEYS, -1, &rect, /*DT_VCENTER |*/ DT_CENTER);
			SelectObject(hdc, hOldFont);
			DeleteObject(hSmallFont);
		}
		{//Текст сочетания клавиш
			LOGFONT lf = {};
			GetObject(pv.hFont, sizeof(LOGFONT), &lf);
			lf.lfHeight -= 18;
			HFONT hSmallFont = CreateFontIndirect(&lf);
			HFONT hOldFont = (HFONT)SelectObject(hdc, hSmallFont);
			pv.hk.textRect = { 20, 50, cx - 20, cy - 100 };
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
		case ID_BTN_OK:
			RegHotKey(pv.hk.newModKey, pv.hk.newOtherKey, ID_HOTKEY_HIDE_ACTIVE);
			SetZeroModKeysState();
			DestroyWindow(hwnd);
			break;
		case ID_BTN_RESET:
			pv.hk.nameArr = TX_TRAY_BTN_HOTKEY;
			RegHotKey(MOD_CONTROL | MOD_ALT, 'H', ID_HOTKEY_HIDE_ACTIVE);
			SetZeroModKeysState();
			DestroyWindow(hwnd);
			break;
		case ID_BTN_CANCEL:
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
	case WM_CREATE:
	{
		pv.isDark = IsSystemInDarkMode();
		EnableDarkForWindow(hwnd, pv.isDark);

		LogAdd(IT_OPEN_SETTINGS);
		prevValue[0] = static_cast<unsigned int>(pv.isHideOn);
		prevValue[1] = pv.timerToHide;

		HICON hIcon = LoadIcon(pv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
		pv.hFont = CreateFontIndirect(&ncm.lfMessageFont);

		struct ControlInfo {
			LPCWSTR className;
			LPCWSTR text;
			DWORD style;
			HMENU id;
			HWND* hWnd;
		} controls[] = {
			{ TX_UI_CONTROL_STATIC,    TX_WINDOWS_LIST,        NULL,  NULL,  &pv.hAppText },
			{ TX_UI_CONTROL_LIST,   NULL,                 WS_BORDER | LBS_SORT | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, (HMENU)INT_PTR(ID_LIST_APPS), &pv.hApplicationsList },
			{ TX_UI_CONTROL_STATIC,    TX_WINDOWS_SELECTED,       NULL,  NULL,  &pv.hFavText },
			{ TX_UI_CONTROL_LIST,   NULL,                 WS_BORDER | LBS_SORT | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, (HMENU)INT_PTR(ID_LIST_FAVORITES), &pv.hFavoritesList },
			{ TX_UI_CONTROL_BUTTON,    TX_BTN_ADD,       BS_CENTER | BS_VCENTER, (HMENU)INT_PTR(ID_BTN_ADD), &pv.hAddButton },
			{ TX_UI_CONTROL_BUTTON,    TX_BTN_REMOVE,    BS_CENTER | BS_VCENTER, (HMENU)INT_PTR(ID_BTN_REMOVE), &pv.hRemoveButton },
			{ TX_UI_CONTROL_BUTTON,    TX_BTN_RELOAD,    BS_CENTER | BS_VCENTER, (HMENU)INT_PTR(ID_BTN_RELOAD), &pv.hReloadButton },
			{ TX_UI_CONTROL_BUTTON,    TX_AUTORUN,         BS_AUTOCHECKBOX | WS_DISABLED, (HMENU)INT_PTR(ID_BTN_AUTOSTART), &pv.hCheckBoxStartUp },
			{ TX_UI_CONTROL_BUTTON,    TX_ICON_HINT,            NULL, (HMENU)INT_PTR(ID_BTN_HINT), &pv.hCheckBoxHint },
			{ TX_UI_CONTROL_BUTTON,    TX_CHECKBOX_RESTORE,        BS_AUTOCHECKBOX, (HMENU)INT_PTR(ID_BTN_TIME_AUTOHIDE), &pv.hCheckBoxButton },
			{ TX_UI_CONTROL_STATIC,    TX_EDITBOX_DELAY,         NULL, NULL, &pv.hEditBoxText },
			{ TX_UI_CONTROL_STATIC,    TX_HOTKEY_TEXT,          NULL, NULL, &pv.hHotKeysText },
			{ TX_UI_CONTROL_STATIC,    pv.hk.nameArr.c_str(),WS_BORDER | SS_CENTER | SS_CENTERIMAGE, NULL, &pv.hHotKeys},
			{ TX_UI_CONTROL_BUTTON,    TX_BTN_HOTKEY_SET,    BS_CENTER | BS_VCENTER, (HMENU)INT_PTR(ID_BTN_HOTKEY_CHANGE), &pv.hHotKeysButton }
		};

		for (auto& ctrl : controls) {
			HWND hWnd = CreateWindowW(ctrl.className, ctrl.text, ctrl.style | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, ctrl.id, pv.hInstance, NULL);
			SendMessage(hWnd, WM_SETFONT, (WPARAM)pv.hFont, TRUE);
			*ctrl.hWnd = hWnd;
		}

		SetWindowSubclass(pv.hCheckBoxButton, CheckBoxSubclassProc, 1, 0);
		SetWindowSubclass(pv.hCheckBoxStartUp, CheckBoxSubclassProc, 1, 0);

		if (IsTaskScheduled(SY_APP_NAME))SendMessage(pv.hCheckBoxStartUp, BM_SETCHECK, BST_CHECKED, 0);
		else SendMessage(pv.hCheckBoxStartUp, BM_SETCHECK, BST_UNCHECKED, 0);
		if (pv.isAdminMode) EnableWindow(pv.hCheckBoxStartUp, TRUE);

		SetWindowSubclass(pv.hEditBoxText, StaticSubclassProc, 1, 0);

		std::wstring timerStr = std::to_wstring(pv.timerToHide);
		pv.hEditBox = CreateWindowExW(0, TX_UI_CONTROL_EDIT, timerStr.c_str(), WS_VISIBLE | WS_BORDER | WS_CHILD | ES_NUMBER | ES_LEFT, 0, 0, 0, 0, hwnd, (HMENU)INT_PTR(ID_EDIT_DELAY_FIELD), pv.hInstance, NULL);
		SendMessage(pv.hEditBox, WM_SETFONT, (WPARAM)pv.hFont, TRUE);

		if (pv.isHideOn) {
			SendMessage(pv.hCheckBoxButton, BM_SETCHECK, BST_CHECKED, 0);
			EnableWindow(pv.hEditBoxText, TRUE);
			EnableWindow(pv.hEditBox, TRUE);
		}
		else {
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

		ApplyThemeToControls(hwnd, pv.isDark);
	}
	break;
	case WM_SETTINGCHANGE:
		if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) {
			bool new_dark = IsSystemInDarkMode();
			if (new_dark != pv.isDark) {
				pv.isDark = new_dark;
				EnableDarkForWindow(hwnd, pv.isDark);
				ApplyThemeToControls(hwnd, pv.isDark);
				InvalidateRect(hwnd, NULL, TRUE);
			}
		}
		break;
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, pv.isDark ? pv.brDark : pv.brLight);
		return 1;
	}
	case WM_CTLCOLORBTN:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORSTATIC:
	{
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, pv.isDark ? ID_CLR_LIGHT : ID_CLR_BLACK);

		return (INT_PTR)(pv.isDark ? pv.brDark : pv.brLight);
	}
	//case WM_PAINT:

	//	break;
	case WM_SIZE:
	{
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
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = CONST_WND_WIDTH;
		mmi->ptMinTrackSize.y = CONST_WND_HEIGHT;
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_BTN_ADD:
		{
			LRESULT sel = SendMessage(pv.hApplicationsList, LB_GETCURSEL, NULL, NULL);
			if (sel != LB_ERR) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(pv.hApplicationsList, LB_GETITEMDATA, sel, NULL);
				HW->isFavorite = TRUE;
				int i = (int)SendMessage(pv.hFavoritesList, LB_ADDSTRING, NULL, (LPARAM)HW->windowTitle.c_str());
				SendMessage(pv.hFavoritesList, LB_SETITEMDATA, i, (LPARAM)HW);
				SendMessage(pv.hApplicationsList, LB_DELETESTRING, sel, NULL);
				SendMessage(pv.hFavoritesList, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(GetMaxTextWidth(pv.hFavoritesList)) + 10, 0);
				SendMessage(pv.hApplicationsList, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(GetMaxTextWidth(pv.hApplicationsList)) + 10, 0);
			}
			break;
		}
		case ID_BTN_REMOVE:
		{
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
				SendMessage(pv.hApplicationsList, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(GetMaxTextWidth(pv.hApplicationsList)) + 10, 0);
				SendMessage(pv.hFavoritesList, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(GetMaxTextWidth(pv.hFavoritesList)) + 10, 0);
				ShowWindow(HW->hwnd, SW_SHOW);
				SetFocus(hwnd);
				delete HW;
				UpdateApplicationsList();
			}
			break;
		}
		case ID_BTN_RELOAD:
			UpdateApplicationsList();
			break;
		case ID_BTN_TIME_AUTOHIDE:
			if (SendMessage(pv.hCheckBoxButton, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				EnableWindow(pv.hEditBoxText, TRUE);
				EnableWindow(pv.hEditBox, TRUE);
			}
			else {
				EnableWindow(pv.hEditBoxText, FALSE);
				EnableWindow(pv.hEditBox, FALSE);
			}
			break;
		case ID_BTN_HINT:
			MBINFO(IX_WINDOW_HINT);
			break;
		case ID_EDIT_DELAY_FIELD:
		{
			if (HIWORD(wParam) != EN_UPDATE)
				break;
			HWND hEdit = (HWND)lParam;
			wchar_t buffer[16] = { 0 };
			GetWindowTextW(hEdit, buffer, 16);
			unsigned int value = wcstoul(buffer, NULL, 10);
			unsigned int prevValue = (unsigned int)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
			if (value < 1 || value > INT_MAX) {
				SetWindowTextW(hEdit, std::to_wstring(prevValue).c_str());
				SendMessageW(hEdit, EM_SETSEL, 0, -1);
			}
			else {
				SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)value);
				pv.timerToHide = value;
			}
		}
		break;
		case ID_BTN_HOTKEY_CHANGE:
			EnableWindow(hwnd, FALSE);
			CreateHKSettWnd();
			break;
		}
		break;
	case WM_CLOSE:
		CollapseToTrayFromFavorite();
		CheckFolderAndFile(SY_SETTINGS_FILENAME);
		{
			std::wstring favorite{};
			for (const auto& vec : favoriteWindows)
				favorite += serializeToWstring(vec);
			WriteMyFile(SY_SETTINGS_FILENAME, favorite);
			pv.isHideOn = IsDlgButtonChecked(hwnd, ID_BTN_TIME_AUTOHIDE);
			bool first = (prevValue[0] != static_cast<unsigned int>(pv.isHideOn)),
				second = (prevValue[1] != pv.timerToHide);
			if (first || second)
				SaveToRegistry(first, second);

			if (pv.isAdminMode)
				if (IsDlgButtonChecked(hwnd, ID_BTN_AUTOSTART)) {
					StartupChanging(true);
					LogAdd(IT_AUTORUN_ADDED);
					SendMessage(pv.hCheckBoxStartUp, BM_SETCHECK, BST_CHECKED, 0);
				}
				else {
					StartupChanging(false);
					LogAdd(IT_AUTORUN_REMOVED);
					SendMessage(pv.hCheckBoxStartUp, BM_SETCHECK, BST_UNCHECKED, 0);
				}

			LogAdd(IT_SETTINGS_UPDATED);
			CheckAndDeleteOldLogs(GetCurrentDate() + SY_FILE_EXTENSION);
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
		pv.brDark = CreateSolidBrush(ID_CLR_DARK);
		pv.brDarkGray = CreateSolidBrush(ID_CLR_DARKGRAY);
		pv.brLight = CreateSolidBrush(ID_CLR_LIGHT);
		pv.brLightGray = CreateSolidBrush(ID_CLR_LIGHTGRAY);
		SetTimer(hwnd, ID_TIMER_AUTOHIDE, CONST_AUTOHIDE_DELAY_MS, NULL);
		break;

	case WM_TIMER:
		for (auto& fw : favoriteWindows) {//наверное лучше сделать отдельный поток под это действие
			FindWindowFromFile(fw, false);
		}
		break;
	case WM_HOTKEY:
	{
		HWND activeWnd = GetForegroundWindow();
		if (!activeWnd)	return 0;
		if (wParam == ID_HOTKEY_HIDE_ACTIVE) CollapseToTray(activeWnd);
		break;
	}
	case TRAY_ICON_MESSAGE:
		switch (lParam) {
		case WM_RBUTTONUP:
		{
			RefreshDarkMenuTheme();
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
					it->isFavorite = ID_WND_TIMED_HIDE;
					SetTimer(NULL, reinterpret_cast<UINT_PTR>(it->hwnd), pv.timerToHide, TIMER_PROC);
				}
				hiddenWindows.erase(hiddenWindows.begin() + index);
			}
			else if (cmd == ID_TRAY_EXIT)
				CloseApp();
			else if (cmd == ID_TRAY_SETTINGS)
				OpenSettings();
			else if (cmd == ID_TRAY_RESTART) {
				ReleaseMutex(pv.hMutex);
				CloseHandle(pv.hMutex);
				UnregisterHotKey(hwnd, ID_HOTKEY_HIDE_ACTIVE);
				LogAdd(IT_RESTARTING);
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
		KillTimer(hwnd, ID_TIMER_AUTOHIDE);
		DeleteObject(pv.brDark);
		DeleteObject(pv.brDarkGray);
		DeleteObject(pv.brLight);
		DeleteObject(pv.brLightGray);
		CloseApp();
		break;
	default:
		if (uMsg == pv.WM_TASKBAR_CREATED) {
			LogAdd(IT_ICON_RECREATE);
			AddTrayIcon(hwnd);
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ WCHAR* lpCmdLine, _In_ int nCmdShow) {
	pv.hMutex = CreateMutexW(NULL, TRUE, SY_APP_NAME);
	if (pv.hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		LogAdd(WT_ALREADY_RUNNING);
		if (pv.hMutex)
			CloseHandle(pv.hMutex);
		return 1;
	}
	pv.hInstance = hInstance;
	pv.isAdminMode = isRunAsAdmin();
	if (!pv.isAdminMode) {
		LogAdd(IT_ADMIN_MISSING);
		MBATTENTION(WT_REQUIRE_ADMIN);
	}
	else LogAdd(IT_ADMIN_GRANTED);
	DebugModCheck(lpCmdLine);
	pv.WM_TASKBAR_CREATED = RegisterWindowMessageW(SY_TASKBAR_MSG);

	INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };
	InitCommonControlsEx(&icc);
	InitDarkMode();
	pv.isDark = IsSystemInDarkMode();

	pv.wc1 = RegisterNewClass(SY_CLASS_TRAY, TrayProc);
	pv.wc2 = RegisterNewClass(SY_CLASS_SETTINGS, SettingsProc);
	pv.wc3 = RegisterNewClass(SY_CLASS_TIMERSET, HKSettProc);
	pv.trayWnd = CreateWindowExW(0, pv.wc1, pv.wc1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, pv.hInstance, NULL);

	ReadHotKeys();
	bool isCanCTIFA = RegisterHotKey(pv.trayWnd, ID_HOTKEY_HIDE_ACTIVE, pv.hk.modKey, pv.hk.otherKey);
	if (!isCanCTIFA) {
		MBERROR(ET_HOTKEY_REGISTER);
		LogAdd(ET_HOTKEY_REGISTER);
		UnregisterHotKey(pv.trayWnd, ID_HOTKEY_HIDE_ACTIVE);
		//ReleaseMutex(pv.hMutex);
		//CloseHandle(pv.hMutex);
		//return -1;
	}
	AddTrayIcon(pv.trayWnd);
	{
		CheckFolderAndFile(SY_SETTINGS_FILENAME);
		std::wstring favorites = ReadSettingsFile();
		if (!favorites.empty())
			deserializeFromWstring(favorites);
		CheckAndDeleteOldLogs(GetCurrentDate() + SY_FILE_EXTENSION);

		LoadNumberFromRegistry();
	}
	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	RemoveTrayIcon(pv.trayWnd);
	UnregisterHotKey(pv.trayWnd, ID_HOTKEY_HIDE_ACTIVE);
	UnregisterClassW(pv.wc1, pv.hInstance);
	UnregisterClassW(pv.wc2, pv.hInstance);
	UnregisterClassW(pv.wc3, pv.hInstance);
	ReleaseMutex(pv.hMutex);
	CloseHandle(pv.hMutex);
	LogAdd(IT_SHUTDOWN);
	return 0;
}