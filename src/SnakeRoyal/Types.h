#pragma once

#include <stdint.h>
#include <limits>
#include <vector>
#include <array>

#include "Vector2.h"
#include "Color.h"

using PlayerId = uint8_t;

static constexpr PlayerId INVALID_PLAYER_ID = std::numeric_limits<
    PlayerId>::max();

using SnakeId = uint8_t;

static constexpr SnakeId INVALID_SNAKE_ID = std::numeric_limits<SnakeId>::max();