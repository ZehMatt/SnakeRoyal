#pragma once

#include "Types.h"

enum class SnakeState : uint8_t
{
    IDLE = 0,
    ALIVE,
    DEAD,
};

struct SnakeBase
{
    SnakeState state = SnakeState::IDLE;
    SnakeId id = INVALID_SNAKE_ID;
    Vector2i direction;
    PlayerId playerId;
};

struct Snake : SnakeBase
{
    std::vector<Vector2i> pieces;
};