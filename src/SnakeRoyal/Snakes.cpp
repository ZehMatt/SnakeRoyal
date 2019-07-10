#include <array>
#include "Snakes.h"
#include "TileMap.h"
#include "Utils.h"
#include "Network.h"
#include "Players.h"

Snakes gSnakes;

static constexpr Color COLOR_DEAD = COLOR_GREY;

static void moveSnakePiece(
    Snake& snake, size_t i, Vector2i newPos, TileType type, Color color)
{
    const Vector2i& oldPos = snake.pieces[i];
    gTileMap.setData(oldPos.x, oldPos.y, TileType::NONE, COLOR_BG);
    snake.pieces[i] = newPos;
    gTileMap.setData(newPos.x, newPos.y, type, color);
}

static void snakeDeath(Snake& snake)
{
    snake.state = SnakeState::DEAD;
    for (auto& pieces : snake.pieces)
    {
        gTileMap.setData(pieces.x, pieces.y, TileType::SNAKE_DEAD, COLOR_DEAD);
    }
}

static void updateSnake(Snake& snake)
{
    if (snake.direction == DIR_NONE)
        return;

    const Vector2i head = snake.pieces[0];

    Vector2i newPos = head + snake.direction;
    newPos.x = Utils::mod(newPos.x, TILE_MAP_GRID_W);
    newPos.y = Utils::mod(newPos.y, TILE_MAP_GRID_H);

    Color color = gPlayers.getColor(snake.playerId);

    bool replaceFood = false;

    TileData_t& tileData = gTileMap.getTileData(newPos.x, newPos.y);
    if (tileData.type == TileType::FOOD)
    {
        // Collision with food, grow one tail.
        snake.pieces.insert(snake.pieces.begin(), newPos);
        replaceFood = true;
    }
    else if (
        tileData.type == TileType::SNAKE_TAIL
        || tileData.type == TileType::SNAKE_HEAD)
    {
        // Snake collision.
        snakeDeath(snake);
        return;
    }
    else if (tileData.type == TileType::SNAKE_DEAD)
    {
        // Remove our tail.
        auto& tailPos = snake.pieces.back();
        gTileMap.setData(tailPos.x, tailPos.y, TileType::SNAKE_DEAD, color);

        snake.pieces.pop_back();
        if (snake.pieces.empty())
        {
            snakeDeath(snake);
            return;
        }

        moveSnakePiece(snake, 0, newPos, TileType::SNAKE_HEAD, color);
    }
    else
    {
        if (snake.pieces.size() > 1)
        {
            // Move tail to front.
            snake.pieces.insert(snake.pieces.begin(), snake.pieces.back());
            snake.pieces.pop_back();
        }
    }

    // Update the tile map based on our pieces.
    moveSnakePiece(snake, 0, newPos, TileType::SNAKE_HEAD, color);
    for (size_t i = 1; i < snake.pieces.size(); i++)
    {
        moveSnakePiece(snake, i, snake.pieces[i], TileType::SNAKE_TAIL, color);
    }

    // Picking up food replaces it with one.
    if (replaceFood)
    {
        gGame.createFood();
    }
}

SnakeId Snakes::create(PlayerId playerId, int32_t x, int32_t y)
{
    SnakeId res = INVALID_SNAKE_ID;

    for (SnakeId id = 0; id < _snakes.size(); id++)
    {
        Snake& snake = _snakes[id];
        if (snake.id == INVALID_SNAKE_ID)
        {
            snake.state = SnakeState::ALIVE;
            snake.pieces.push_back({ x, y });
            snake.id = id;
            snake.playerId = playerId;
            snake.direction = DIR_NONE;
            res = id;

            Color color = gPlayers.getColor(playerId);
            gTileMap.setData(x, y, TileType::SNAKE_HEAD, color);
            break;
        }
    }

    return res;
}

Snake& Snakes::getData(SnakeId id)
{
    return _snakes[id];
}

void Snakes::remove(SnakeId id)
{
    Snake& snake = _snakes[id];
    snake.id = INVALID_SNAKE_ID;
    snake.direction = DIR_NONE;
    for (auto& piece : snake.pieces)
    {
        gTileMap.setData(piece.x, piece.y, TileType::NONE, COLOR_BG);
    }
    snake.pieces.clear();
}

void Snakes::update()
{
    size_t alive = 0;
    size_t count = 0;

    for (auto& snake : _snakes)
    {
        if (snake.id == INVALID_SNAKE_ID)
            continue;

        count++;

        if (snake.state != SnakeState::ALIVE)
            continue;

        updateSnake(snake);

        if (snake.state == SnakeState::ALIVE)
            alive++;
    }
}

void Snakes::setDirection(SnakeId id, const Vector2i& dir)
{
    Snake& snake = _snakes[id];
    snake.direction = dir;
}

Vector2i Snakes::getDirection(SnakeId id) const
{
    const Snake& snake = _snakes[id];
    return snake.direction;
}

size_t Snakes::count() const
{
    size_t res = 0;
    for (auto& snake : _snakes)
    {
        if (snake.id != INVALID_SNAKE_ID)
            res++;
    }
    return res;
}

size_t Snakes::alive() const
{
    size_t res = 0;
    for (auto& snake : _snakes)
    {
        if (snake.id == INVALID_SNAKE_ID)
            continue;
        if (snake.state == SnakeState::ALIVE)
            res++;
    }
    return res;
}

const std::array<Snake, MAX_PLAYERS>& Snakes::getSnakes() const
{
    return _snakes;
}
