#pragma once

#include "Types.h"
#include "Buffer.h"
#include "Serialization.h"
#include "TileMap.h"
#include "Players.h"
#include "Game.h"
#include "Snake.h"

enum NetworkMessage : uint16_t
{
    BASE = 0,

    CLIENT_HELLO,
    CLIENT_SNAKE_DIRECTION,
    CLIENT_PING,

    SERVER_PONG,
    SERVER_PLAYER_LIST,
    SERVER_PLAYER_DISCONNECTED,
    SERVER_LOCAL_PLAYER_ID,
    SERVER_STATE,
    SERVER_TICK,
    SERVER_SNAKE_LIST,
    SERVER_SNAKE_DIRECTION,
    SERVER_ROUND_STATE,
    SERVER_ROUND_RESTART,
    SERVER_ROUND_START,
    SERVER_ASSIGN_SNAKE,
};

static constexpr uint32_t NETWORK_MESSAGE_SIGNATURE = 0xDEADBEEF;
static constexpr uint32_t NETWORK_VERSION = 1;

template<typename T, NetworkMessage MSG = NetworkMessage::BASE>
struct MessageBasePOD
{
    enum
    {
        MESSAGE_ID = MSG
    };

    bool serialize(Buffer& buffer) const
    {
        return buffer.write(reinterpret_cast<const T&>(*this)) == sizeof(T);
    }

    bool deserialize(Buffer& buffer)
    {
        return buffer.read(reinterpret_cast<T&>(*this)) == sizeof(T);
    }
};

template<typename T, NetworkMessage MSG = NetworkMessage::BASE>
struct MessageBaseComplex
{
    enum
    {
        MESSAGE_ID = MSG
    };

    template<typename T>
    bool serializeField(Buffer& buffer, const T& data) const
    {
        return Serializer<T>::serialize(buffer, data);
    }

    template<typename T> bool deserializeField(Buffer& buffer, T& data)
    {
        return Serializer<T>::deserialize(buffer, data);
    }
};

struct MessageHeader_t : MessageBasePOD<MessageHeader_t>
{
    uint32_t signature;
    uint32_t size;
    NetworkMessage msg;
};

struct MessageClientHello
    : MessageBasePOD<MessageClientHello, NetworkMessage::CLIENT_HELLO>
{
    uint32_t version;
    char name[128];
};

struct MessageServerTick
    : MessageBasePOD<MessageServerTick, NetworkMessage::SERVER_TICK>
{
    uint32_t tick;
};

struct MessageServerPlayerList : MessageBasePOD<
                                     MessageServerPlayerList,
                                     NetworkMessage::SERVER_PLAYER_LIST>
{
    uint32_t tick;
    std::array<Player, MAX_PLAYERS> players;
};

struct MessageServerLocalPlayerId : MessageBasePOD<
                                        MessageServerLocalPlayerId,
                                        NetworkMessage::SERVER_LOCAL_PLAYER_ID>
{
    PlayerId playerId;
};

struct MessageServerState
    : MessageBasePOD<MessageServerState, NetworkMessage::SERVER_STATE>
{
    uint32_t tick;
    uint64_t randState;
    RoundData_t roundData;
    uint32_t timeout;
    std::array<TileData_t, TILE_MAP_SIZE> tiles;
};

struct MessageServerSnakeList : MessageBaseComplex<
                                    MessageServerSnakeList,
                                    NetworkMessage::SERVER_SNAKE_LIST>
{
    uint32_t tick;
    std::array<Snake, MAX_PLAYERS> snakes;

    bool serialize(Buffer& buffer) const
    {
        serializeField(buffer, tick);

        uint16_t snakeCount = static_cast<uint16_t>(snakes.size());
        serializeField(buffer, snakeCount);
        for (uint16_t i = 0; i < snakeCount; i++)
        {
            const Snake& snake = snakes[i];
            const SnakeBase& baseData = reinterpret_cast<const SnakeBase&>(
                snake);

            buffer.write(baseData);

            uint16_t tailCount = static_cast<uint16_t>(snake.pieces.size());
            serializeField(buffer, tailCount);

            for (uint16_t y = 0; y < tailCount; y++)
            {
                buffer.write(snake.pieces[y]);
            }
        }
        return true;
    }

    bool deserialize(Buffer& buffer)
    {
        buffer.read(tick);

        uint16_t snakeCount = 0;
        deserializeField(buffer, snakeCount);
        for (uint16_t i = 0; i < snakeCount; i++)
        {
            Snake& snake = snakes[i];
            SnakeBase& baseData = reinterpret_cast<SnakeBase&>(snake);

            buffer.read(baseData);

            uint16_t tailCount = 0;
            deserializeField(buffer, tailCount);

            snake.pieces.resize(tailCount);
            for (uint16_t y = 0; y < tailCount; y++)
            {
                buffer.read(snake.pieces[y]);
            }
        }
        return true;
    }
};

struct MessageClientSnakeDirection : MessageBasePOD<
                                         MessageClientSnakeDirection,
                                         NetworkMessage::CLIENT_SNAKE_DIRECTION>
{
    Vector2i newDirection;
};

struct MessageServerSnakeDirection : MessageBasePOD<
                                         MessageServerSnakeDirection,
                                         NetworkMessage::SERVER_SNAKE_DIRECTION>
{
    uint32_t tick;
    SnakeId snakeId;
    Vector2i newDirection;
};

struct MessageServerRoundState : MessageBasePOD<
                                     MessageServerRoundState,
                                     NetworkMessage::SERVER_ROUND_STATE>
{
    uint32_t tick;
    RoundState state;
    uint32_t delay;
};

struct MessageServerRoundRestart : MessageBasePOD<
                                       MessageServerRoundRestart,
                                       NetworkMessage::SERVER_ROUND_RESTART>
{
    uint32_t tick;
    uint32_t delay;
};

struct MessageServerRoundStart : MessageBasePOD<
                                     MessageServerRoundStart,
                                     NetworkMessage::SERVER_ROUND_START>
{
    uint32_t tick;
};

struct MessageServerPlayerDisconnected
    : MessageBasePOD<
          MessageServerPlayerDisconnected,
          NetworkMessage::SERVER_PLAYER_DISCONNECTED>
{
    uint32_t tick;
    PlayerId playerId;
};

struct MessageClientPing
    : MessageBasePOD<MessageClientPing, NetworkMessage::CLIENT_PING>
{
    double timestamp;
};

struct MessageServerPong
    : MessageBasePOD<MessageServerPong, NetworkMessage::SERVER_PONG>
{
    double timestamp;
};