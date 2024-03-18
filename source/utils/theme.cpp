#include <windows.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#pragma comment(lib, "Dwmapi")

bool EnableDarkTheme(size_t wnd)
{
    BOOL value = TRUE;
    ::DwmSetWindowAttribute((HWND)wnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    return value;
}