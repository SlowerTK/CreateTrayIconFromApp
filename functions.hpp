#pragma once

// Базовые типы и константы
#include "Types.hpp"
#include "Constants.hpp"

// Функциональные модули
#include "WindowManager.hpp"
#include "UIManager.hpp"
#include "HotkeysManager.hpp"
#include "FileManager.hpp"
#include "ProcessManager.hpp"
#include "Logger.hpp"
#include "StartupManager.hpp"
#include "Utils.hpp"

// Общие библиотеки Windows
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <vssym32.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")