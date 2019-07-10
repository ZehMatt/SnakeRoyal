#pragma once

#include <stdint.h>
#include <windows.h>

#include "Color.h"

class Painter
{
    HWND _hWnd;
    PAINTSTRUCT _ps;
    HDC _hdc;
    HDC _hdcMem;
    HBITMAP _hbmMem;
    HBITMAP _hbmOld;
    HFONT _hFont;
    RECT _rect;
    HBRUSH _brush;
    Color _brushColor;
    HPEN _pen;
    Color _penColor;

public:
    Painter(HWND hWnd, RECT rect);
    ~Painter();

    void setFont(const char* fontName, int weight);
    void setBgColor(Color color);
    void setColor(Color color);

    void clear(Color color);
    void rect(int32_t x, int32_t y, int32_t w, int32_t h, Color color);
    void filledRect(int32_t x, int32_t y, int32_t w, int32_t h, Color color);
    void filledEllipse(int32_t x, int32_t y, int32_t w, int32_t h, Color color);
    void text(
        const std::string& txt,
        int32_t x,
        int32_t y,
        Color color = COLOR_WHITE);
    void textCentered(
        const std::string& txt,
        int32_t x,
        int32_t y,
        int32_t w,
        int32_t h,
        Color color = COLOR_WHITE);
    void textRight(
        const std::string& txt,
        int32_t x,
        int32_t y,
        int32_t w,
        int32_t h,
        Color color);
};