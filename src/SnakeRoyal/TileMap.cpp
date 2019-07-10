#include "TileMap.h"
#include "Game.h"
#include <assert.h>

TileMap gTileMap;

void TileMap::draw(Painter& painter)
{
    painter.rect(
        TILE_MAP_MARGIN_LEFT - 1, TILE_MAP_MARGIN_TOP - 1, TILE_MAP_SIZE_W + 2,
        TILE_MAP_SIZE_H + 2, { 0, 255, 0 });

    for (int32_t x = 0; x < TILE_MAP_GRID_W; x++)
    {
        int32_t xx = x * (TILE_SIZE_W + 1);
        for (int32_t y = 0; y < TILE_MAP_GRID_H; y++)
        {
            int32_t yy = y * (TILE_SIZE_H + 1);

            const TileData_t& data = getTileData(x, y);
            switch (data.type)
            {
                case TileType::NONE:
                    painter.filledRect(
                        xx + TILE_MAP_MARGIN_LEFT, yy + TILE_MAP_MARGIN_TOP,
                        TILE_SIZE_W, TILE_SIZE_H, { 0, 0, 0 });
                    break;
                case TileType::SNAKE_HEAD:
                    painter.filledRect(
                        xx + TILE_MAP_MARGIN_LEFT, yy + TILE_MAP_MARGIN_TOP,
                        TILE_SIZE_W, TILE_SIZE_H, data.color);
                    break;
                case TileType::SNAKE_TAIL:
                {
                    Color color = data.color;
                    color.r /= 2;
                    color.g /= 2;
                    color.b /= 2;
                    painter.filledRect(
                        xx + TILE_MAP_MARGIN_LEFT, yy + TILE_MAP_MARGIN_TOP,
                        TILE_SIZE_W, TILE_SIZE_H, color);
                }
                break;
                case TileType::SNAKE_DEAD:
                    painter.filledRect(
                        xx + TILE_MAP_MARGIN_LEFT, yy + TILE_MAP_MARGIN_TOP,
                        TILE_SIZE_W, TILE_SIZE_H, data.color);
                    break;
                case TileType::FOOD:
                    painter.filledEllipse(
                        xx + TILE_MAP_MARGIN_LEFT, yy + TILE_MAP_MARGIN_TOP,
                        TILE_SIZE_W, TILE_SIZE_H, data.color);
                    break;
                default:
                    assert(false);
            }
        }
    }
}

void TileMap::setData(int32_t x, int32_t y, TileType type, Color color)
{
    TileData_t& data = getTileData(x, y);
    data.type = type;
    data.color = color;
}

void TileMap::reset()
{
    for (auto& tileData : _tiles)
    {
        tileData.type = TileType::NONE;
    }
}

std::array<TileData_t, TILE_MAP_SIZE>& TileMap::getData()
{
    return _tiles;
}
