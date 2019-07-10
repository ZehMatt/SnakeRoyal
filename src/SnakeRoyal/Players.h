#pragma once

#include "Types.h"

#include "Config.h"
#include "Player.h"

class Painter;

class Players
{
    std::array<Player, MAX_PLAYERS> _players;
    PlayerId _localId = INVALID_PLAYER_ID;

public:
    PlayerId createLocalPlayer(const char* name, SnakeId snakeId);
    PlayerId createPlayer(const char* name, SnakeId snakeId);
    bool removePlayer(PlayerId playerId);
    void setPlayerById(PlayerId playerId, const Player& data);
    void setLocalPlayerId(PlayerId playerId);
    void setSnake(PlayerId playerId, SnakeId snakeId);
    uint32_t getScore(PlayerId playerId) const;
    Color getColor(PlayerId playerId) const;

    void update();
    void draw(Painter& painter) const;

    bool isValidPlayer(PlayerId) const;
    const Player& getLocalPlayer() const;
    const Player& getPlayer(PlayerId playerId) const;

    const size_t count() const;
};

extern Players gPlayers;