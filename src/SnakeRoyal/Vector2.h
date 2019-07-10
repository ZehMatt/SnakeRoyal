#pragma once

#include <stdint.h>

struct Vector2i
{
    int32_t x;
    int32_t y;

    Vector2i operator+(const Vector2i& other) const
    {
        return Vector2i{ x + other.x, y + other.y };
    }

    Vector2i operator-(const Vector2i& other) const
    {
        return Vector2i{ x + other.x, y + other.y };
    }

    bool operator==(const Vector2i& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vector2i& other) const
    {
        return !(other == *this);
    }
};

static constexpr Vector2i DIR_NONE{ 0, 0 };
static constexpr Vector2i DIR_UP{ 0, -1 };
static constexpr Vector2i DIR_DOWN{ 0, 1 };
static constexpr Vector2i DIR_LEFT{ -1, 0 };
static constexpr Vector2i DIR_RIGHT{ 1, 0 };