#include "AppConstants.hpp"
#include "FileManager.hpp"
#include <ShlObj.h>
#include <sstream>
#include "WindowManager.hpp"

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
    }
    else {
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
    if (!ReadFile(hFile, &content[0], static_cast<DWORD>(fileSize.QuadPart), &bytesRead, NULL) || bytesRead != fileSize.QuadPart) {
        MBATTENTION(ET_FILE_READ);
        CloseHandle(hFile);
        return{};
    }
    CloseHandle(hFile);
    return content;
}
std::wstring serializeToWstring(const HiddenWindow& hw) {
    std::wostringstream oss;
    oss << hw.windowTitle << L'|'
        << hw.className << L'|'
        << hw.processName << L'|'
        << hw.commandLine << L'\n';
    return oss.str();
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
    s.isFavorite = FavoriteState::SavedFavorite;
    FindWindowFromFile(s);
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