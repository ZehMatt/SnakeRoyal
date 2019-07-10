#pragma once

#include <stdint.h>
#include <vector>
#include <array>

#include "Painter.h"
#include "Color.h"
#include "Config.h"

enum class TileType
{
    NONE = 0,
    SNAKE_HEAD,
    SNAKE_TAIL,
    SNAKE_DEAD,
    FOOD,
};

struct TileData_t
{
    TileType type = TileType::NONE;
    Color color = COLOR_BLACK;
};

static constexpr size_t TILE_MAP_SIZE = TILE_MAP_GRID_H * TILE_MAP_GRID_W;

class TileMap
{
private:
    std::array<TileData_t, TILE_MAP_SIZE> _tiles;

public:
    void draw(Painter& painter);

    TileData_t& getTileData(int32_t x, int32_t y)
    {
        const size_t index = x + (TILE_MAP_GRID_W * y);
        return _tiles[index];
    }

    const TileData_t& getTileData(int32_t x, int32_t y) const
    {
        const size_t index = x + (TILE_MAP_GRID_W * y);
        return _tiles[index];
    }

    void setData(int32_t x, int32_t y, TileType type, Color color);

    void reset();

    std::array<TileData_t, TILE_MAP_SIZE>& getData();
};

extern TileMap gTileMap;