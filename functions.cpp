#pragma once
#include "functions.hpp"
std::vector<HiddenWindow> hiddenWindows = {};
std::vector<HiddenWindow> favoriteWindows = {};
ProgramVariable pv = {0};
LPCWSTR RegisterNewClass(LPCWSTR className, WNDPROC wndproc) {
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpszClassName = className;
	wc.hInstance = pv.hInstance;
	wc.lpfnWndProc = wndproc;
	wc.hbrBackground = nullptr;
	wc.hCursor = (HCURSOR)LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	RegisterClassW(&wc);
	return wc.lpszClassName;
}
bool isRunAsAdmin() {
	BOOL isAdmin = FALSE;
	PSID adminGroup = NULL;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_ADMINS, NULL, NULL, NULL, NULL, NULL, NULL, &adminGroup)) {
		if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
			isAdmin = FALSE;
		}
		FreeSid(adminGroup);
	}
	return isAdmin;
}
void DebugModCheck(wchar_t* lpCmdLine) {
	pv.isDebugMode = false;
	int argsCount;
	wchar_t** cmdArpvLine;
	if (cmdArpvLine = CommandLineToArgvW(lpCmdLine, &argsCount))
		for (int i = 0; i != argsCount; i++)
			if (!lstrcmpW(cmdArpvLine[i], SY_DEBUG_ARG)) {
				pv.isDebugMode = true;
				break;
			}
}
void OpenSettings() {
	if (!pv.settWin)
		pv.settWin = CreateWindowExW(NULL, pv.wc2, pv.isAdminMode ? pv.wc2 : TX_UI_TITLE, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX | WS_VISIBLE,
									 (GetSystemMetrics(SM_CXSCREEN) - CONST_WND_WIDTH) >> 1, (GetSystemMetrics(SM_CYSCREEN) - CONST_WND_HEIGHT) >> 1, CONST_WND_WIDTH, CONST_WND_HEIGHT, 0, 0, pv.hInstance, 0);
	else SetForegroundWindow(pv.settWin);
}
static HBITMAP IconToBitmap(HICON hIcon) {
	const int width = 16;
	const int height = 16;
	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
	HBRUSH hBrush = pv.isDark ? CreateSolidBrush(ID_CLR_TRAYGRAY) : CreateSolidBrush(ID_CLR_TRAYLIGHT);
	RECT rect = {0, 0, width, height};
	FillRect(hdcMem, &rect, hBrush);
	DeleteObject(hBrush);
	DrawIconEx(hdcMem, 0, 0, hIcon, width, height, 0, 0, DI_NORMAL);
	SelectObject(hdcMem, hOldBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
	return hBitmap;
}
void UpdateTrayMenu(bool isDebug) {
	if (pv.hMenu) DestroyMenu(pv.hMenu);
	pv.hMenu = CreatePopupMenu();
	for (auto it = hiddenWindows.begin(); it != hiddenWindows.end();) {
		if (!GetWindowThreadProcessId(it->hwnd, NULL)) {
			it = hiddenWindows.erase(it);
			continue;
		}
		UINT id = static_cast<UINT>(1000) + static_cast<UINT>(std::distance(hiddenWindows.begin(), it));
		std::wstring menuText = (pv.isDebugMode || isDebug) ? it->processName.c_str() : TruncateWithEllipsis(it->windowTitle);
		AppendMenuW(pv.hMenu, MF_STRING, id, menuText.c_str());
		HBITMAP hBitmap = IconToBitmap(it->hIcon);
		if (hBitmap) SetMenuItemBitmaps(pv.hMenu, id, MF_BYCOMMAND, hBitmap, hBitmap);
		++it;
	}
	if (!hiddenWindows.empty()) AppendMenuW(pv.hMenu, MF_SEPARATOR, NULL, NULL);
	if (pv.isAdminMode) AppendMenuW(pv.hMenu, MF_STRING | MF_DISABLED, ID_TRAY_RESTART, pv.hk.nameArr.c_str());
	else AppendMenuW(pv.hMenu, MF_STRING, ID_TRAY_RESTART, TX_ADMIN_RESTART);
	AppendMenuW(pv.hMenu, MF_STRING, ID_TRAY_SETTINGS, TX_TRAY_BTN_SETTINGS);
	AppendMenuW(pv.hMenu, MF_STRING, ID_TRAY_EXIT, TX_TRAY_BTN_EXIT);
}
void CloseApp() {
	for (const auto& hiddenWindow : hiddenWindows) {
		ShowWindow(hiddenWindow.hwnd, SW_SHOW);
	}
	hiddenWindows.clear();
	if (pv.settWin) {
		DestroyWindow(pv.settWin);
	}
	PostQuitMessage(0);
}
static std::wstring GetProcessCommandLine(DWORD processID) {
	HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres)) {
		LogAdd(ET_COM_INIT);
		return{};
	}
	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
								RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres)) {
		CoUninitialize();
		LogAdd(ET_COM_SECURITY);
		return{};
	}

	IWbemLocator* pLoc = NULL;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
	if (FAILED(hres)) {
		CoUninitialize();
		LogAdd(ET_WMI_CREATE);
		return{};
	}

	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
	if (FAILED(hres)) {
		pLoc->Release();
		CoUninitialize();
		LogAdd(ET_WMI_CONNECT);
		return{};
	}

	hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, 0, 0, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE);
	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		LogAdd(ET_WMI_SECURITY);
		return{};
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	std::wstring query = L"SELECT CommandLine FROM Win32_Process WHERE ProcessId = " + std::to_wstring(processID);

	hres = pSvc->ExecQuery(_bstr_t("WQL"), _bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		LogAdd(ET_WMI_QUERY);
		return{};
	}

	IWbemClassObject* pClsObj = NULL;
	ULONG uReturn = 0;
	VARIANT vtProp{};

	std::wstring commandLine = {};
	if (pEnumerator->Next(WBEM_INFINITE, 1, &pClsObj, &uReturn) == S_OK) {
		hres = pClsObj->Get(SY_COMMAND_LINE, 0, &vtProp, 0, 0);
		if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
			commandLine = vtProp.bstrVal;
		} else {
			LogAdd(ET_GET_CMDLINE);
			commandLine = {};
		}
		VariantClear(&vtProp);
		pClsObj->Release();
	} else {
		commandLine = {};
	}

	pEnumerator->Release();
	pSvc->Release();
	pLoc->Release();
	CoUninitialize();

	return commandLine;
}
int GetMaxTextWidth(HWND hwndListBox) {
	HDC hdc = GetDC(hwndListBox);
	HFONT hOldFont = (HFONT)SelectObject(hdc, pv.hFont);
	int count = (int)SendMessageW(hwndListBox, LB_GETCOUNT, 0, 0);
	SIZE sz = { 0 };
	int maxWidth = 0;
	for (int i = 0; i < count; ++i) {
		wchar_t buffer[1024]{};
		SendMessageW(hwndListBox, LB_GETTEXT, i, (LPARAM)buffer);
		GetTextExtentPoint32W(hdc, buffer, lstrlenW(buffer), &sz);
		if (sz.cx > maxWidth)
			maxWidth = sz.cx;
	}
	SelectObject(hdc, hOldFont);
	ReleaseDC(hwndListBox, hdc);
	return maxWidth;
}
static BOOL CALLBACK EnumWindowsUpdateAppListProc(HWND hwnd, LPARAM lParam) {
	HWND hApplicationsList = (HWND)lParam;
	wchar_t windowTitle[MAX_PATH] = {0};

	if (IsWindowVisible(hwnd) && GetWindowTextW(hwnd, windowTitle, SIZEOF(windowTitle)) > 0) {
		std::wstring className = GetWstringClassName(hwnd);
		if (className.empty())
			return true;
		DWORD processID = GetProcessId(hwnd);
		std::wstring processName = GetAllowedProcessName(processID);
		if (processName.empty())
			return true;

		int count = (int)SendMessageW(pv.hFavoritesList, LB_GETCOUNT, NULL, NULL);
		for (int i = 0; i < count; i++) {
			HiddenWindow* hw = reinterpret_cast<HiddenWindow*>(SendMessageW(pv.hFavoritesList, LB_GETITEMDATA, i, NULL));
			if (hw->processName == processName && hw->hwnd == hwnd)
				return true;
		}

		HiddenWindow* HW = new HiddenWindow;
		HW->hwnd = hwnd;
		HW->windowTitle = windowTitle;
		HW->className = className;
		HW->processID = processID;
		HW->processName = processName;
		HW->isFavorite = false;
		int index = (int)SendMessageW(hApplicationsList, LB_ADDSTRING, NULL, (LPARAM)windowTitle);
		SendMessageW(hApplicationsList, LB_SETITEMDATA, index, (LPARAM)HW);
	}
	return true;
}
void UpdateApplicationsList() {
	if (!pv.hApplicationsList)
		return;
	DeleteList(pv.hApplicationsList);
	EnumWindows(EnumWindowsUpdateAppListProc, (LPARAM)pv.hApplicationsList);

	SendMessageW(pv.hApplicationsList, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(GetMaxTextWidth(pv.hApplicationsList)) + 10, 0);
}
void UpdateFavoriteList() {
	if (!pv.hFavoritesList)
		return;
	DeleteList(pv.hFavoritesList);
	for (const auto& hw : favoriteWindows) {
		HiddenWindow* HW = new HiddenWindow;
		*HW = hw;
		int index = (int)SendMessageW(pv.hFavoritesList, LB_ADDSTRING, NULL, (LPARAM)hw.windowTitle.c_str());
		SendMessageW(pv.hFavoritesList, LB_SETITEMDATA, index, (LPARAM)HW);
	}
	SendMessageW(pv.hFavoritesList, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(GetMaxTextWidth(pv.hFavoritesList)) + 10, 0);
}
static HICON GetIcon(HWND hwnd) {
	HICON hIcon = (HICON)SendMessageW(hwnd, WM_GETICON, ICON_SMALL, 0);
	if (!hIcon) {
		hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
		if (!hIcon) {
			hIcon = LoadIconW(NULL, IDI_APPLICATION);
		}
	}
	return hIcon;
}
std::wstring GetWindowTitle(HWND hwnd) {
	wchar_t windowTitle[MAX_PATH];
	ZeroMemory(&windowTitle, sizeof(windowTitle));
	GetWindowTextW(hwnd, windowTitle, SIZEOF(windowTitle));
	return windowTitle;
}
void CollapseToTray(HWND hwnd, HiddenWindow* HW) {
	if (HW == nullptr) {
		DWORD processID = GetProcessId(hwnd);
		std::wstring processName = GetAllowedProcessName(processID);
		if (processName.empty())
			return;
		std::wstring className = GetWstringClassName(hwnd);
		if (className.empty())
			return;
		hiddenWindows.push_back({hwnd, GetIcon(hwnd), processID, false, GetWindowTitle(hwnd), className, processName, GetProcessCommandLine(processID)});
	} else {
		HW->hIcon = GetIcon(hwnd);
		hiddenWindows.push_back(*HW);
	}
	ShowWindow(hwnd, SW_HIDE);
	UpdateTrayMenu(false);
}
void CollapseToTrayFromFavorite() {
	int count = static_cast<int>(SendMessageW(pv.hFavoritesList, LB_GETCOUNT, 0, 0));
	if (count == LB_ERR) return;

	for (int i = 0; i < count; ++i) {
		HiddenWindow* HW = reinterpret_cast<HiddenWindow*>(SendMessageW(pv.hFavoritesList, LB_GETITEMDATA, i, 0));

		if (HW->isFavorite == TRUE && std::none_of(hiddenWindows.begin(), hiddenWindows.end(), [HW](const HiddenWindow& hw) { return hw.hwnd == HW->hwnd; })) {
			std::wstring commandLine = GetProcessCommandLine(HW->processID);
			if (!commandLine.empty()) {
				HW->commandLine = std::move(commandLine);
				if (std::none_of(favoriteWindows.begin(), favoriteWindows.end(), [HW](const HiddenWindow& hw) { return hw.commandLine == HW->commandLine && hw.hwnd == HW->hwnd; })) {
					favoriteWindows.push_back(*HW);
					std::wstring appTittle = HW->windowTitle.c_str();
					LogAdd(IT_FAVORITE_ADDED + appTittle);
				}
				CollapseToTray(HW->hwnd, HW);
			}
		}
	}
}
void DeleteList(HWND list) {
	if (list) {
		int count = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
		for (int i = 0; i < count; ++i) {
			HiddenWindow* HW = reinterpret_cast<HiddenWindow*>(SendMessageW(list, LB_GETITEMDATA, i, 0));
			delete HW;
		}
		SendMessageW(list, LB_RESETCONTENT, 0, 0);
	}
}
std::wstring GetWstringClassName(HWND hwnd) {
	wchar_t classNam[256];
	ZeroMemory(&classNam, sizeof(classNam));
	GetClassNameW(hwnd, classNam, SIZEOF(classNam));
	for (const auto& t : exceptionClassNames) {
		if (lstrcmpW(classNam, t.c_str()) == 0)
			return{};
	}
	return classNam;
}
DWORD GetProcessId(HWND hwnd) {
	DWORD processID;
	GetWindowThreadProcessId(hwnd, &processID);
	return processID;
}
std::wstring GetAllowedProcessName(DWORD processID) {
	std::wstring processName = {};
	wchar_t processNam[MAX_PATH] = {0};
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (hProcess) {
		HMODULE hMod;
		DWORD cbNeeded;
		if (K32EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
			K32GetModuleBaseNameW(hProcess, hMod, processNam, SIZEOF(processNam));
		processName = processNam;
		CloseHandle(hProcess);
	}
	for (const auto& t : exceptionProcessNames) {
		if (t == processName)
			return{};
	}
	return processName;
}
void CheckFolderAndFile(const std::wstring& fileName) {
	PWSTR appDataPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
		MBATTENTION(ET_GET_APPDATA);
		return;
	}
	std::wstring folderPath = std::wstring(appDataPath) + SY_FOLDER_NAME;
	if (!CreateDirectoryW(folderPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
		MBATTENTION(ET_CREATE_FOLDER);
		return;
	}
	std::wstring filePath = folderPath + fileName;
	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, NULL, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (GetLastError() != ERROR_FILE_EXISTS) {
			MBATTENTION(ET_CREATE_FILE);
		}
	} else {
		CloseHandle(hFile);
	}
}
std::wstring ReadSettingsFile() {
	PWSTR appDataPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
		MBATTENTION(ET_GET_APPDATA);
		return{};
	}
	std::wstring filePath = std::wstring(appDataPath) + SY_FOLDER_NAME + SY_SETTINGS_FILENAME;
	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MBATTENTION(ET_OPEN_FILE);
		return{};
	}
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize)) {
		MBATTENTION(ET_FILE_SIZE);
		CloseHandle(hFile);
		return{};
	}
	std::wstring content(fileSize.QuadPart, L'\0');
	DWORD bytesRead;
	if (!ReadFile(hFile, &content[0], (DWORD)fileSize.QuadPart, &bytesRead, NULL) || bytesRead != fileSize.QuadPart) {
		MBATTENTION(ET_FILE_READ);
		CloseHandle(hFile);
		return{};
	}
	CloseHandle(hFile);
	return content;
}
void WriteMyFile(std::wstring fileName, const std::wstring& content, bool isAdd) {
	PWSTR appDataPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
		MBATTENTION(ET_GET_APPDATA);
		return;
	}
	std::wstring filePath = std::wstring(appDataPath) + SY_FOLDER_NAME + fileName;
	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, isAdd ? OPEN_EXISTING : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MBATTENTION(ET_OPEN_FILE);
		return;
	}
	if (isAdd) SetFilePointer(hFile, 0, NULL, FILE_END);
	DWORD bytesWritten;
	if (!WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.length() * sizeof(wchar_t)), &bytesWritten, NULL))
		MBATTENTION(ET_FILE_WRITE);
	CloseHandle(hFile);
}
std::wstring serializeToWstring(const HiddenWindow& hw) {
	std::wostringstream oss;
	oss << hw.windowTitle << L'|'
		<< hw.className << L'|'
		<< hw.processName << L'|'
		<< hw.commandLine << L'\n';
	return oss.str();
}
void SerchWindow(HiddenWindow& window) {
	HWND hwnd = FindWindow(window.className.c_str(), window.windowTitle.c_str());
	DWORD processId = GetProcessId(hwnd);
	std::wstring processName = GetAllowedProcessName(processId);
	if (processName == window.processName) {
		std::wstring commandLine = GetProcessCommandLine(processId);
		if (commandLine == window.commandLine) {
			window.hwnd = hwnd;
			window.processID = processId;
		}
	}
}
void FindWindowFromFile(HiddenWindow& windowToFind, bool isFromFile) {
	if (isFromFile) windowToFind.isFavorite = ID_WND_SAVED_FAVORITES;
	SerchWindow(windowToFind);
	if (windowToFind.hwnd && windowToFind.isFavorite == ID_WND_SAVED_FAVORITES) {
		windowToFind.isFavorite = true;
		CollapseToTray(windowToFind.hwnd, &windowToFind);
	}
}
static HiddenWindow deserializeOne(const std::wstring& str) {
	std::wistringstream iss(str);
	HiddenWindow s{};
	std::wstring token;
	std::getline(iss, token, L'|');
	s.windowTitle = token;
	std::getline(iss, token, L'|');
	s.className = token;
	std::getline(iss, token, L'|');
	s.processName = token;
	std::getline(iss, token, L'\n');
	s.commandLine = token;
	FindWindowFromFile(s, true);
	return s;
}
void deserializeFromWstring(const std::wstring& str) {
	std::vector<HiddenWindow> result;
	std::wistringstream iss(str);
	std::wstring line;

	while (std::getline(iss, line)) {
		if (*line.c_str() != L'\0') {
			result.push_back(deserializeOne(line));
		}
	}
	favoriteWindows = result;
}
void RestartWithAdminRights() {
	wchar_t szPath[MAX_PATH];
	GetModuleFileNameW(NULL, szPath, MAX_PATH);
	SHELLEXECUTEINFO sei = {0};
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd = NULL;
	sei.lpVerb = L"runas";
	sei.lpFile = szPath;
	sei.nShow = SW_SHOWNORMAL;
	ShellExecuteExW(&sei);
}
std::wstring GetCurrentDate() {
	std::wstringstream wss;
	std::time_t t = std::time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);
	wss << (tm.tm_year + 1900) << L'-'
		<< (tm.tm_mon + 1) << L'-'
		<< tm.tm_mday;
	return wss.str();
}
void CheckAndDeleteOldLogs(const std::wstring& currentLogFile) {
	PWSTR appDataPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
		MBATTENTION(ET_GET_APPDATA);
		return;
	}
	std::wstring folderPath = std::wstring(appDataPath) + SY_FOLDER_NAME;
	std::vector<std::pair<std::wstring, FILETIME>> logFiles;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFileW((folderPath + L"\\*" + SY_FILE_EXTENSION).c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && findFileData.cFileName != currentLogFile)
			logFiles.push_back({findFileData.cFileName, findFileData.ftLastWriteTime});
	} while (FindNextFileW(hFind, &findFileData) != 0);
	FindClose(hFind);
	if (logFiles.size() > 7) {
		auto oldestFile = std::min_element(logFiles.begin(), logFiles.end(), [](const auto& a, const auto& b) {return CompareFileTime(&a.second, &b.second) < 0; });
		if (oldestFile != logFiles.end())
			DeleteFileW((folderPath + L"\\" + oldestFile->first).c_str());
	}
}
static std::wstring GetTime() {
	std::wstringstream wss;
	std::time_t t = std::time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);
	wss << std::setw(2) << std::setfill(L'0') << tm.tm_hour << L':'
		<< std::setw(2) << std::setfill(L'0') << tm.tm_min << L':'
		<< std::setw(2) << std::setfill(L'0') << tm.tm_sec;
	return wss.str();
}
#ifdef DEBUG
void LogAdd(std::wstring&& content) {
	OutputDebugStringW((content + L"\n").c_str());
}
#else
void LogAdd(std::wstring&& content) {
	std::wstring currentLogFile = L"\\" + GetCurrentDate() + SY_FILE_EXTENSION;
	CheckFolderAndFile(currentLogFile);
	WriteMyFile(currentLogFile, L"[" + GetTime() + L"]\t" + content + L"\n", true);
}
#endif // DEBUG
bool InitializeTaskService(CComPtr<ITaskService>& pService) {
	HRESULT hr = pService.CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, ET_TASKSERVICE_CREATE, static_cast<UINT>(hr));
		LogAdd(buf);
		return false;
	}

	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, ET_TASKSERVICE_CONNECT, static_cast<UINT>(hr));
		LogAdd(buf);
		return false;
	}
	return true;
}
bool GetRootFolder(CComPtr<ITaskService>& pService, CComPtr<ITaskFolder>& pRootFolder) {
	HRESULT hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, ET_TASK_ROOT_ACCESS, static_cast<UINT>(hr));
		LogAdd(buf);
		return false;
	}
	return true;
}
bool IsTaskScheduled(const std::wstring& taskName) {
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, ET_COINIT_EX, static_cast<UINT>(hr));
		LogAdd(buf);
		return false;
	}

	CComPtr<ITaskService> pService;
	if (!InitializeTaskService(pService)) {
		LogAdd(ET_INITIALIZE_TASK);
		CoUninitialize();
		return false;
	}

	CComPtr<ITaskFolder> pRootFolder;
	if (!GetRootFolder(pService, pRootFolder)) {
		LogAdd(ET_GET_ROOT_FOLDER);
		CoUninitialize();
		return false;
	}

	if (!pRootFolder) {
		LogAdd(ET_ROOTFOLDER_NULL);
		CoUninitialize();
		return false;
	}

	CComPtr<IRegisteredTask> pTask = nullptr;
	hr = pRootFolder->GetTask(_bstr_t(taskName.c_str()), &pTask);

	bool result = SUCCEEDED(hr) && pTask != nullptr;
	pTask.Release();
	pRootFolder.Release();
	pService.Release();
	CoUninitialize();
	return result;
}
void DeleteScheduledTask(CComPtr<ITaskService>& pService, const std::wstring& taskName) {
	CComPtr<ITaskFolder> pRootFolder;
	if (!GetRootFolder(pService, pRootFolder)) {
		return;
	}
	HRESULT hr = pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, ET_TASK_DELETE, static_cast<UINT>(hr));
		LogAdd(buf);
	} else {
		LogAdd(IT_TASK_DELETED);
	}
}
void CreateScheduledTask(CComPtr<ITaskService>& pService, const std::wstring& taskName, const std::wstring& exePath) {
	CComPtr<ITaskFolder> pRootFolder;
	if (!GetRootFolder(pService, pRootFolder)) {
		return;
	}
	pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);

	CComPtr<ITaskDefinition> pTask;
	HRESULT hr = pService->NewTask(0, &pTask);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, ET_TASK_CREATE, static_cast<UINT>(hr));
		LogAdd(buf);
		return;
	}

	CComPtr<IRegistrationInfo> pRegInfo;
	pTask->get_RegistrationInfo(&pRegInfo);
	pRegInfo->put_Author(const_cast<BSTR>(L"Admin"));

	CComPtr<IPrincipal> pPrincipal;
	pTask->get_Principal(&pPrincipal);
	pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
	pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);

	CComPtr<ITriggerCollection> pTriggerCollection;
	pTask->get_Triggers(&pTriggerCollection);
	CComPtr<ITrigger> pTrigger;
	pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);

	CComPtr<IActionCollection> pActionCollection;
	pTask->get_Actions(&pActionCollection);
	CComPtr<IAction> pAction;
	pActionCollection->Create(TASK_ACTION_EXEC, &pAction);

	CComPtr<IExecAction> pExecAction;
	pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
	pExecAction->put_Path(_bstr_t(exePath.c_str()));

	CComPtr<ITaskSettings> pSettings;
	pTask->get_Settings(&pSettings);
	pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
	pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
	pSettings->put_AllowHardTerminate(VARIANT_FALSE);
	pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S"));

	CComPtr<IRegisteredTask> pRegisteredTask;
	hr = pRootFolder->RegisterTaskDefinition(
		_bstr_t(taskName.c_str()), pTask, TASK_CREATE_OR_UPDATE,
		_variant_t(L""), _variant_t(L""), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &pRegisteredTask
	);

	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, ET_TASK_REGISTER, static_cast<UINT>(hr));
		LogAdd(buf);
	} else {
		LogAdd(IT_TASK_REGISTERED);
	}
}
void StartupChanging(bool isAdd) {
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, ET_COINIT_EX, static_cast<UINT>(hr));
		LogAdd(buf);
		return;
	}

	CComPtr<ITaskService> pService;
	if (!InitializeTaskService(pService)) {
		CoUninitialize();
		return;
	}

	wchar_t szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	std::wstring appPath = szPath;
	if (isAdd) {
		CreateScheduledTask(pService, SY_APP_NAME, appPath);
	} else {
		DeleteScheduledTask(pService, SY_APP_NAME);
	}

	pService.Release();
	CoUninitialize();
}
void CreateHKSettWnd() {
	if (!pv.settHK) {
		RECT rect;
		GetWindowRect(pv.settWin, &rect);
		int cx = 600;
		int cy = 300;
		int x = rect.left + ((rect.right - rect.left) >> 1) - (cx >> 1);
		int y = rect.top + ((rect.bottom - rect.top) >> 1) - (cy >> 1);
		pv.settHK = CreateWindowExW(NULL, pv.wc3, TX_PRESS_HOTKEY, WS_SYSMENU | WS_POPUP | WS_VISIBLE, x, y, cx, cy, pv.settWin, NULL, pv.hInstance, NULL);
		//SetWindowLongPtr(pv.settHK, GWLP_HWNDPARENT, (LONG_PTR)pv.settWin);
		SetForegroundWindow(pv.settHK);
	}
}
template<typename T>
static bool WriteRegistryValue(HKEY root, const wchar_t* subkey, const wchar_t* valueName, DWORD type, const T& data) {
	HKEY hKey;
	if (RegCreateKeyExW(root, subkey, 0, 0, 0, KEY_WRITE, 0, &hKey, 0) != ERROR_SUCCESS)
		return false;

	bool result = RegSetValueExW(hKey, valueName, 0, type, reinterpret_cast<const BYTE*>(&data), sizeof(T)) == ERROR_SUCCESS;
	RegCloseKey(hKey);
	return result;
}
template<typename T>
static bool ReadRegistryValue(HKEY root, const wchar_t* subkey, const wchar_t* valueName, DWORD expectedType, T& out, T fallback, bool writeFallback = false) {
	HKEY hKey;
	DWORD size = sizeof(T);
	DWORD type = 0;
	if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		out = fallback;
		return false;
	}

	LONG status = RegQueryValueExW(hKey, valueName, NULL, &type, reinterpret_cast<BYTE*>(&out), &size);
	RegCloseKey(hKey);

	if (status != ERROR_SUCCESS || type != expectedType || size != sizeof(T)) {
		out = fallback;
		if (writeFallback)
			WriteRegistryValue(root, subkey, valueName, expectedType, fallback);
		return false;
	}

	return true;
}
void LoadNumberFromRegistry() {
	bool a = !ReadRegistryValue(HKEY_CURRENT_USER, SY_REGISTRY_PATH, SY_REG_KEY_TIMER_FLAG, REG_BINARY, pv.isHideOn, true, true);
	bool b = !ReadRegistryValue(HKEY_CURRENT_USER, SY_REGISTRY_PATH, SY_REG_KEY_TIMER_VAL, REG_DWORD, pv.timerToHide, CONST_AUTOHIDE_DELAY_MS, true);

	if (a)
		LogAdd(IT_NO_KEY1 + std::to_wstring(pv.isHideOn));
	if (b)
		LogAdd(IT_NO_KEY2 + std::to_wstring(pv.timerToHide));
}
void SaveToRegistry(bool writeHideOn, bool writeTimer) {
	if (writeHideOn) {
		WriteRegistryValue(HKEY_CURRENT_USER, SY_REGISTRY_PATH, SY_REG_KEY_TIMER_FLAG, REG_BINARY, pv.isHideOn);
		LogAdd(std::wstring(IT_TIMER) + (pv.isHideOn ? IT_TIMER_ON : IT_TIMER_OFF));
	}
	if (writeTimer) {
		WriteRegistryValue(HKEY_CURRENT_USER, SY_REGISTRY_PATH, SY_REG_KEY_TIMER_VAL, REG_DWORD, pv.timerToHide);
		LogAdd(IT_TIMER_VALUE_SET + std::to_wstring(pv.timerToHide));
	}
}
void SetZeroModKeysState() {
	BYTE keyState[256];
	if (GetKeyboardState(keyState)) {}
	keyState[VK_MENU] = 0;
	keyState[VK_CONTROL] = 0;
	keyState[VK_SHIFT] = 0;
	keyState[92] = 0;
	keyState[91] = 0;
	SetKeyboardState(keyState);
}
void SetZeroModKeysState(BYTE* keyState) {
	keyState[VK_MENU] = 0;
	keyState[VK_CONTROL] = 0;
	keyState[VK_SHIFT] = 0;
	keyState[92] = 0;
	keyState[91] = 0;
}
void RegHotKey(UINT mod, UINT other, int id) {
	UnregisterHotKey(pv.trayWnd, id);
	RegisterHotKey(pv.trayWnd, id, mod, other);
	SaveHotKeys(mod, other);
}
std::wstring convertKeysToWstring(UINT modKeys, UINT otherKey) {
	std::wstring result;//
	if (modKeys) {
		static const std::wstring sysVk[] = {L"Ctrl", L"Win", L"Alt", L"Shift"};
		static const byte MODKEYS[] = {MOD_CONTROL, MOD_WIN, MOD_ALT, MOD_SHIFT};
		for (int i = 0; i < sizeof(sysVk) / sizeof(sysVk[0]); i++)
			if (modKeys & MODKEYS[i]) {
				if (!result.empty()) result += L" + ";
				result += sysVk[i];
			}
	}
	if (otherKey) {
		static std::map<UINT, std::wstring> keyNames = {
		{VK_SPACE, L"Space"}, {VK_RETURN, L"Enter"},
		{VK_TAB, L"Tab"}, {VK_BACK, L"BkSp"},
		{VK_ESCAPE, L"Esc"}, {VK_MENU, L"Alt"},
		{VK_PAUSE, L"Pause"}, {VK_CAPITAL, L"CapsLk"},
		{VK_NUMLOCK, L"NumLk"}, {VK_SCROLL, L"ScrLk"},
		{VK_INSERT, L"Ins"}, {VK_SNAPSHOT, L"PrtSc"},
		{VK_DELETE, L"Del"}, {VK_HOME, L"Home"},
		{VK_END, L"End"}, {VK_PRIOR, L"PgUp"},
		{VK_NEXT, L"PgDn"}, {VK_UP, L"Up"},
		{VK_DOWN, L"Down"},	{VK_LEFT, L"Left"},
		{VK_RIGHT, L"Right"}, {VK_F1, L"F1"},
		{VK_F2, L"F2"},	{VK_F3, L"F3"},
		{VK_F4, L"F4"},	{VK_F5, L"F5"},
		{VK_F6, L"F6"},	{VK_F7, L"F7"},
		{VK_F8, L"F8"},	{VK_F9, L"F9"},
		{VK_F10, L"F10"}, {VK_F11, L"F11"},
		{VK_F12, L"F12"}, {VK_F13, L"F13"},
		{VK_F14, L"F14"}, {VK_F15, L"F15"},
		{VK_F16, L"F16"}, {VK_F17, L"F17"},
		{VK_F18, L"F18"}, {VK_F19, L"F19"},
		{VK_F20, L"F20"}, {VK_F21, L"F21"},
		{VK_F22, L"F22"}, {VK_F23, L"F23"},
		{VK_F24, L"F24"}, {VK_VOLUME_MUTE, L"Mute"},
		{VK_VOLUME_DOWN, L"VolDn"}, {VK_VOLUME_UP, L"VolUp"},
		{VK_MEDIA_NEXT_TRACK, L"Next"}, {VK_MEDIA_PREV_TRACK, L"Prev"},
		{VK_MEDIA_PLAY_PAUSE, L"Play"},
		};
		if (modKeys) result += L" + ";

		if (keyNames.find(otherKey) != keyNames.end()) {
			result += keyNames[otherKey];
			return result;
		}

		BYTE keyState[256];
		ZeroMemory(keyState, sizeof(keyState));
		if (GetKeyboardState(keyState)) {}
		keyState[20] = 1;
		SetZeroModKeysState(keyState);
		wchar_t buffer[5] = {0};
		int len = ToUnicode(otherKey, MapVirtualKeyW(otherKey, MAPVK_VK_TO_VSC), keyState, buffer, 5, 0);
		if (len == 1) {
			result += buffer;
			return result;
		}
		result += L"NULL";
		pv.hk.canSet = 0;
	}
	if (result.empty())
		result = TX_PRESS_KEYS;
	return result;
}
void SaveHotKeys(byte mod, byte other) {
	WriteRegistryValue(HKEY_CURRENT_USER, SY_REGISTRY_PATH, SY_REG_KEY_MODKEYS, REG_DWORD, mod);
	WriteRegistryValue(HKEY_CURRENT_USER, SY_REGISTRY_PATH, SY_REG_KEY_VKEYS, REG_DWORD, other);
	LogAdd(std::wstring(IT_NEW_HOTKEY) + pv.hk.nameArr);
}
void ReadHotKeys() {
	const byte defMod = MOD_CONTROL | MOD_ALT;
	const byte defKey = 'H';

	bool a = !ReadRegistryValue(HKEY_CURRENT_USER, SY_REGISTRY_PATH, SY_REG_KEY_MODKEYS, REG_DWORD, pv.hk.modKey, defMod, true);
	bool b = !ReadRegistryValue(HKEY_CURRENT_USER, SY_REGISTRY_PATH, SY_REG_KEY_VKEYS, REG_DWORD, pv.hk.otherKey, defKey, true);

	if (a)
		LogAdd(IT_NO_KEY1 + std::to_wstring(pv.hk.modKey));
	if (b)
		LogAdd(IT_NO_KEY2 + std::to_wstring(pv.hk.otherKey));

	pv.hk.nameArr = convertKeysToWstring(pv.hk.modKey, pv.hk.otherKey);
}
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
	nid.hIcon = LoadIconW(pv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	if (!nid.hIcon) LogAdd(ET_ICON_LOAD);
	wcscpy_s(nid.szTip, TX_TRAY_TOOLTIP);
	if (Shell_NotifyIconW(NIM_ADD, &nid)) {
		LogAdd(IT_ICON_CREATED);
	} else if (!count) {
		LogAdd(IT_TRAY_WAIT_RETRY);
		count = 1;
	} else {
		DWORD err = GetLastError();
		wchar_t buf[64];
		swprintf_s(buf, ET_ICON_CREATE, static_cast<UINT>(err));
		LogAdd(buf);
	}
}
void RemoveTrayIcon(HWND hwnd) {
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = 1;
	if (Shell_NotifyIconW(NIM_DELETE, &nid)) {
		LogAdd(IT_ICON_DELETED);
	} else {
		DWORD err = GetLastError();
		wchar_t buf[64];
		swprintf_s(buf, ET_ICON_DELETE, static_cast<UINT>(err));
		LogAdd(buf);
	}
}
std::wstring TruncateWithEllipsis(const std::wstring& text, size_t maxLen) {
	if (text.size() <= maxLen)
		return text;
	return text.substr(0, maxLen - 3) + L"...";
}