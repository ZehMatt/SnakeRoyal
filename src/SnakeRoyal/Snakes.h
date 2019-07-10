#pragma once

#include "Types.h"
#include "Config.h"
#include "Snake.h"

class Snakes
{
    std::array<Snake, MAX_PLAYERS> _snakes;

public:
    Snakes() = default;

    SnakeId create(PlayerId playerId, int32_t x, int32_t y);
    Snake& getData(SnakeId id);
    void remove(SnakeId id);
    void update();
    void setDirection(SnakeId id, const Vector2i& dir);
    Vector2i getDirection(SnakeId id) const;

    size_t count() const;
    size_t alive() const;

    const std::array<Snake, MAX_PLAYERS>& getSnakes() const;
};

extern Snakes gSnakes;