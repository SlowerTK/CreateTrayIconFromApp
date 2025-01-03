#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <Psapi.h>
#include <ShlObj.h>
#include <sstream>
#include "resource.h"
#define POINT2CORD(point) point.left, point.top, point.right - point.left, point.bottom - point.top
#define SIZEOF(str) (sizeof(str) / sizeof(str[0]))
#define MBATTENTION(str) MessageBox(NULL, str, L"Внимание!", MB_ICONWARNING)
#define MBERROR(str) MessageBox(NULL, str, L"Ошибка!", MB_ICONERROR)

#define HK_CTIFA_ID 1
#define HK_FBL_ID 2
#define TB_SETTINGS 1
#define TB_EXIT 2
#define TRAY_ICON_MESSAGE (WM_USER + 1)
#define wndX 700
#define wndY 375
#define ID_LIST_APPLICATIONS 11
#define ID_LIST_FAVORITES    12
#define ID_BUTTON_ADD        13
#define ID_BUTTON_REMOVE     14
#define TID_UPDATE	1
extern bool isDebugMode;
struct ProgramVariable {
	HWND settWin;
	WNDCLASS wc1, wc2;
	HINSTANCE hInstance;
	HMENU hMenu;
	HWND hApplicationsList, hFavoritesList, hAddButton, hRemoveButton, hText;
};
struct HiddenWindow {
	HWND hwnd = 0;
	std::wstring windowTitle;
	HICON hIcon = 0;
	DWORD processID = 0;
	bool isFavorite = 0;
	std::wstring className;
	std::wstring processName;
};
typedef HiddenWindow FavoriteWindow;
struct FullscreenBorderlessWindow {
	HWND hwnd;
	DWORD dwStyle;
	DWORD dwExStyle;
	RECT rect;
};
extern std::vector<HiddenWindow> hiddenWindows;
extern std::vector<FavoriteWindow> favoriteWindows;
extern std::vector<FullscreenBorderlessWindow> fullscreenBorderlessWindows;

extern ProgramVariable pv;

static std::wstring exceptionClassNames[] = {
			L"",
			L"Shell_TrayWnd", //Панель задач
			L"Progman", //Рабочий стол
			L"TaskManagerWindow", //Диспетчер задач
			L"TopLevelWindowForOverflowXamlIsland", //Скрытая панель трея
			L"WinUIDesktopWin32WindowClass", //Всплывающие окно трея у приложений использующих WinUI3
			L"SystemTray_Main", //Всплывающие окно трея у системных приложений
			L"Windows.UI.Core.CoreWindow",//Другие окна UI Windows
			L"WindowsDashboard", //Панель слева
			L"CTRIFATrayApp", //myself
			L"CTIFA Settings", //myself2
			L"Xaml_WindowedPopupClass" //Предпросмотр окна
};

WNDCLASS RegisterNewClass(LPCWSTR className, WNDPROC wndproc);
bool RunAsAdmin();
void DebugModCheck(WCHAR* lpCmdLine);
static HBITMAP IconToBitmap(HICON hIcon);
void OpenSettings();
void UpdateTrayMenu();
void CloseApp();
void UpdateApplicationsList();
void UpdateFavoriteList();
void CollapseToTray(HWND hwnd, HiddenWindow* HW = nullptr);
void CollapseToTrayFromFavorite();
void DeleteList(HWND list);
std::wstring GetAllowedClassName(HWND hwnd);
DWORD GetProcessId(HWND hwnd);
std::wstring GetProcessName(DWORD processID);
std::wstring s2ws(const std::string& str);
void CheckFolderAndFile();
std::string ReadSettingsFile();
std::string serializeToString(const FavoriteWindow& s);
void deserializeFromString(const std::string& str);
void WriteSettingsFile(const std::string& content);