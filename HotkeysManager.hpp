#pragma once
#include "Types.hpp"

void LoadNumberFromRegistry();
void SaveToRegistry(bool a, bool b);
void SetZeroModKeysState(BYTE* keyState = nullptr);
void RegHotKey(UINT mod, UINT other, int id);
std::wstring convertKeysToWstring(UINT modKeys, UINT otherKey);
void SaveHotKeys(byte mod, byte other);
void ReadHotKeys();