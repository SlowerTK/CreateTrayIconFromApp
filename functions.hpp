#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include "resource.h"
#include <algorithm>
#include <comdef.h>
#include <Wbemidl.h>
#include <Psapi.h>
#include <ShlObj.h>
#include <sstream>
#include <iomanip>
#pragma comment(lib, "wbemuuid.lib")

#include <taskschd.h>
#include <comdef.h>
#include <atlbase.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
struct ProgramVariable {
	HWND settWin, settTimer;
	WNDCLASS wc1, wc2, wc3;
	HINSTANCE hInstance;
	HMENU hMenu, hSettMenu, hSettSubMenu;
	HWND hApplicationsList, hFavoritesList, hAddButton, hRemoveButton, hText, hReloadButton;
	HANDLE hMutex;
	UINT WM_TASKBAR_CREATED;
	unsigned int timerToHide;
	bool isDebugMode, isAdminMode;
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

			//L"Shell_TrayWnd", //Панель задач
			//L"Progman", //Рабочий стол
			//L"TaskManagerWindow", //Диспетчер задач
			//L"TopLevelWindowForOverflowXamlIsland", //Скрытая панель трея
			//L"WinUIDesktopWin32WindowClass", //Всплывающие окно трея у приложений использующих WinUI3
			//L"SystemTray_Main", //Всплывающие окно трея у системных приложений
			//L"Windows.UI.Core.CoreWindow",//Другие окна UI Windows
			//L"WindowsDashboard", //Панель слева
			//L"CTRIFATrayApp", //myself
			//L"CTIFA Settings", //myself2
			//L"Xaml_WindowedPopupClass" //Предпросмотр окна
};

WNDCLASS RegisterNewClass(LPCWSTR className, WNDPROC wndproc, COLORREF color);
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
void CreateTimerSettWnd();
void LoadNumberFromRegistry();
void SaveNumberToRegistry();
