#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#define MAX_LOADSTRING 100
#define WM_KEY_PRESS_EVENT (WM_USER + 1)
#define WM_TRAYICON_MESSAGE (WM_USER + 2) 
struct ProgramInfo {
    std::wstring path;    
    std::wstring command; 
    WORD hotkey;          
};
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void ShowMainWindowControls(HWND hWnd, bool show);
void ShowAddProgramScreen(HWND hWnd, bool show);
void ClearAddProgramInputs(HWND hWnd);
void LoadProgramsToList(HWND hWnd);
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
void StartKeyboardMonitoring(HWND hWnd);
void StopKeyboardMonitoring();
HWND GetWindowHandleFromProcessId(DWORD processId);
void ToggleProgramByHotkey(HWND hWnd, WORD hotkey);
