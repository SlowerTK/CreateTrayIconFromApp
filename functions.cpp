#pragma once
#include "functions.hpp"
std::vector<HiddenWindow> hiddenWindows = {};
std::vector<HiddenWindow> favoriteWindows = {};
ProgramVariable pv = {0};
WNDCLASS RegisterNewClass(LPCWSTR className, WNDPROC wndproc) {
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpszClassName = className;
	wc.hInstance = pv.hInstance;
	wc.lpfnWndProc = wndproc;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.hCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	RegisterClass(&wc);
	return wc;
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
		pv.settWin = CreateWindowExW(WS_EX_TOPMOST, pv.wc2.lpszClassName, pv.isAdminMode ? pv.wc2.lpszClassName : WND_TITLE, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX | WS_VISIBLE,
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
		UINT id = static_cast<UINT>(1000) + std::distance(hiddenWindows.begin(), it);
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
std::wstring GetProcessCommandLine(DWORD processID) {
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
			commandLine = ET_COMMANDLINE;
		}
		VariantClear(&vtProp);
		pClsObj->Release();
	} else {
		commandLine = ET_COMMANDLINE_PROC;
	}

	pEnumerator->Release();
	pSvc->Release();
	pLoc->Release();
	CoUninitialize();

	return commandLine.empty() ? ET_ARGLOST : commandLine;
}
BOOL CALLBACK EnumWindowsUpdateAppListProc(HWND hwnd, LPARAM lParam) {
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
}
HICON GetIcon(HWND hwnd) {
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
	ZeroMemory(&windowTitle, SIZEOF(windowTitle));
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

		if (HW->isFavorite == TRUE && std::none_of(hiddenWindows.begin(), hiddenWindows.end(), [HW](const HiddenWindow& hw) { return hw.processID == HW->processID; })) {
			HW->comandLine = GetProcessCommandLine(HW->processID);
			if (std::none_of(favoriteWindows.begin(), favoriteWindows.end(), [HW](const HiddenWindow& hw) { return hw.processID == HW->processID; })){
				favoriteWindows.push_back(*HW);
				LogAdd(L"В изброное добавлено " + *HW->className.c_str());
			}
			CollapseToTray(HW->hwnd, HW);
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
	ZeroMemory(&classNam, SIZEOF(classNam));
	GetClassNameW(hwnd, classNam, SIZEOF(classNam));
	if (lstrcmpW(classNam, pv.wc2.lpszClassName) == 0)
		return L"";
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
    wchar_t appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, appDataPath))) {
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
	wchar_t appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, appDataPath))) {
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
    wchar_t appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, appDataPath))) {
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
		<< hw.comandLine << L'\n';
	return oss.str();
}
void FastSerchWindow(HiddenWindow& window) {
	HWND hwnd = FindWindow(window.className.c_str(), window.windowTitle.c_str());
	DWORD processId = GetProcessId(hwnd);
	std::wstring processName = GetAllowedProcessName(processId);
	if (processName == window.processName) {
		window.hwnd = hwnd;
		window.processID = processId;
	}
}
//BOOL CALLBACK EnumWindowsFindWindow(HWND hwnd, LPARAM lParam) {
//	static std::vector<HWND> candidates;
//	HiddenWindow& hw = *(HiddenWindow*)lParam;
//	if (hw.hwnd == nullptr) {
//		candidates.clear();
//	}
//	hw.processID = GetProcessId(hwnd);
//	std::wstring processName = GetAllowedProcessName(hw.processID);
//	if (processName.empty() || processName != hw.processName || hw.className != GetWstringClassName(hwnd)) {
//		return TRUE;
//	}
//	std::wstring commandLine = GetProcessCommandLine(hw.processID);
//	if (commandLine == hw.comandLine) {
//		candidates.push_back(hwnd);
//		if (candidates.size() > 1) {
//			for (HWND candidateHwnd : candidates) {
//				if (GetWindowTitle(candidateHwnd) == hw.windowTitle) {
//					hw.hwnd = candidateHwnd;
//					*(HiddenWindow*)lParam = hw;
//					candidates.clear();
//					return FALSE;
//				}
//			}
//		} else {
//			hw.hwnd = hwnd;
//			*(HiddenWindow*)lParam = hw;
//		}
//	}
//	return TRUE;
//}
void FindWindowFromFile(HiddenWindow& windowToFind, bool isFromFile) {
	if (isFromFile) windowToFind.isFavorite = SAVED_WINDOW;
	FastSerchWindow(windowToFind);
	//if (!windowToFind.hwnd && windowToFind.isFavorite == SAVED_WINDOW) {
	//	EnumWindows(EnumWindowsFindWindow, (LPARAM)&windowToFind);
	//}
	if (windowToFind.hwnd && windowToFind.isFavorite == SAVED_WINDOW) {
		windowToFind.isFavorite = true;
		CollapseToTray(windowToFind.hwnd, &windowToFind);
	}
}
HiddenWindow deserializeOne(const std::wstring& str) {
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
	s.comandLine = token;
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
	wchar_t appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, appDataPath))) {
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
std::wstring GetTime() {
	std::wstringstream wss;
	std::time_t t = std::time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);
	wss << std::setw(2) << std::setfill(L'0') << tm.tm_hour << L':'
		<< std::setw(2) << std::setfill(L'0') << tm.tm_min << L':'
		<< std::setw(2) << std::setfill(L'0') << tm.tm_sec;
	return wss.str();
}
void LogAdd(std::wstring&& content) {
	std::wstring currentLogFile = L"\\" + GetCurrentDate() + L".log";
	CheckFolderAndFile(currentLogFile);
	WriteMyFile(currentLogFile, L"[" + GetTime() + L"]\t" + content + L"\n", true);
}