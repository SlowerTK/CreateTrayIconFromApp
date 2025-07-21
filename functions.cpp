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
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	RegisterClass(&wc);
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
			if (!lstrcmpW(cmdArpvLine[i], DEBUG_STRING)) {
				pv.isDebugMode = true;
				break;
			}
}
void OpenSettings() {
	if (!pv.settWin)
		pv.settWin = CreateWindowExW(NULL, pv.wc2, pv.isAdminMode ? pv.wc2 : WND_TITLE, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX | WS_VISIBLE,
									 (GetSystemMetrics(SM_CXSCREEN) - wndX) >> 1, (GetSystemMetrics(SM_CYSCREEN) - wndY) >> 1, wndX, wndY, 0, 0, pv.hInstance, 0);
	else SetForegroundWindow(pv.settWin);
}
static HBITMAP IconToBitmap(HICON hIcon) {
	const int width = 16;
	const int height = 16;
	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
	HBRUSH hBrush = CreateSolidBrush({0xf9f9f9});
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
		AppendMenu(pv.hMenu, MF_STRING, id, (pv.isDebugMode || isDebug) ? it->processName.c_str() : it->windowTitle.c_str());
		HBITMAP hBitmap = IconToBitmap(it->hIcon);
		if (hBitmap) SetMenuItemBitmaps(pv.hMenu, id, MF_BYCOMMAND, hBitmap, hBitmap);
		++it;
	}
	if (!hiddenWindows.empty()) AppendMenu(pv.hMenu, MF_SEPARATOR, NULL, NULL);
	if (pv.isAdminMode) AppendMenu(pv.hMenu, MF_STRING | MF_DISABLED, TB_RESTART, TB_HOTKEY_TEXT);
	else AppendMenu(pv.hMenu, MF_STRING, TB_RESTART, NEED_ADMIN_RIGHTS);
	AppendMenu(pv.hMenu, MF_STRING, TB_SETTINGS, TB_SETTINGS_TEXT);
	AppendMenu(pv.hMenu, MF_STRING, TB_EXIT, TB_EXIT_TEXT);
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
		LogAdd(L"Ошибка инициализации COM");
		return L"";
	}
	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
								RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres)) {
		CoUninitialize();
		LogAdd(L"Ошибка настройки безопасности COM");
		return L"";
	}

	IWbemLocator* pLoc = NULL;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
	if (FAILED(hres)) {
		CoUninitialize();
		LogAdd(L"Не удалось создать WMI-локатор");
		return L"";
	}

	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
	if (FAILED(hres)) {
		pLoc->Release();
		CoUninitialize();
		LogAdd(L"Не удалось подключиться к WMI");
		return L"";
	}

	hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		LogAdd(L"Не удалось установить параметры безопасности");
		return L"";
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	std::wstring query = QUERY + std::to_wstring(processID);

	hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		LogAdd(L"Не удалось выполнить запрос WMI");
		return L"";
	}

	IWbemClassObject* pClsObj = NULL;
	ULONG uReturn = 0;
	VARIANT vtProp{};

	std::wstring commandLine = L"";
	if (pEnumerator->Next(WBEM_INFINITE, 1, &pClsObj, &uReturn) == S_OK) {
		hres = pClsObj->Get(L"CommandLine", 0, &vtProp, 0, 0);
		if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
			commandLine = vtProp.bstrVal;
		} else {
			LogAdd(ET_COMMANDLINE);
			commandLine = L"";
		}
		VariantClear(&vtProp);
		pClsObj->Release();
	} else {
		commandLine = L"";
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
	int count = (int)SendMessage(hwndListBox, LB_GETCOUNT, 0, 0);
	SIZE sz = { 0 };
	int maxWidth = 0;
	for (int i = 0; i < count; ++i) {
		wchar_t buffer[1024]{};
		SendMessage(hwndListBox, LB_GETTEXT, i, (LPARAM)buffer);
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

		int count = (int)SendMessage(pv.hFavoritesList, LB_GETCOUNT, NULL, NULL);
		for (int i = 0; i < count; i++) {
			HiddenWindow* hw = reinterpret_cast<HiddenWindow*>(SendMessage(pv.hFavoritesList, LB_GETITEMDATA, i, NULL));
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
		int index = (int)SendMessage(hApplicationsList, LB_ADDSTRING, NULL, (LPARAM)windowTitle);
		SendMessage(hApplicationsList, LB_SETITEMDATA, index, (LPARAM)HW);
	}
	return true;
}
void UpdateApplicationsList() {
	if (!pv.hApplicationsList)
		return;
	DeleteList(pv.hApplicationsList);
	EnumWindows(EnumWindowsUpdateAppListProc, (LPARAM)pv.hApplicationsList);

	SendMessage(pv.hApplicationsList, LB_SETHORIZONTALEXTENT, GetMaxTextWidth(pv.hApplicationsList) + 10, 0);
}
void UpdateFavoriteList() {
	if (!pv.hFavoritesList)
		return;
	DeleteList(pv.hFavoritesList);
	for (const auto& hw : favoriteWindows) {
		HiddenWindow* HW = new HiddenWindow;
		*HW = hw;
		int index = (int)SendMessage(pv.hFavoritesList, LB_ADDSTRING, NULL, (LPARAM)hw.windowTitle.c_str());
		SendMessage(pv.hFavoritesList, LB_SETITEMDATA, index, (LPARAM)HW);
	}
	SendMessage(pv.hFavoritesList, LB_SETHORIZONTALEXTENT, GetMaxTextWidth(pv.hFavoritesList) + 10, 0);
}
static HICON GetIcon(HWND hwnd) {
	HICON hIcon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0);
	if (!hIcon) {
		hIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM);
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
	int count = static_cast<int>(SendMessage(pv.hFavoritesList, LB_GETCOUNT, 0, 0));
	if (count == LB_ERR) return;

	for (int i = 0; i < count; ++i) {
		HiddenWindow* HW = reinterpret_cast<HiddenWindow*>(SendMessage(pv.hFavoritesList, LB_GETITEMDATA, i, 0));

		if (HW->isFavorite == TRUE && std::none_of(hiddenWindows.begin(), hiddenWindows.end(), [HW](const HiddenWindow& hw) { return hw.hwnd == HW->hwnd; })) {
			std::wstring commandLine = GetProcessCommandLine(HW->processID);
			if (!commandLine.empty()) {
				HW->commandLine = std::move(commandLine);
				if (std::none_of(favoriteWindows.begin(), favoriteWindows.end(), [HW](const HiddenWindow& hw) { return hw.commandLine == HW->commandLine && hw.hwnd == HW->hwnd; })) {
					favoriteWindows.push_back(*HW);
					std::wstring appTittle = HW->windowTitle.c_str();
					LogAdd(L"В избранное добавлено " + appTittle);
				}
				CollapseToTray(HW->hwnd, HW);
			}
		}
	}
}
void DeleteList(HWND list) {
	if (list) {
		int count = static_cast<int>(SendMessage(list, LB_GETCOUNT, 0, 0));
		for (int i = 0; i < count; ++i) {
			HiddenWindow* HW = reinterpret_cast<HiddenWindow*>(SendMessage(list, LB_GETITEMDATA, i, 0));
			delete HW;
		}
		SendMessage(list, LB_RESETCONTENT, 0, 0);
	}
}
std::wstring GetWstringClassName(HWND hwnd) {
	wchar_t classNam[256];
	ZeroMemory(&classNam, sizeof(classNam));
	GetClassNameW(hwnd, classNam, SIZEOF(classNam));
	for (const auto& t : exceptionClassNames) {
		if (lstrcmpW(classNam, t.c_str()) == 0)
			return L"";
	}
	return classNam;
}
DWORD GetProcessId(HWND hwnd) {
	DWORD processID;
	GetWindowThreadProcessId(hwnd, &processID);
	return processID;
}
std::wstring GetAllowedProcessName(DWORD processID) {
	std::wstring processName = L"";
	wchar_t processNam[MAX_PATH] = {0};
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (hProcess) {
		HMODULE hMod;
		DWORD cbNeeded;
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
			GetModuleBaseName(hProcess, hMod, processNam, SIZEOF(processNam));
		processName = processNam;
		CloseHandle(hProcess);
	}
	for (const auto& t : exceptionProcessNames) {
		if (t == processName)
			return L"";
	}
	return processName;
}
void CheckFolderAndFile(const std::wstring& fileName) {
	PWSTR appDataPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
		MBATTENTION(ET_APPDATA);
		return;
	}
	std::wstring folderPath = std::wstring(appDataPath) + FOLDERNAME;
	if (!CreateDirectory(folderPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
		MBATTENTION(ET_CREATEFOLDER);
		return;
	}
	std::wstring filePath = folderPath + fileName;
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, NULL, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		if (GetLastError() != ERROR_FILE_EXISTS) {
			MBATTENTION(ET_CREATEFILE);
		}
	} else {
		CloseHandle(hFile);
	}
}
std::wstring ReadSettingsFile() {
	PWSTR appDataPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
		MBATTENTION(ET_APPDATA);
		return L"";
	}
	std::wstring filePath = std::wstring(appDataPath) + FOLDERNAME + FILEPATHNAME;
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MBATTENTION(ET_FILEOPEN);
		return L"";
	}
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize)) {
		MBATTENTION(ET_FILESIZE);
		CloseHandle(hFile);
		return L"";
	}
	std::wstring content(fileSize.QuadPart, L'\0');
	DWORD bytesRead;
	if (!ReadFile(hFile, &content[0], (DWORD)fileSize.QuadPart, &bytesRead, NULL) || bytesRead != fileSize.QuadPart) {
		MBATTENTION(ET_FILECONTENT);
		CloseHandle(hFile);
		return L"";
	}
	CloseHandle(hFile);
	return content;
}
void WriteMyFile(std::wstring fileName, const std::wstring& content, bool isAdd) {
	PWSTR appDataPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
		MBATTENTION(ET_APPDATA);
		return;
	}
	std::wstring filePath = std::wstring(appDataPath) + FOLDERNAME + fileName;
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, isAdd ? OPEN_EXISTING : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MBATTENTION(ET_FILEOPEN);
		return;
	}
	if (isAdd) SetFilePointer(hFile, 0, NULL, FILE_END);
	DWORD bytesWritten;
	if (!WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.length() * sizeof(wchar_t)), &bytesWritten, NULL))
		MBATTENTION(ET_FILEWRITE);
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
	if (isFromFile) windowToFind.isFavorite = SAVED_WINDOW;
	SerchWindow(windowToFind);
	if (windowToFind.hwnd && windowToFind.isFavorite == SAVED_WINDOW) {
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
	GetModuleFileName(NULL, szPath, MAX_PATH);
	SHELLEXECUTEINFO sei = {0};
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd = NULL;
	sei.lpVerb = L"runas";
	sei.lpFile = szPath;
	sei.nShow = SW_SHOWNORMAL;
	ShellExecuteEx(&sei);
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
		MBATTENTION(ET_APPDATA);
		return;
	}
	std::wstring folderPath = std::wstring(appDataPath) + FOLDERNAME;
	std::vector<std::pair<std::wstring, FILETIME>> logFiles;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile((folderPath + L"\\*.log").c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && findFileData.cFileName != currentLogFile)
			logFiles.push_back({findFileData.cFileName, findFileData.ftLastWriteTime});
	} while (FindNextFile(hFind, &findFileData) != 0);
	FindClose(hFind);
	if (logFiles.size() > 7) {
		auto oldestFile = std::min_element(logFiles.begin(), logFiles.end(), [](const auto& a, const auto& b) {return CompareFileTime(&a.second, &b.second) < 0; });
		if (oldestFile != logFiles.end())
			DeleteFile((folderPath + L"\\" + oldestFile->first).c_str());
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
	std::wstring currentLogFile = L"\\" + GetCurrentDate() + L".log";
	CheckFolderAndFile(currentLogFile);
	WriteMyFile(currentLogFile, L"[" + GetTime() + L"]\t" + content + L"\n", true);
}
#endif // DEBUG
bool InitializeTaskService(CComPtr<ITaskService>& pService) {
	HRESULT hr = pService.CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, L"Не удалось создать экземпляр TaskService: 0x%08X", static_cast<UINT>(hr));
		LogAdd(buf);
		return false;
	}

	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, L"Не удалось подключиться к TaskScheduler: 0x%08X", static_cast<UINT>(hr));
		LogAdd(buf);
		return false;
	}
	return true;
}
bool GetRootFolder(CComPtr<ITaskService>& pService, CComPtr<ITaskFolder>& pRootFolder) {
	HRESULT hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, L"Не удалось получить доступ к корневой папке: 0x%08X", static_cast<UINT>(hr));
		LogAdd(buf);
		return false;
	}
	return true;
}
bool IsTaskScheduled(const std::wstring& taskName) {
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, L"Не удалось выполнить CoInitializeEx: 0x%08X", static_cast<UINT>(hr));
		LogAdd(buf);
		return false;
	}

	CComPtr<ITaskService> pService;
	if (!InitializeTaskService(pService)) {
		LogAdd(L"Не удалось выполнить InitializeTaskService");
		CoUninitialize();
		return false;
	}

	CComPtr<ITaskFolder> pRootFolder;
	if (!GetRootFolder(pService, pRootFolder)) {
		LogAdd(L"Не удалось выполнить GetRootFolder");
		CoUninitialize();
		return false;
	}

	if (!pRootFolder) {
		LogAdd(L"pRootFolder == NULL");
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
		swprintf_s(buf, L"Не удалось удалить задачу: 0x%08X", static_cast<UINT>(hr));
		LogAdd(buf);
	} else {
		LogAdd(L"Задача успешно удалена!");
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
		swprintf_s(buf, L"Не удалось создать новую задачу: 0x%08X", static_cast<UINT>(hr));
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
		swprintf_s(buf, L"Не удалось зарегистрировать задачу: 0x%08X", static_cast<UINT>(hr));
		LogAdd(buf);
	} else {
		LogAdd(L"Задача успешно зарегистрирована!");
	}
}
void StartupChanging(bool isAdd) {
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		wchar_t buf[64];
		swprintf_s(buf, L"Не удалось выполнить CoInitializeEx: 0x%08X", static_cast<UINT>(hr));
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
		CreateScheduledTask(pService, appName, appPath);
	} else {
		DeleteScheduledTask(pService, appName);
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
		pv.settHK = CreateWindowExW(NULL, pv.wc3, L"Введите новое сочетание клавиш", WS_SYSMENU | WS_POPUP | WS_VISIBLE, x, y, cx, cy, pv.settWin, NULL, pv.hInstance, NULL);
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
	bool a = !ReadRegistryValue(HKEY_CURRENT_USER, REG_PATH, KEY1, REG_BINARY, pv.isHideOn, true, true);
	bool b = !ReadRegistryValue(HKEY_CURRENT_USER, REG_PATH, KEY2, REG_DWORD, pv.timerToHide, SECOND, true);

	if (a)
		LogAdd(L"Значение KEY1 отсутствует, используем стандартное: " + std::to_wstring(pv.isHideOn));
	if (b)
		LogAdd(L"Значение KEY2 отсутствует, используем стандартное: " + std::to_wstring(pv.timerToHide));
}
void SaveToRegistry(bool writeHideOn, bool writeTimer) {
	if (writeHideOn) {
		WriteRegistryValue(HKEY_CURRENT_USER, REG_PATH, KEY1, REG_BINARY, pv.isHideOn);
		LogAdd(std::wstring(L"Таймер ") + (pv.isHideOn ? L"включён" : L"выключен"));
	}
	if (writeTimer) {
		WriteRegistryValue(HKEY_CURRENT_USER, REG_PATH, KEY2, REG_DWORD, pv.timerToHide);
		LogAdd(L"Записано значение таймера: " + std::to_wstring(pv.timerToHide));
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
		result = PRESSKEYS;
	return result;
}
void SaveHotKeys(byte mod, byte other) {
	WriteRegistryValue(HKEY_CURRENT_USER, REG_PATH, HOTKEYM, REG_DWORD, mod);
	WriteRegistryValue(HKEY_CURRENT_USER, REG_PATH, HOTKEYVK, REG_DWORD, other);
	LogAdd(std::wstring(L"Новое сочетание записано: ") + pv.hk.nameArr);
}
void ReadHotKeys() {
	const byte defMod = MOD_CONTROL | MOD_ALT;
	const byte defKey = 'H';

	bool a = !ReadRegistryValue(HKEY_CURRENT_USER, REG_PATH, HOTKEYM, REG_DWORD, pv.hk.modKey, defMod, true);
	bool b = !ReadRegistryValue(HKEY_CURRENT_USER, REG_PATH, HOTKEYVK, REG_DWORD, pv.hk.otherKey, defKey, true);

	if (a)
		LogAdd(L"Значение HOTKEYM отсутствует, используем стандартное: " + std::to_wstring(pv.hk.modKey));
	if (b)
		LogAdd(L"Значение HOTKEYVK отсутствует, используем стандартное: " + std::to_wstring(pv.hk.otherKey));

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