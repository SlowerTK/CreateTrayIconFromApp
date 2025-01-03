#pragma once
#include "functions.hpp"
bool isDebugMode = false;
std::vector<HiddenWindow> hiddenWindows = {};
std::vector<FavoriteWindow> favoriteWindows = {};
std::vector<FullscreenBorderlessWindow> fullscreenBorderlessWindows = {};
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
bool RunAsAdmin() {
	BOOL isAdmin = FALSE;
	PSID adminGroup = NULL;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
		if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
			isAdmin = FALSE;
		}
		FreeSid(adminGroup);
	}
	return isAdmin;
}
void DebugModCheck(WCHAR* lpCmdLine) {
	int argsCount;
	wchar_t** cmdArpvLine;
	if (cmdArpvLine = CommandLineToArgvW(lpCmdLine, &argsCount))
		for (int i = 0; i != argsCount; i++)
			if (!lstrcmpW(cmdArpvLine[i], L"debug")) {
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
void UpdateTrayMenu() {
	if (pv.hMenu) {
		DestroyMenu(pv.hMenu);
	}
	pv.hMenu = CreatePopupMenu();
	for (UINT i = 0; i < hiddenWindows.size(); ++i) {
		UINT id = 1000 + i;
		AppendMenu(pv.hMenu, MF_STRING, id, isDebugMode ? hiddenWindows[i].className.c_str() : hiddenWindows[i].windowTitle.c_str());
		HBITMAP hBitmap = IconToBitmap(hiddenWindows[i].hIcon);
		if (hBitmap) {
			SetMenuItemBitmaps(pv.hMenu, id, MF_BYCOMMAND, hBitmap, hBitmap);
		}
	}
	if (!hiddenWindows.empty()) {
		AppendMenu(pv.hMenu, MF_SEPARATOR, 0, NULL);
	}
	if (isDebugMode)
		AppendMenu(pv.hMenu, MF_STRING | MF_DISABLED, 0, L"Debug mode is enabled");
	else
		AppendMenu(pv.hMenu, MF_STRING | MF_DISABLED, 0, L"Ctrl + Alt + H");
	AppendMenu(pv.hMenu, MF_STRING, TB_SETTINGS, L"Настройки");
	AppendMenu(pv.hMenu, MF_STRING, TB_EXIT, L"Выход");
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
BOOL CALLBACK EnumWindowsAppProc(HWND hwnd, LPARAM lParam) {
	HWND hApplicationsList = (HWND)lParam;
	WCHAR windowTitle[MAX_PATH] = {0};

	if (IsWindowVisible(hwnd) && GetWindowTextW(hwnd, windowTitle, SIZEOF(windowTitle)) > 0) {
		std::wstring className = GetAllowedClassName(hwnd);
		if (!className.size())
			return true;

		DWORD processID = GetProcessId(hwnd);
		std::wstring processName = GetProcessName(processID);
		if (!processName.size())
			return true;
		int count = (int)SendMessage(pv.hFavoritesList, LB_GETCOUNT, 0, 0);
		if (count)
			for (int i = 0; i < count; i++) {
				HiddenWindow* hw = (HiddenWindow*)SendMessage(pv.hFavoritesList, LB_GETITEMDATA, i, 0);
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
		int index = (int)SendMessage(hApplicationsList, LB_ADDSTRING, 0, (LPARAM)windowTitle);
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
	for (const auto& hw : favoriteWindows){
		FavoriteWindow* HW = new FavoriteWindow;
		HW->hwnd = hw.hwnd;
		HW->windowTitle = hw.windowTitle;
		HW->className = hw.className;
		HW->hIcon = hw.hIcon;
		HW->processID = hw.processID;
		HW->processName = hw.processName;
		HW->isFavorite = hw.isFavorite;
		int index = (int)SendMessage(pv.hFavoritesList, LB_ADDSTRING, 0, (LPARAM)hw.windowTitle.c_str());
		SendMessage(pv.hFavoritesList, LB_SETITEMDATA, index, (LPARAM)HW);
		}
}
void CollapseToTray(HWND hwnd, HiddenWindow* HW) {
	wchar_t windowTitle[MAX_PATH];
	ZeroMemory(&windowTitle, SIZEOF(windowTitle));
	HICON hIcon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0);
	if (!hIcon) {
		hIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM);
		if (!hIcon) {
			hIcon = LoadIconW(NULL, IDI_APPLICATION);
		}
	}
	if (HW == nullptr) {
		HiddenWindow HW;
		GetWindowText(hwnd, windowTitle, SIZEOF(windowTitle));
		HW.hwnd = hwnd;
		HW.processID = GetProcessId(hwnd);
		HW.processName = GetProcessName(HW.processID);
		if (wcslen(windowTitle) == 0) {
			wcscpy_s(windowTitle, HW.processName.c_str());
		}
		HW.windowTitle = windowTitle;
		HW.className = GetAllowedClassName(hwnd);
		HW.hIcon = hIcon;
		HW.isFavorite = false;
		hiddenWindows.push_back(HW);
	} else {
		HW->hIcon = hIcon;
		hiddenWindows.push_back(*HW);
	}
	ShowWindow(hwnd, SW_HIDE);
	UpdateTrayMenu();

}
void CollapseToTrayFromFavorite() {
	int count = (int)SendMessage(pv.hFavoritesList, LB_GETCOUNT, 0, 0);
	if (count == LB_ERR)
		return;
	HiddenWindow* HW;
	for (int i = 0; i < count; i++) {
		HW = (HiddenWindow*)SendMessage(pv.hFavoritesList, LB_GETITEMDATA, i, 0);
		auto it = std::find_if(hiddenWindows.begin(), hiddenWindows.end(),
							   [HW](const HiddenWindow& hw) { return hw.processID == HW->processID; });
		if (it == hiddenWindows.end()) {
			CollapseToTray(HW->hwnd, HW);
		}
	}
}
void DeleteList(HWND list) {
	if (list) {
		int count = (int)SendMessage(list, LB_GETCOUNT, 0, 0);
		if (count)
			for (int i = 0; i < count; i++) {
				HiddenWindow* HW = (HiddenWindow*)SendMessage(list, LB_GETITEMDATA, i, 0);
				delete HW;
			}
		SendMessage(list, LB_RESETCONTENT, 0, 0);
	}
}
std::wstring GetAllowedClassName(HWND hwnd) {
	std::wstring className;
	wchar_t classNam[256];
	ZeroMemory(&classNam, SIZEOF(classNam));
	GetClassNameW(hwnd, classNam, SIZEOF(classNam));
	className = classNam;
	for (const auto& t : exceptionClassNames) {
		if (t == className)
			return L"";
	}
	return className;
}
DWORD GetProcessId(HWND hwnd) {
	DWORD processID;
	GetWindowThreadProcessId(hwnd, &processID);
	return processID;
}
std::wstring GetProcessName(DWORD processID) {
	std::wstring processName = L"";
	WCHAR processNam[MAX_PATH] = {0};
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (hProcess) {
		HMODULE hMod;
		DWORD cbNeeded;
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
			GetModuleBaseName(hProcess, hMod, processNam, SIZEOF(processNam));
		processName = processNam;
		CloseHandle(hProcess);
	}
	return processName;
}
std::wstring s2ws(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}
void CheckFolderAndFile() {
	char appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
		MBATTENTION(ET_APPDATA);
		return;
	}
	std::string folderPath = std::string(appDataPath) + FOLDER;
	if (CreateDirectoryA(folderPath.c_str(), NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
		MBATTENTION(ET_CREATEFOLDER);
		return;
	}
	std::string filePath = folderPath + FILEname;
	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS) {
		MBATTENTION(ET_CREATEFILE);
		return;
	}
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		return;
	}
}
std::string ReadSettingsFile() {
	char appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
		MBATTENTION(ET_APPDATA);
		return "";
	}
	std::string filePath = std::string(appDataPath) + FOLDERFILE;
	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MBATTENTION(ET_FILEOPEN);
		return "";
	}
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize)) {
		MBATTENTION(ET_FILESIZE);
		CloseHandle(hFile);
		return "";
	}
	std::string content(fileSize.QuadPart, '\0');
	DWORD bytesRead;
	if (!ReadFile(hFile, &content[0], (DWORD)fileSize.QuadPart, &bytesRead, NULL) || bytesRead != fileSize.QuadPart) {
		MBATTENTION(ET_FILECONTENT);
		CloseHandle(hFile);
		return "";
	}
	CloseHandle(hFile);
	return content;
}
void WriteSettingsFile(const std::string& content) {
	char appDataPath[MAX_PATH];
	if (FAILED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
		MBATTENTION(ET_APPDATA);
		return;
	}
	std::string filePath = std::string(appDataPath) + FOLDERFILE;
	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MBATTENTION(ET_FILEOPEN);
		return;
	}
	DWORD bytesWritten;
	if (!WriteFile(hFile, content.c_str(), static_cast<DWORD>(content.size()), &bytesWritten, NULL)) {
		MBATTENTION(ET_FILEWRITE);
	}
	CloseHandle(hFile);
}

std::string ws2s(const std::wstring& wstr) {
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string str(size_needed - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
	return str;
}
std::string serializeToString(const FavoriteWindow& s) {
	std::ostringstream oss;
	oss << reinterpret_cast<uintptr_t>(s.hwnd) << "|"
		<< ws2s(s.windowTitle) << "|"
		<< reinterpret_cast<uintptr_t>(s.hIcon) << "|"
		<< s.processID << "|"
		<< ws2s(s.className) << "|"
		<< ws2s(s.processName) << "\n";
	return oss.str();
}
FavoriteWindow deserializeOne(const std::string& str) {
	std::istringstream iss(str);
	FavoriteWindow s;
	std::string token;
	std::getline(iss, token, '|');
	s.hwnd = reinterpret_cast<HWND>(std::stoull(token));
	std::getline(iss, token, '|');
	s.windowTitle = s2ws(token);
	std::getline(iss, token, '|');
	s.hIcon = reinterpret_cast<HICON>(std::stoull(token));
	std::getline(iss, token, '|');
	s.processID = std::stoul(token);
	std::getline(iss, token, '|');
	s.className = s2ws(token);
	std::getline(iss, token, '\n');
	s.processName = s2ws(token);
	return s;
}
void deserializeFromString(const std::string& str) {
	std::vector<HiddenWindow> result;
	std::istringstream iss(str);
	std::string line;

	while (std::getline(iss, line)) {
		if (!line.empty()) {
			result.push_back(deserializeOne(line));
		}
	}
	favoriteWindows = result;
}