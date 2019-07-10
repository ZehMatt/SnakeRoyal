#include <windows.h>
#include <assert.h>

#include "Players.h"
#include "Snakes.h"
#include "Network.h"

Players gPlayers;

PlayerId Players::createLocalPlayer(const char* name, SnakeId snakeId)
{
    PlayerId id = createPlayer(name, snakeId);
    _localId = id;
    return id;
}

PlayerId Players::createPlayer(const char* name, SnakeId snakeId)
{
    PlayerId res = INVALID_PLAYER_ID;

    for (PlayerId id = 0; id < _players.size(); id++)
    {
        Player& player = _players[id];
        if (player.id == INVALID_PLAYER_ID)
        {
            player.id = id;
            player.snakeId = snakeId;
            player.color = COLOR_PLAYER_PALETTE[id];
            strcpy_s(player.name, name);

            res = id;
            break;
        }
    }

    return res;
}

bool Players::removePlayer(PlayerId playerId)
{
    assert(playerId != INVALID_PLAYER_ID);
    if (playerId == INVALID_PLAYER_ID)
        return false;

    Player& player = _players[playerId];
    if (player.id == INVALID_PLAYER_ID)
        return false;

    player.id = INVALID_PLAYER_ID;
    player.snakeId = INVALID_SNAKE_ID;
    player.name[0] = '\0';

    if (_localId == playerId)
        _localId = INVALID_PLAYER_ID;

    return true;
}

void Players::setPlayerById(PlayerId playerId, const Player& data)
{
    _players[playerId] = data;
}

void Players::setLocalPlayerId(PlayerId playerId)
{
    _localId = playerId;
}

void Players::setSnake(PlayerId playerId, SnakeId snakeId)
{
    assert(isValidPlayer(playerId) == true);
    _players[playerId].snakeId = snakeId;
}

uint32_t Players::getScore(PlayerId playerId) const
{
    if (!isValidPlayer(playerId))
        return 0;

    const Player& player = getPlayer(playerId);
    if (player.snakeId == INVALID_SNAKE_ID)
        return 0;

    const Snake& snake = gSnakes.getData(player.snakeId);
    if (snake.pieces.empty())
        return 0;

    uint32_t score = static_cast<uint32_t>(snake.pieces.size());

    // Magical score!
    return (score * score);
}

Color Players::getColor(PlayerId playerId) const
{
    assert(isValidPlayer(playerId) == true);
    return _players[playerId].color;
}

void Players::update()
{
    if (_localId == INVALID_PLAYER_ID)
        return;

    Player& player = _players[_localId];
    if (player.snakeId == INVALID_SNAKE_ID)
        return;

    player.pressed = 0;

    // TODO: Refactor this part, it should be picked up by the window messages.
    if (gGame.hasFocus())
    {
        if ((GetAsyncKeyState('W') & 0x8000) != 0)
            player.pressed |= static_cast<uint32_t>(Buttons::UP);
        if ((GetAsyncKeyState('A') & 0x8000) != 0)
            player.pressed |= static_cast<uint32_t>(Buttons::LEFT);
        if ((GetAsyncKeyState('S') & 0x8000) != 0)
            player.pressed |= static_cast<uint32_t>(Buttons::DOWN);
        if ((GetAsyncKeyState('D') & 0x8000) != 0)
            player.pressed |= static_cast<uint32_t>(Buttons::RIGHT);
    }

    Vector2i curDirection = gSnakes.getDirection(player.snakeId);
    Vector2i newDirection = curDirection;

    if (curDirection == DIR_UP || curDirection == DIR_DOWN)
    {
        if (player.isButtonPressed(Buttons::LEFT))
            newDirection = DIR_LEFT;
        else if (player.isButtonPressed(Buttons::RIGHT))
            newDirection = DIR_RIGHT;
    }
    else if (curDirection == DIR_RIGHT || curDirection == DIR_LEFT)
    {
        if (player.isButtonPressed(Buttons::UP))
            newDirection = DIR_UP;
        else if (player.isButtonPressed(Buttons::DOWN))
            newDirection = DIR_DOWN;
    }
    else if (curDirection == DIR_NONE)
    {
        if (player.isButtonPressed(Buttons::UP))
            newDirection = DIR_UP;
        else if (player.isButtonPressed(Buttons::DOWN))
            newDirection = DIR_DOWN;
        else if (player.isButtonPressed(Buttons::LEFT))
            newDirection = DIR_LEFT;
        else if (player.isButtonPressed(Buttons::RIGHT))
            newDirection = DIR_RIGHT;
    }

    if (newDirection != curDirection)
    {
        if (gNetwork.getMode() == NetworkMode::CLIENT)
        {
            MessageClientSnakeDirection msgSnakeDir;
            msgSnakeDir.newDirection = newDirection;

            gNetwork.sendMessage(msgSnakeDir);
        }
        else
        {
            gSnakes.setDirection(player.snakeId, newDirection);
        }
    }
}

void Players::draw(Painter& painter) const
{
    painter.rect(
        PLAYER_LIST_X, PLAYER_LIST_Y, PLAYER_LIST_W, PLAYER_LIST_H,
        { 0, 255, 0 });
    painter.textCentered(
        "  Players  ", PLAYER_LIST_X, PLAYER_LIST_Y - 8, PLAYER_LIST_W, 20);

    std::vector<Player> players(_players.begin(), _players.end());

    auto getSortValue = [this](const Player& player) -> int32_t {
        if (player.id == INVALID_PLAYER_ID)
            return -2;
        if (player.snakeId == INVALID_SNAKE_ID)
            return -1;
        if (gSnakes.getData(player.snakeId).state == SnakeState::DEAD)
            return -1;
        return getScore(player.id);
    };

    std::sort(
        players.begin(), players.end(),
        [&getSortValue](const Player& a, const Player& b) -> bool {
            return getSortValue(a) > getSortValue(b);
        });

    int32_t offsetY = PLAYER_LIST_Y + 20;
    for (auto& player : players)
    {
        if (player.id == INVALID_PLAYER_ID)
            break;

        painter.filledRect(
            PLAYER_LIST_X + 10, offsetY, TILE_SIZE_W, TILE_SIZE_H,
            player.color);

        Color textColor = COLOR_GREEN;
        if (player.snakeId == INVALID_SNAKE_ID
            || gSnakes.getData(player.snakeId).state == SnakeState::DEAD)
        {
            textColor = COLOR_RED;
        }

        std::string score = std::to_string(getScore(player.id));
        painter.textRight(
            score, PLAYER_LIST_X + 10, offsetY, PLAYER_LIST_W - 20, 20,
            textColor);

        painter.text(
            player.name, PLAYER_LIST_X + 10 + TILE_SIZE_W + 10, offsetY,
            textColor);

        offsetY = offsetY + 20;
    }
}

bool Players::isValidPlayer(PlayerId playerId) const
{
    if (playerId >= MAX_PLAYERS || _players[playerId].id == INVALID_PLAYER_ID)
        return false;

    return true;
}

const Player& Players::getLocalPlayer() const
{
    assert(_localId != INVALID_PLAYER_ID);
    return _players[_localId];
}

const Player& Players::getPlayer(PlayerId playerId) const
{
    return _players[playerId];
}

const size_t Players::count() const
{
    size_t res = 0;
    for (auto& player : _players)
    {
        if (player.id != INVALID_PLAYER_ID)
            res++;
    }
    return res;
}
