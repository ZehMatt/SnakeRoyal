#include "Network.h"
#include "NetworkMessage.h"
#include "Logging.h"
#include "Utils.h"
#include "Snakes.h"

Network gNetwork;

Network::Network()
{
    InitializeWSA();
}

Network::~Network()
{
    DisposeWSA();
}

void Network::startServer(const std::string& address, uint16_t port)
{
    logPrint("%s(%s, %u)\n", __FUNCTION__, address.c_str(), port);

    _mode = NetworkMode::SERVER;

    _listenSocket = CreateTcpSocket();
    _listenSocket->Listen(address, port);

    logPrint("Ready for clients...\n");
}

void Network::startClient(const std::string& address, uint16_t port)
{
    logPrint("%s(%s, %u)\n", __FUNCTION__, address.c_str(), port);

    _mode = NetworkMode::CLIENT;

    _serverConnection = std::make_unique<Connection>();
    _serverConnection->sock = CreateTcpSocket();
    _serverConnection->lastStatus = _serverConnection->sock->GetStatus();

    _serverConnection->sock->ConnectAsync(address, port);
}

void Network::update()
{
    if (_mode == NetworkMode::CLIENT)
        updateClient();
    else if (_mode == NetworkMode::SERVER)
        updateServer();

    processQueue();
}

void Network::flush()
{
    if (_mode == NetworkMode::SERVER)
    {
        for (auto& connection : _connections)
        {
            flushConnection(connection);
        }
    }
    else if (_mode == NetworkMode::CLIENT)
    {
        flushConnection(_serverConnection);
    }
}

void Network::processQueue()
{
    for (auto it = _connections.begin(); it != _connections.end();)
    {
        auto& connection = *it;

        if (!processConnection(connection))
        {
            onClientDisconnected(connection);
            it = _connections.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void Network::updateServer()
{
    std::unique_ptr<ITcpSocket> clientSock = _listenSocket->Accept();
    if (clientSock != nullptr)
    {
        auto connection = std::make_unique<Connection>();
        connection->playerId = INVALID_PLAYER_ID;
        connection->sock = std::move(clientSock);

        onClientConnected(connection);

        _connections.push_back(std::move(connection));
    }

    MessageServerTick msgTick;
    msgTick.tick = gGame.getTick();
    sendMessage(msgTick);
}

void Network::updateClient()
{
    SocketStatus currentStatus = _serverConnection->sock->GetStatus();

    // Check current socket state against the last known to print out whats happening.
    if (_serverConnection->lastStatus != currentStatus)
    {
        switch (currentStatus)
        {
            case SocketStatus::CLOSED:
                logPrint("Connection closed\n");
                break;
            case SocketStatus::RESOLVING:
                logPrint("Resolving host...\n");
                break;
            case SocketStatus::CONNECTING:
                logPrint("Connecting...\n");
                break;
            case SocketStatus::CONNECTED:
                onConnected();
                break;
        }
        _serverConnection->lastStatus = currentStatus;
    }

    if (currentStatus != SocketStatus::CONNECTED)
    {
        return;
    }

    if (!processConnection(_serverConnection))
    {
        onDisconnected();
    }

    MessageClientPing msgPing;
    msgPing.timestamp = Utils::getTime();
    sendMessage(msgPing);

    auto itRange = _tickQueue.equal_range(gGame.getTick());
    for (auto it = itRange.first; it != itRange.second;)
    {
        it->second();
        it = _tickQueue.erase(it);
    }
}

void Network::flushConnection(std::unique_ptr<Connection>& connection)
{
    auto& buffer = connection->sendBuffer;
    if (buffer.empty())
        return;

    connection->sock->SendData(buffer.base(), buffer.size());
    buffer.clear();
}

bool Network::processConnection(std::unique_ptr<Connection>& connection)
{
    uint8_t tempBuffer[NETWORK_BUFFER_SIZE]{};
    size_t received = 0;

    auto readStatus = connection->sock->ReceiveData(
        tempBuffer, sizeof(tempBuffer), &received);
    if (readStatus == SocketReadStatus::SUCCESS)
    {
        auto& buffer = connection->recvBuffer;
        buffer.seek(0, BufferSeek::END);
        buffer.write(tempBuffer, received);
    }
    else if (readStatus == SocketReadStatus::DISCONNECTED)
    {
        return false;
    }

    if (!processPackets(connection))
    {
        return false;
    }

    return true;
}

bool Network::processPackets(std::unique_ptr<Connection>& connection)
{
    auto& buffer = connection->recvBuffer;
    buffer.seek(0);

    size_t processed = 0;
    while (!buffer.eob())
    {
        MessageHeader_t header;
        if (buffer.read(header) < sizeof(header))
        {
            break;
        }

        if (header.signature != NETWORK_MESSAGE_SIGNATURE)
        {
            logPrint("Invalid signature!\n");
            return false;
        }

        if (buffer.offset() + header.size > buffer.size())
        {
            // Need more data.
            break;
        }

        switch (header.msg)
        {
            // Server.
            //////////////////////////////////////////////////////////////////////////
            case MessageClientHello::MESSAGE_ID:
                if (!dispatchMessage<MessageClientHello>(
                        buffer, connection, &Network::onClientMessageHello))
                    return false;
                break;
            case MessageClientSnakeDirection::MESSAGE_ID:
                if (!dispatchMessage<MessageClientSnakeDirection>(
                        buffer, connection, &Network::onClientSnakeDirection))
                    return false;
                break;
            case MessageClientPing::MESSAGE_ID:
                if (!dispatchMessage<MessageClientPing>(
                        buffer, connection, &Network::onClientMessagePing))
                    return false;
                break;
            // Client
            //////////////////////////////////////////////////////////////////////////
            case MessageServerTick::MESSAGE_ID:
                if (!dispatchMessage<MessageServerTick>(
                        buffer, _serverConnection,
                        &Network::onServerMessageTick))
                    return false;
                break;
            case MessageServerState::MESSAGE_ID:
                if (!dispatchMessage<MessageServerState>(
                        buffer, _serverConnection,
                        &Network::onServerMessageState))
                    return false;
                break;
            case MessageServerLocalPlayerId::MESSAGE_ID:
                if (!dispatchMessage<MessageServerLocalPlayerId>(
                        buffer, _serverConnection,
                        &Network::onServerMessagePlayerLocalId))
                    return false;
                break;
            case MessageServerPlayerList::MESSAGE_ID:
                if (!dispatchMessage<MessageServerPlayerList>(
                        buffer, _serverConnection,
                        &Network::onServerMessagePlayerList))
                    return false;
                break;
            case MessageServerSnakeList::MESSAGE_ID:
                if (!dispatchMessage<MessageServerSnakeList>(
                        buffer, _serverConnection,
                        &Network::onServerMessageSnakeList))
                    return false;
                break;
            case MessageServerSnakeDirection::MESSAGE_ID:
                if (!dispatchMessage<MessageServerSnakeDirection>(
                        buffer, _serverConnection,
                        &Network::onServerMessageSnakeDirection))
                    return false;
                break;
            case MessageServerRoundRestart::MESSAGE_ID:
                if (!dispatchMessage<MessageServerRoundRestart>(
                        buffer, _serverConnection,
                        &Network::onServerMessageRoundRestart))
                    return false;
                break;
            case MessageServerRoundState::MESSAGE_ID:
                if (!dispatchMessage<MessageServerRoundState>(
                        buffer, _serverConnection,
                        &Network::onServerMessageRoundState))
                    return false;
                break;
            case MessageServerRoundStart::MESSAGE_ID:
                if (!dispatchMessage<MessageServerRoundStart>(
                        buffer, _serverConnection,
                        &Network::onServerMessageRoundStart))
                    return false;
                break;
            case MessageServerPlayerDisconnected::MESSAGE_ID:
                if (!dispatchMessage<MessageServerPlayerDisconnected>(
                        buffer, _serverConnection,
                        &Network::onServerMessagePlayerDisconnected))
                    return false;
                break;
            case MessageServerPong::MESSAGE_ID:
                if (!dispatchMessage<MessageServerPong>(
                        buffer, _serverConnection,
                        &Network::onServerMessagePong))
                    return false;
                break;
            default:
                logPrint("Unhandled network message: %u\n", header.msg);
                assert(false);
                break;
        }

        processed = buffer.offset();
    }

    if (processed > 0)
    {
        buffer.seek(0);
        buffer.erase(processed);
    }

    return true;
}

void Network::onClientConnected(std::unique_ptr<Connection>& connection)
{
    logPrint("Client connected: %s\n", connection->sock->GetHostName());
}

void Network::onClientDisconnected(std::unique_ptr<Connection>& connection)
{
    logPrint("Client disconnected: %s\n", connection->sock->GetHostName());

    PlayerId playerId = connection->playerId;
    if (playerId != INVALID_PLAYER_ID)
    {
        const Player& player = gPlayers.getPlayer(playerId);
        if (player.snakeId != INVALID_SNAKE_ID)
        {
            gSnakes.remove(player.snakeId);
        }
        gPlayers.removePlayer(playerId);

        MessageServerPlayerDisconnected msgDisconnected;
        msgDisconnected.tick = gGame.getTick();
        msgDisconnected.playerId = playerId;
        sendMessage(msgDisconnected);
    }

    if (gPlayers.count() == 0)
    {
        gGame.setRoundState(RoundState::IDLE, 0);
    }
}

void Network::onClientMessageHello(
    std::unique_ptr<Connection>& connection, const MessageClientHello& msg)
{
    bool isFirstPlayer = gPlayers.count() == 0;

    SnakeId newSnakeId = INVALID_SNAKE_ID;

    // Create new player.
    PlayerId newPlayerId = gPlayers.createPlayer(msg.name, newSnakeId);

    // Create new snake.
    if (isFirstPlayer == false && gGame.getRoundState() == RoundState::RUNNING)
    {
        newSnakeId = gSnakes.create(newPlayerId, 0, 0);
        gPlayers.setSnake(newPlayerId, newSnakeId);
    }

    connection->playerId = newPlayerId;

    // Send new player list.
    {
        MessageServerPlayerList msgPlayerList;
        msgPlayerList.tick = gGame.getTick();

        for (PlayerId id = 0; id < MAX_PLAYERS; id++)
        {
            msgPlayerList.players[id] = gPlayers.getPlayer(id);
        }

        sendMessage(msgPlayerList);
    }

    // Send client his local player id.
    {
        MessageServerLocalPlayerId msgPlayerId;
        msgPlayerId.playerId = newPlayerId;

        sendMessage(msgPlayerId, connection);
    }

    // Send current state to client.
    {
        MessageServerState msgServerState;
        msgServerState.randState = gGame.getRandState();
        msgServerState.tick = gGame.getTick();
        msgServerState.tiles = gTileMap.getData();
        msgServerState.roundData = gGame.getRoundData();

        sendMessage(msgServerState, connection);
    }

    // Send all snakes to client.
    {
        MessageServerSnakeList msgServerSnakeList;
        msgServerSnakeList.tick = gGame.getTick();
        msgServerSnakeList.snakes = gSnakes.getSnakes();
        sendMessage(msgServerSnakeList);
    }

    // Restart round, if its already restarting it resets the timeout.
    if (isFirstPlayer || gGame.getRoundState() == RoundState::RESTARTING)
    {
        gGame.restart(GAME_ROUND_RESTART_TICKS);
    }
}

void Network::onClientSnakeDirection(
    std::unique_ptr<Connection>& connection,
    const MessageClientSnakeDirection& msg)
{
    const Player& player = gPlayers.getPlayer(connection->playerId);

    gSnakes.setDirection(player.snakeId, msg.newDirection);

    MessageServerSnakeDirection msgSnakeDir;
    msgSnakeDir.tick = gGame.getTick();
    msgSnakeDir.snakeId = player.snakeId;
    msgSnakeDir.newDirection = msg.newDirection;

    sendMessage(msgSnakeDir);
}

void Network::onClientMessagePing(
    std::unique_ptr<Connection>& connection, const MessageClientPing& msg)
{
    MessageServerPong msgPong;
    msgPong.timestamp = msg.timestamp;
    sendMessage(msgPong, connection);
}

void Network::onConnected()
{
    logPrint("Connected.\n");

    MessageClientHello msg;
    msg.version = NETWORK_VERSION;
    Utils::getUsername(msg.name, sizeof(msg.name));

    sendMessage(msg, _serverConnection);
}

void Network::onDisconnected()
{
    logPrint("Disconnected.\n");

    _mode = NetworkMode::NONE;
    _tickQueue.clear();
    _serverTick = 0;
}

void Network::onServerMessageTick(
    std::unique_ptr<Connection>& serverConnection, const MessageServerTick& msg)
{
    _serverTick = msg.tick;
}

void Network::onServerMessagePlayerLocalId(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerLocalPlayerId& msg)
{
    gPlayers.setLocalPlayerId(msg.playerId);
}

void Network::onServerMessageSnakeList(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerSnakeList& msg)
{
    _tickQueue.emplace(msg.tick, [msg]() -> void {
        for (SnakeId id = 0; id < msg.snakes.size(); id++)
        {
            const Snake& newData = msg.snakes[id];

            Snake& curData = gSnakes.getData(id);
            curData = newData;
        }
    });
}

void Network::onServerMessagePlayerList(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerPlayerList& msg)
{
    _tickQueue.emplace(msg.tick, [msg]() -> void {
        PlayerId id = 0;
        for (auto& playerData : msg.players)
        {
            gPlayers.setPlayerById(id, playerData);
            id++;
        }
    });
}

void Network::onServerMessageState(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerState& msg)
{
    gTileMap.getData() = msg.tiles;
    gGame.setTick(msg.tick);
    gGame.setRandState(msg.randState);
    gGame.getRoundData() = msg.roundData;
}

void Network::onServerMessageSnakeDirection(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerSnakeDirection& msg)
{
    _tickQueue.emplace(msg.tick, [msg]() -> void {
        gSnakes.setDirection(msg.snakeId, msg.newDirection);
    });
}

void Network::onServerMessageRoundRestart(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerRoundRestart& msg)
{
    _tickQueue.emplace(msg.tick, [msg]() -> void { gGame.restart(msg.delay); });
}

void Network::onServerMessageRoundState(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerRoundState& msg)
{
    _tickQueue.emplace(msg.tick, [msg]() -> void {
        gGame.setRoundState(msg.state, msg.delay);
    });
}

void Network::onServerMessagePlayerDisconnected(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerPlayerDisconnected& msg)
{
    _tickQueue.emplace(msg.tick, [msg]() -> void {
        PlayerId playerId = msg.playerId;
        if (playerId != INVALID_PLAYER_ID)
        {
            const Player& player = gPlayers.getPlayer(playerId);
            if (player.snakeId != INVALID_SNAKE_ID)
            {
                gSnakes.remove(player.snakeId);
            }
            gPlayers.removePlayer(playerId);
        }
    });
}

void Network::onServerMessageRoundStart(
    std::unique_ptr<Connection>& serverConnection,
    const MessageServerRoundStart& msg)
{
    _tickQueue.emplace(msg.tick, [msg]() -> void { gGame.startRound(); });
}

void Network::onServerMessagePong(
    std::unique_ptr<Connection>& serverConnection, const MessageServerPong& msg)
{
    double currentTime = Utils::getTime();
    double delta = currentTime - msg.timestamp;

    _currentPing = static_cast<uint32_t>(delta * 1000.0);
}
