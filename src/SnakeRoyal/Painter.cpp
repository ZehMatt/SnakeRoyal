#include "Painter.h"
#include <windows.h>

Painter::Painter(HWND hwnd, RECT rect)
    : _hWnd(hwnd)
    , _rect(rect)
    , _brush(nullptr)
    , _brushColor{ 0, 0, 0 }
    , _pen(nullptr)
    , _penColor{ 255, 255, 255 }
    , _hFont(nullptr)
{
    _hdc = BeginPaint(_hWnd, &_ps);
    _hdcMem = CreateCompatibleDC(_ps.hdc);
    _hbmMem = CreateCompatibleBitmap(
        _ps.hdc, _rect.right - rect.left, _rect.bottom - _rect.top);

    _hbmOld = (HBITMAP)SelectObject(_hdcMem, _hbmMem);
}

Painter::~Painter()
{
    BitBlt(
        _ps.hdc, _rect.left, _rect.top, _rect.right - _rect.left,
        _rect.bottom - _rect.top, _hdcMem, 0, 0, SRCCOPY);

    SelectObject(_hdcMem, _hbmOld);

    if (_brush != nullptr)
        DeleteObject(_brush);

    if (_pen != nullptr)
        DeleteObject(_pen);

    if (_hbmMem)
        DeleteObject(_hbmMem);

    if (_hdcMem)
        DeleteDC(_hdcMem);

    if (_hFont)
        DeleteObject(_hFont);

    EndPaint(_hWnd, &_ps);
}

void Painter::setFont(const char* fontName, int weight)
{
    if (_hFont != nullptr)
        DeleteObject(_hFont);

    _hFont = CreateFontA(
        weight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, fontName);
}

void Painter::setBgColor(Color color)
{
    if (_brush != nullptr && color == _brushColor)
        return;

    if (_brush != nullptr)
    {
        DeleteObject(_brush);
    }

    _brush = CreateSolidBrush(color.getColorRef());
    _brushColor = color;
}

void Painter::setColor(Color color)
{
    if (_pen != nullptr && color == _penColor)
        return;

    if (_brush != nullptr)
    {
        DeleteObject(_brush);
    }

    _pen = CreatePen(PS_SOLID, 0, color.getColorRef());
    _penColor = color;
}

void Painter::clear(Color color)
{
    setBgColor(color);
    FillRect(_hdcMem, &_rect, _brush);
}

void Painter::rect(int32_t x, int32_t y, int32_t w, int32_t h, Color color)
{
    setBgColor(color);
    RECT rc{ x, y, x + w, y + h };
    FrameRect(_hdcMem, &rc, _brush);
}

void Painter::filledRect(
    int32_t x, int32_t y, int32_t w, int32_t h, Color color)
{
    setBgColor(color);

    RECT rc;
    rc.left = x;
    rc.top = y;
    rc.right = x + w;
    rc.bottom = y + h;

    FillRect(_hdcMem, &rc, _brush);
}

void Painter::filledEllipse(
    int32_t x, int32_t y, int32_t w, int32_t h, Color color)
{
    setBgColor(color);
    SelectObject(_hdcMem, _brush);
    Ellipse(_hdcMem, x, y, x + w, y + h);
}

void Painter::text(const std::string& txt, int32_t x, int32_t y, Color color)
{
    setFont("Terminal", 12);
    SelectObject(_hdcMem, _hFont);
    SetBkColor(_hdcMem, RGB(0, 0, 0));
    SetTextColor(_hdcMem, color.getColorRef());

    RECT rc;
    rc.left = x;
    rc.top = y;
    rc.right = x;
    rc.bottom = y;

    DrawTextA(_hdcMem, txt.c_str(), -1, &rc, DT_LEFT | DT_NOPREFIX | DT_NOCLIP);
}

void Painter::textCentered(
    const std::string& txt,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    Color color)
{
    setFont("Terminal", 12);
    SelectObject(_hdcMem, _hFont);
    SetBkColor(_hdcMem, COLOR_BG.getColorRef());
    SetTextColor(_hdcMem, color.getColorRef());

    RECT rc;
    rc.left = x;
    rc.top = y;
    rc.right = x + w;
    rc.bottom = y + h;

    DrawTextA(_hdcMem, txt.c_str(), -1, &rc, DT_CENTER | DT_NOPREFIX);
}

void Painter::textRight(
    const std::string& txt,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    Color color)
{
    setFont("Terminal", 12);
    SelectObject(_hdcMem, _hFont);
    SetBkColor(_hdcMem, COLOR_BG.getColorRef());
    SetTextColor(_hdcMem, color.getColorRef());

    RECT rc;
    rc.left = x;
    rc.top = y;
    rc.right = x + w;
    rc.bottom = y + h;

    DrawTextA(_hdcMem, txt.c_str(), -1, &rc, DT_RIGHT | DT_NOPREFIX);
}