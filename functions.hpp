#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include "resource.h"
#include <algorithm>
#include <Wbemidl.h>
#include <Psapi.h>
#include <ShlObj.h>
#include <sstream>
#include <iomanip>
#include <map>
#pragma comment(lib, "wbemuuid.lib")

#include <taskschd.h>
#include <comdef.h>
#include <atlbase.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

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
	bool isDebugMode, isAdminMode, isHideOn;
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

static std::wstring exceptionProcessNames[] = {
	L"", //Пустая строка
	L"Explorer.EXE", //Проводник
	L"TextInputHost.exe", //Интерфейс ввода windows
	L"ShellExperienceHost.exe", //Центр уведомлений
	L"ShellHost.exe", //Быстрые настройки
	L"SearchHost.exe", //Поиск
	L"PowerToys.PowerLauncher.exe", //PowerToys Run
	L"ApplicationFrameHost.exe", //Параметры
	L"SystemSettings.exe", //Параметры
};
static std::wstring exceptionClassNames[] = {
	L"",
	L"CTRIFATrayApp",
	L"CTIFA Settings",
	L"CTIFA Timer Settings"
};

LPCWSTR RegisterNewClass(LPCWSTR className, WNDPROC wndproc);
bool isRunAsAdmin();
void DebugModCheck(wchar_t* lpCmdLine);
static HBITMAP IconToBitmap(HICON hIcon);
void OpenSettings();
void UpdateTrayMenu(bool isDebug);
void CloseApp();
void UpdateApplicationsList();
void UpdateFavoriteList();
void CollapseToTray(HWND hwnd, HiddenWindow* HW = nullptr);
void CollapseToTrayFromFavorite();
void DeleteList(HWND list);
std::wstring GetWstringClassName(HWND hwnd);
DWORD GetProcessId(HWND hwnd);
std::wstring GetAllowedProcessName(DWORD processID);
void CheckFolderAndFile(const std::wstring& fileName);
std::wstring ReadSettingsFile();
std::wstring serializeToWstring(const HiddenWindow& hw);
void deserializeFromWstring(const std::wstring& str);
void WriteMyFile(std::wstring fileName, const std::wstring& content, bool isAdd = false);
void FindWindowFromFile(HiddenWindow& windowToFind, bool isFile);
std::wstring GetWindowTitle(HWND hwnd);
void SerchWindow(HiddenWindow& window);
void RestartWithAdminRights();
std::wstring GetCurrentDate();
void LogAdd(std::wstring&& content);
void CheckAndDeleteOldLogs(const std::wstring& currentLogFile);
bool InitializeTaskService(CComPtr<ITaskService>& pService);
bool GetRootFolder(CComPtr<ITaskService>& pService, CComPtr<ITaskFolder>& pRootFolder);
bool IsTaskScheduled(const std::wstring& taskName);
void DeleteScheduledTask(CComPtr<ITaskService>& pService, const std::wstring& taskName);
void CreateScheduledTask(CComPtr<ITaskService>& pService, const std::wstring& taskName, const std::wstring& exePath);
void StartupChanging(bool isAdd);
void CreateHKSettWnd();
void LoadNumberFromRegistry();
void SaveToRegistry(bool a, bool b);
void SetZeroModKeysState();
void SetZeroModKeysState(BYTE* keyState);
void RegHotKey(UINT mod, UINT other, int id);
std::wstring convertKeysToWstring(UINT modKeys, UINT otherKey);
void SaveHotKeys(byte mod, byte other);
void ReadHotKeys();
void AddTrayIcon(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);