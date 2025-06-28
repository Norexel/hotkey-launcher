#include "Design.h"
#include <vector> 
void DrawCustomBackground(HDC hdc, HWND hWnd) {
    Gdiplus::Graphics graphics(hdc);
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Gdiplus::Color color1(255, 60, 60, 60);  
    Gdiplus::Color color2(255, 100, 100, 100); 
    Gdiplus::LinearGradientBrush gradientBrush(
        Gdiplus::Point(clientRect.left, clientRect.top),
        Gdiplus::Point(clientRect.right, clientRect.bottom),
        color1,
        color2
    );

    
    graphics.FillRectangle(&gradientBrush, Gdiplus::Rect(clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top));
}


void DrawWSAHeader(HDC hdc, HWND hWnd) {
    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias); 
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);

  
    Gdiplus::FontFamily fontFamily(L"Arial"); 
    Gdiplus::Font font(&fontFamily, 72, Gdiplus::FontStyleBold, Gdiplus::UnitPixel); 

    Gdiplus::SolidBrush solidBrush(Gdiplus::Color(255, 200, 200, 200)); 

    Gdiplus::StringFormat stringFormat;
    stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);   
    stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter); 
    Gdiplus::RectF layoutRect(
        (Gdiplus::REAL)clientRect.left,
        (Gdiplus::REAL)clientRect.top,
        (Gdiplus::REAL)(clientRect.right - clientRect.left),
        (Gdiplus::REAL)(clientRect.bottom - clientRect.top) / 4 
    );

   
}


HFONT CreateAppFont(int fontSize, bool bold) {
    LOGFONT lf = { 0 };
    lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
    lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Segoe UI"); 

    return CreateFontIndirect(&lf);
}


void SetControlFont(HWND hControl, HFONT hFont) {
    if (hControl && hFont) {
        SendMessage(hControl, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
}