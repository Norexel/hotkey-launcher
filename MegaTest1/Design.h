#pragma once
#include <windows.h>
#include <string>
#include <gdiplus.h> 
void DrawCustomBackground(HDC hdc, HWND hWnd);
void DrawWSAHeader(HDC hdc, HWND hWnd);
HFONT CreateAppFont(int fontSize, bool bold);
void SetControlFont(HWND hControl, HFONT hFont);