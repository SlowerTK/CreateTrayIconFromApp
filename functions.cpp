#pragma once
#include "functions.hpp"
bool isDebugMode = false;
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
	int argsCount;
	wchar_t** cmdArpvLine;
	if (cmdArpvLine = CommandLineToArgvW(lpCmdLine, &argsCount))
		for (int i = 0; i != argsCount; i++)
			if (!lstrcmpW(cmdArpvLine[i], DEBUG_STRING)) {
				isDebugMode = true;
				break;
			}
}
void OpenSettings() {
	if (!pv.settWin)
		pv.settWin = CreateWindowExW(0, pv.wc2.lpszClassName, pv.wc2.lpszClassName, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX | WS_VISIBLE,
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
	for (UINT i = 0; i < hiddenWindows.size(); ++i) {
		UINT id = 1000 + i;
		AppendMenu(pv.hMenu, MF_STRING, id, (isDebugMode || isDebug) ? hiddenWindows[i].processName.c_str() : hiddenWindows[i].windowTitle.c_str());
		HBITMAP hBitmap = IconToBitmap(hiddenWindows[i].hIcon);
		if (hBitmap) SetMenuItemBitmaps(pv.hMenu, id, MF_BYCOMMAND, hBitmap, hBitmap);
	}
	if (!hiddenWindows.empty()) AppendMenu(pv.hMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(pv.hMenu, MF_STRING | MF_DISABLED, 0, TB_HOTKEY_TEXT);
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
		return L"";
		//return L"Ошибка инициализации COM.";
	}

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
								RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres)) {
		CoUninitialize();
		return L"";
		//return L"Ошибка настройки безопасности COM.";
	}

	IWbemLocator* pLoc = NULL;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
	if (FAILED(hres)) {
		CoUninitialize();
		return L"";
		//return L"Не удалось создать WMI-локатор.";
	}

	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
	if (FAILED(hres)) {
		pLoc->Release();
		CoUninitialize();
		return L"";
		//return L"Не удалось подключиться к WMI.";
	}

	hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return L"";
		//return L"Не удалось установить параметры безопасности.";
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	std::wstring query = QUERY + std::to_wstring(processID);

	hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return L"";
		//return L"Не удалось выполнить запрос WMI.";
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
BOOL CALLBACK EnumWindowsAppProc(HWND hwnd, LPARAM lParam) {
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
		if (count)
			for (int i = 0; i < count; i++) {
				HiddenWindow* hw = (HiddenWindow*)SendMessage(pv.hFavoritesList, LB_GETITEMDATA, i, NULL);
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
	EnumWindows(EnumWindowsAppProc, (LPARAM)pv.hApplicationsList);
}
void UpdateFavoriteList() {
	if (!pv.hFavoritesList)
		return;
	DeleteList(pv.hFavoritesList);
	for (const auto& hw : favoriteWindows) {
		HiddenWindow* HW = new HiddenWindow;
		HW->hwnd = hw.hwnd;
		HW->windowTitle = hw.windowTitle;
		HW->className = hw.className;
		HW->hIcon = hw.hIcon;
		HW->processID = hw.processID;
		HW->processName = hw.processName;
		HW->isFavorite = hw.isFavorite;
		HW->comandLine = hw.comandLine;
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
void CollapseToTray(HWND hwnd, HiddenWindow* HW) {
	if (HW == nullptr) {
		DWORD processID = GetProcessId(hwnd);
		std::wstring processName = GetAllowedProcessName(processID);
		if (processName.empty())
			return;
		std::wstring className = GetWstringClassName(hwnd);
		if (className.empty())
			return;
		wchar_t windowTitle[MAX_PATH];
		ZeroMemory(&windowTitle, SIZEOF(windowTitle));
		GetWindowTextW(hwnd, windowTitle, SIZEOF(windowTitle));
		hiddenWindows.push_back({hwnd, GetIcon(hwnd), processID, false, windowTitle, className, processName, GetProcessCommandLine(processID)});
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

		if (HW->isFavorite != SAVED_WINDOW && std::none_of(hiddenWindows.begin(), hiddenWindows.end(), [HW](const HiddenWindow& hw) { return hw.processID == HW->processID; })) {
			HW->comandLine = GetProcessCommandLine(HW->processID);
			if (std::none_of(favoriteWindows.begin(), favoriteWindows.end(), [HW](const HiddenWindow& hw) { return hw.processID == HW->processID; }))
				favoriteWindows.push_back(*HW);
			CollapseToTray(HW->hwnd, HW);
		}
	}
}
void DeleteList(HWND list) {
	if (list) {
		int count = (int)SendMessage(list, LB_GETCOUNT, NULL, NULL);
		if (count)
			for (int i = 0; i < count; i++) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(list, LB_GETITEMDATA, i, NULL);
				delete HW;
			}
		SendMessage(list, LB_RESETCONTENT, NULL, NULL);
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
void CheckFolderAndFile() {
	wchar_t appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, appDataPath))) {
		MBATTENTION(ET_APPDATA);
		return;
	}
	std::wstring folderPath = std::wstring(appDataPath) + FOLDERNAME;
	if (CreateDirectory(folderPath.c_str(), NULL) == NULL && GetLastError() != ERROR_ALREADY_EXISTS) {
		MBATTENTION(ET_CREATEFOLDER);
		return;
	}
	std::wstring filePath = folderPath + FILEPATHNAME;
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, NULL, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS) {
		MBATTENTION(ET_CREATEFILE);
		return;
	}
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		return;
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
void WriteSettingsFile(const std::wstring& content) {
	wchar_t appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, appDataPath))) {
		MBATTENTION(ET_APPDATA);
		return;
	}
	std::wstring filePath = std::wstring(appDataPath) + FOLDERNAME + FILEPATHNAME;
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MBATTENTION(ET_FILEOPEN);
		return;
	}
	DWORD bytesWritten;
	if (!WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.length() * sizeof(wchar_t)), &bytesWritten, NULL)) {
		MBATTENTION(ET_FILEWRITE);
	}
	CloseHandle(hFile);
}
std::wstring serializeToWstring(const HiddenWindow& s) {
	std::wostringstream oss;
	oss << s.windowTitle << L'|'
		<< s.className << L'|'
		<< s.processName << L'|'
		<< s.comandLine << L'\n';
	return oss.str();
}
BOOL CALLBACK EnumWindowsHWND(HWND hwnd, LPARAM lParam) {
	HiddenWindow hw = *(HiddenWindow*)lParam;
	hw.processID = GetProcessId(hwnd);
	std::wstring processName = GetAllowedProcessName(hw.processID);
	if (processName == L"") {
		return TRUE;
	}
	if (processName == hw.processName) {
		hw.hwnd = hwnd;
		hw.processName = processName;
		*(HiddenWindow*)lParam = hw;
		return FALSE;
	}
	return TRUE;
}
void FindWindowFromFile(HiddenWindow& s, bool isFromFile) {
	EnumWindows(EnumWindowsHWND, (LPARAM)&s);
	if (isFromFile) s.isFavorite = SAVED_WINDOW;
	if (s.hwnd && s.isFavorite == SAVED_WINDOW) {
		wchar_t windowTitle[MAX_PATH];
		ZeroMemory(&windowTitle, SIZEOF(windowTitle));
		GetWindowTextW(s.hwnd, windowTitle, SIZEOF(windowTitle));
		if (lstrcmpW(windowTitle, s.windowTitle.c_str()) == 0) {
			s.isFavorite = true;
			CollapseToTray(s.hwnd, &s);
		}
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
			line.clear();
		}
	}
	favoriteWindows = result;
	result.clear();
	for (const auto& hw : favoriteWindows)
		if (hw.isFavorite == TRUE)
			result.push_back(hw);
	hiddenWindows = result;
}