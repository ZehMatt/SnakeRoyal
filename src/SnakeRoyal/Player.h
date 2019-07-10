#pragma once

#include <stdint.h>
#include <array>
#include <limits>

#include "Config.h"
#include "Types.h"

enum class Buttons : uint32_t
{
    NONE = 0,
    UP = (1 << 0),
    DOWN = (1 << 1),
    LEFT = (1 << 2),
    RIGHT = (1 << 3),
};

struct PlayerData
{
    PlayerId id = INVALID_PLAYER_ID;
    SnakeId snakeId = INVALID_SNAKE_ID;
    Color color = COLOR_BLACK;
};

struct Player : PlayerData
{
    uint32_t pressed = 0;
    char name[128] = {};

    bool isButtonPressed(Buttons button) const
    {
        return (pressed & static_cast<uint32_t>(button)) != 0;
    }
};