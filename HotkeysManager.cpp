#include "HotkeysManager.hpp"
#include "AppConstants.hpp"
#include "Logger.hpp"
#include <map>

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
        static const std::wstring sysVk[] = { L"Ctrl", L"Win", L"Alt", L"Shift" };
        static const byte MODKEYS[] = { MOD_CONTROL, MOD_WIN, MOD_ALT, MOD_SHIFT };
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
        wchar_t buffer[5] = { 0 };
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