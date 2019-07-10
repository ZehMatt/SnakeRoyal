#pragma once

#include <stdint.h>
#include <iterator>
#include <windows.h>

#include "Config.h"

struct Color
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    COLORREF getColorRef() const
    {
        return RGB(r, g, b);
    }

    bool operator==(const Color& other) const
    {
        return r == other.r && g == other.g && b == other.b;
    }

    bool operator!=(const Color& other) const
    {
        return !(other == *this);
    }
};

static constexpr Color COLOR_BLACK{ 0, 0, 0 };
static constexpr Color COLOR_BLUE{ 0, 0, 128 };
static constexpr Color COLOR_BRIGHT_BLUE{ 0, 0, 255 };
static constexpr Color COLOR_BRIGHT_RED{ 255, 0, 0 };
static constexpr Color COLOR_MAGENTA{ 128, 0, 128 };
static constexpr Color COLOR_MAUVE{ 128, 0, 255 };
static constexpr Color COLOR_PURPLE{ 255, 0, 128 };
static constexpr Color COLOR_BRIGHT_MAGENTA{ 255, 0, 255 };
static constexpr Color COLOR_GREEN{ 0, 128, 0 };
static constexpr Color COLOR_CYAN{ 0, 128, 128 };
static constexpr Color COLOR_SKY_BLUE{ 0, 128, 255 };
static constexpr Color COLOR_RED{ 128, 0, 0 };
static constexpr Color COLOR_YELLOW{ 128, 128, 0 };
static constexpr Color COLOR_GREY{ 128, 128, 128 };
static constexpr Color COLOR_PASTEL_BLUE{ 128, 128, 255 };
static constexpr Color COLOR_ORANGE{ 255, 128, 0 };
static constexpr Color COLOR_PINK{ 255, 128, 128 };
static constexpr Color COLOR_PASTEL_MAGENTA{ 255, 128, 255 };
static constexpr Color COLOR_BRIGHT_GREEN{ 0, 255, 0 };
static constexpr Color COLOR_SEA_GREEN{ 0, 255, 128 };
static constexpr Color COLOR_BRIGHT_CYAN{ 0, 255, 255 };
static constexpr Color COLOR_LIME{ 128, 255, 0 };
static constexpr Color COLOR_PASTEL_GREEN{ 128, 255, 128 };
static constexpr Color COLOR_PASTEL_CYAN{ 128, 255, 255 };
static constexpr Color COLOR_BRIGHT_YELLOW{ 255, 255, 0 };
static constexpr Color COLOR_PASTEL_YELLOW{ 255, 255, 128 };
static constexpr Color COLOR_WHITE{ 255, 255, 255 };

static constexpr Color COLOR_BG = COLOR_BLACK;

static constexpr Color COLOR_PLAYER_PALETTE[] = {
    COLOR_BRIGHT_BLUE,    COLOR_RED,           COLOR_MAGENTA,
    COLOR_MAUVE,          COLOR_PURPLE,        COLOR_BRIGHT_MAGENTA,
    COLOR_GREEN,          COLOR_PASTEL_GREEN,  COLOR_CYAN,
    COLOR_SKY_BLUE,       COLOR_BRIGHT_YELLOW, COLOR_YELLOW,
    COLOR_PASTEL_BLUE,    COLOR_BLUE,          COLOR_PINK,
    COLOR_PASTEL_MAGENTA, COLOR_BRIGHT_GREEN,  COLOR_BRIGHT_RED,
    COLOR_SEA_GREEN,      COLOR_BRIGHT_CYAN,   COLOR_LIME,
    COLOR_PASTEL_CYAN,    COLOR_PASTEL_YELLOW, COLOR_WHITE,
};

static_assert(MAX_PLAYERS <= std::size(COLOR_PLAYER_PALETTE));