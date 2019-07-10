#include <iostream>
#include <windows.h>
#include <thread>
#include <assert.h>

#include "Logging.h"
#include "Game.h"
#include "TileMap.h"
#include "Snakes.h"
#include "Players.h"
#include "Utils.h"
#include "Network.h"

Game gGame;

static constexpr Color COLOR_FOOD = COLOR_ORANGE;

void Game::setHeadless(bool headless)
{
    _headless = headless;
}

bool Game::getHeadless() const
{
    return _headless;
}

void Game::init(HWND hWnd)
{
    logPrint("%s\n", __FUNCTION__);

    _hWnd = hWnd;
    _randState = static_cast<uint32_t>(time(nullptr));
    _tick = 0;

    if (gNetwork.getMode() == NetworkMode::NONE)
    {
        restart(GAME_ROUND_RESTART_TICKS);
    }
}

void Game::restart(uint32_t delayInTicks)
{
    logPrint("%s\n", __FUNCTION__);

    setRoundState(RoundState::RESTARTING, delayInTicks);

    if (gNetwork.getMode() == NetworkMode::SERVER)
    {
        MessageServerRoundRestart msgRoundRestart;
        msgRoundRestart.tick = _tick;
        msgRoundRestart.delay = delayInTicks;
        gNetwork.sendMessage(msgRoundRestart);
    }
}

void Game::startRound()
{
    logPrint("%s\n", __FUNCTION__);

    gTileMap.reset();

    size_t playerCount = gPlayers.count();

    int32_t spawnY = TILE_MAP_GRID_H / 2;
    int32_t spawnSpacing = TILE_MAP_GRID_W / (playerCount + 1);
    int32_t spawnX = spawnSpacing;

    for (PlayerId playerId = 0; playerId < MAX_PLAYERS; playerId++)
    {
        if (!gPlayers.isValidPlayer(playerId))
            continue;

        const Player& player = gPlayers.getPlayer(playerId);
        if (player.snakeId != INVALID_SNAKE_ID)
        {
            gSnakes.remove(player.snakeId);
        }

        SnakeId snakeId = gSnakes.create(playerId, spawnX, spawnY);
        spawnX += spawnSpacing;

        gPlayers.setSnake(playerId, snakeId);
    }

    // Place food.
    for (int i = 0; i < 3; i++)
    {
        createFood();
    }

    setRoundState(RoundState::RUNNING, 0);

    if (gNetwork.getMode() == NetworkMode::SERVER)
    {
        MessageServerRoundStart msgRoundStart;
        msgRoundStart.tick = _tick;
        gNetwork.sendMessage(msgRoundStart);
    }
}

void Game::update()
{
    int32_t numUpdates = 1;

    gNetwork.update();

    if (gNetwork.getMode() == NetworkMode::CLIENT)
    {
        // Allow client to catch up for GAME_TICK_DELTA_THRESHOLD *
        // GAME_TICK_RATE in ms
        numUpdates = std::min<int32_t>(
            gNetwork.getServerTick() - _tick, GAME_TICK_DELTA_THRESHOLD);
    }

    for (int i = 0; i < numUpdates; i++)
    {
        gNetwork.update();

        if (gNetwork.getMode() == NetworkMode::CLIENT)
        {
            if (_tick >= gNetwork.getServerTick())
                break;
        }

        if (getRoundState() == RoundState::RUNNING)
        {
            gPlayers.update();
            gSnakes.update();
        }

        if (gNetwork.getMode() == NetworkMode::SERVER)
        {
            if (getRoundState() == RoundState::RESTARTING
                && _tick >= _roundData.timeout)
            {
                startRound();
            }
            else if (getRoundState() == RoundState::RUNNING)
            {
                size_t snakesCount = gSnakes.count();
                size_t snakesAlive = gSnakes.alive();
                if (snakesCount > 0 && snakesAlive == 0)
                {
                    gGame.restart(GAME_ROUND_RESTART_TICKS);
                }
            }
        }

        _tick++;

        if (gNetwork.getMode() == NetworkMode::CLIENT)
        {
            assert(_tick <= gNetwork.getServerTick());
        }

        gNetwork.flush();
    }
}

void Game::draw()
{
    RECT rc;
    GetClientRect(_hWnd, &rc);

    Painter painter(_hWnd, rc);
    painter.clear({ 0, 0, 0 });

    gTileMap.draw(painter);

    drawInfo(painter);
}

void Game::drawInfo(Painter& painter)
{
    gPlayers.draw(painter);

    char roundInfo[1024]{};

    if (gNetwork.getMode() == NetworkMode::CLIENT)
    {
        char pingInfo[128]{};
        sprintf_s(pingInfo, "Ping: %d ms, ", gNetwork.getCurrentPing());
        strcat_s(roundInfo, pingInfo);
    }

    switch (_roundData.state)
    {
        case RoundState::IDLE:
            strcat_s(roundInfo, "State: Idle");
            break;
        case RoundState::RUNNING:
            strcat_s(roundInfo, "State: Running");
            break;
        case RoundState::RESTARTING:
        {
            char restartInfo[128]{};

            uint32_t remainingTicks = 0;
            if (_tick < _roundData.timeout)
                remainingTicks = _roundData.timeout - _tick;

            double secsRemaining = (GAME_TICK_RATE * remainingTicks);
            sprintf_s(
                restartInfo, "State: Restarting in %.02f secs ...",
                secsRemaining);

            strcat_s(roundInfo, restartInfo);
        }
        break;
    }

    painter.text(roundInfo, TILE_MAP_MARGIN_LEFT, 10);
}

uint64_t Game::getRandState() const
{
    return _randState;
}

void Game::setRandState(uint64_t state)
{
    _randState = state;
}

uint32_t Game::getRand()
{
    // Park Miller PRNG
    return _randState = ((uint64_t)_randState * 48271u) % 0x7fffffff;
}

RoundState Game::getRoundState() const
{
    return _roundData.state;
}

RoundData_t& Game::getRoundData()
{
    return _roundData;
}

void Game::setRoundState(RoundState state, uint32_t delay /* = 0*/)
{
    _roundData.state = state;
    _roundData.timeout = _tick + delay;

    if (gNetwork.getMode() == NetworkMode::SERVER)
    {
        MessageServerRoundState msgRoundState;
        msgRoundState.tick = _tick;
        msgRoundState.state = state;
        msgRoundState.delay = delay;
        gNetwork.sendMessage(msgRoundState);
    }
}

uint32_t Game::getClientTime() const
{
    return static_cast<uint32_t>(
        (static_cast<double>(_tick) * GAME_TICK_RATE) * 1000.0);
}

uint32_t Game::getHostTime() const
{
    if (gNetwork.getMode() == NetworkMode::SERVER
        || gNetwork.getMode() == NetworkMode::NONE)
        return static_cast<uint32_t>(
            (static_cast<double>(_tick) * GAME_TICK_RATE) * 1000.0);

    return static_cast<uint32_t>(
        (static_cast<double>(gNetwork.getServerTick()) * GAME_TICK_RATE)
        * 1000.0);
}

void Game::setFocus(bool hasFocus)
{
    _hasFocus = hasFocus;
}

bool Game::hasFocus() const
{
    return _hasFocus;
}

uint32_t Game::getTick() const
{
    return _tick;
}

void Game::setTick(uint32_t tick)
{
    _tick = tick;
}

void Game::createFood()
{
    while (true)
    {
        int32_t x = gGame.getRand() % TILE_MAP_GRID_W;
        int32_t y = gGame.getRand() % TILE_MAP_GRID_H;

        const TileData_t& data = gTileMap.getTileData(x, y);
        if (data.type != TileType::NONE)
            continue;

        gTileMap.setData(x, y, TileType::FOOD, COLOR_FOOD);
        break;
    }
}
