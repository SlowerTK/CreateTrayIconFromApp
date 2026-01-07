#pragma once
#include <string>
#include <vector>
#include <windows.h>

struct HotKeys {
	std::wstring nameArr;
	byte modKey, otherKey, newModKey, newOtherKey;
	bool canSet, isFixed, isActive;
	RECT textRect;
};
struct ProgramVariable {
	HWND trayWnd, settWin, settHK;
	LPCWSTR wc1, wc2, wc3;
	HINSTANCE hInstance;
	HMENU hMenu, hSettMenu, hSettSubMenu;
	HWND hAppText, hApplicationsList, hFavoritesList, hAddButton, hRemoveButton,
		hFavText, hReloadButton, hCheckBoxStartUp, hCheckBoxButton, hCheckBoxHint,
		hEditBoxText, hEditBox, hHotKeysText, hHotKeys, hHotKeysButton, hButton1, hButton2, hButton3;
	HANDLE hMutex;
	HFONT hFont;
	HHOOK hHook;
	UINT WM_TASKBAR_CREATED;
	unsigned int timerToHide;
	HotKeys hk;
	bool isDebugMode, isAdminMode, isHideOn, isDark;
	HBRUSH brDark, brDarkGray, brLight, brLightGray;
};
struct HiddenWindow {
	HWND hwnd = 0;
	HICON hIcon = 0;
	DWORD processID = 0;
	DWORD isFavorite = 0;
	std::wstring windowTitle;
	std::wstring className;
	std::wstring processName;
	std::wstring commandLine;
};

extern std::vector<HiddenWindow> hiddenWindows;
extern std::vector<HiddenWindow> favoriteWindows;
extern ProgramVariable pv;