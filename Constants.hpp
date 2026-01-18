#pragma once
#include <string>
#include "resource.h"
#include "AppConstants.hpp"

static std::wstring exceptionProcessNames[] = {
    L"",
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
    SY_CLASS_TRAY,
    SY_CLASS_SETTINGS,
    SY_CLASS_TIMERSET };