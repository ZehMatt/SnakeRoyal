#pragma once

#include "Socket.h"
#include "NetworkMessage.h"
#include "Game.h"
#include "Buffer.h"

#include <map>
#include <functional>

enum class NetworkMode
{
    NONE = 0,
    CLIENT,
    SERVER,
};

static constexpr const char* NETWORK_DEFAULT_HOST = "0.0.0.0";
static constexpr uint16_t NETWORK_DEFAULT_PORT = 11754;
static constexpr size_t NETWORK_BUFFER_SIZE = 1024 * 64;

struct Connection
{
    std::unique_ptr<ITcpSocket> sock;
    SocketStatus lastStatus = SocketStatus::CLOSED;
    PlayerId playerId = INVALID_PLAYER_ID;
    Buffer recvBuffer;
    Buffer sendBuffer;
};

class Network
{
    NetworkMode _mode = NetworkMode::NONE;

private:     // Server specific data.
    std::unique_ptr<ITcpSocket> _listenSocket;
    std::vector<std::unique_ptr<Connection>> _connections;

private:     // Client specific data.
    std::unique_ptr<ITcpSocket> _clientSocket;
    std::unique_ptr<Connection> _serverConnection;

    // Last server known server tick, as a client we can not run beyond that.
    uint32_t _serverTick = 0;

    // Last measured client ping.
    uint32_t _currentPing = 0;

    // This is used as a queue where events have to be executed at a 
    // specific tick. The order of events is same order as packets
    // arrive which is on TCP always ordered per tick.
    std::multimap<uint32_t, std::function<void()>> _tickQueue;

public:
    Network();
    ~Network();

    void startServer(
        const std::string& address, uint16_t port = NETWORK_DEFAULT_PORT);
    void startClient(
        const std::string& address, uint16_t port = NETWORK_DEFAULT_PORT);
    void update();
    void flush();

    uint32_t getCurrentPing() const
    {
        return _currentPing;
    }

    NetworkMode getMode() const
    {
        return _mode;
    }

    uint32_t getServerTick() const
    {
        return _serverTick;
    }

    // Send message to specified connection.
    template<typename T>
    void sendMessage(const T& message, std::unique_ptr<Connection>& connection)
    {
        Buffer messageBuffer;
        message.serialize(messageBuffer);

        MessageHeader_t header;
        header.signature = NETWORK_MESSAGE_SIGNATURE;
        header.size = messageBuffer.size();
        header.msg = static_cast<NetworkMessage>(T::MESSAGE_ID);

        auto& buffer = connection->sendBuffer;
        header.serialize(buffer);
        buffer.write(messageBuffer);
    }

    // Broadcast a message.
    template<typename T> void sendMessage(const T& message)
    {
        if (getMode() == NetworkMode::CLIENT)
        {
            sendMessage(message, _serverConnection);
        }
        else
        {
            for (auto& connection : _connections)
            {
                sendMessage(message, connection);
            }
        }
    }

    template<typename T, typename F>
    bool dispatchMessage(
        Buffer& buffer, std::unique_ptr<Connection>& connection, F fn)
    {
        T msg;
        if (!msg.deserialize(buffer))
        {
            assert(false);
            return false;
        }
        (this->*fn)(connection, msg);
        return true;
    }

    void processQueue();

private: // Common
    void updateServer();
    void updateClient();
    void flushConnection(std::unique_ptr<Connection>& connection);
    bool processConnection(std::unique_ptr<Connection>& connection);
    bool processPackets(std::unique_ptr<Connection>& connection);

private: // Server events
    void onClientConnected(std::unique_ptr<Connection>& clientConnection);
    void onClientDisconnected(std::unique_ptr<Connection>& clientConnection);

private: // Server message dispatchers.
    void onClientMessageHello(
        std::unique_ptr<Connection>& clientConnection,
        const MessageClientHello& msg);
    void onClientSnakeDirection(
        std::unique_ptr<Connection>& clientConnection,
        const MessageClientSnakeDirection& msg);
    void onClientMessagePing(
        std::unique_ptr<Connection>& connection, const MessageClientPing& msg);

private: // Client events.
    void onConnected();
    void onDisconnected();

private: // Client message dispatchers.
    void onServerMessageTick(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerTick& msg);
    void onServerMessagePlayerLocalId(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerLocalPlayerId& msg);
    void onServerMessageSnakeList(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerSnakeList& msg);
    void onServerMessagePlayerList(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerPlayerList& msg);
    void onServerMessageState(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerState& msg);
    void onServerMessageSnakeDirection(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerSnakeDirection& msg);
    void onServerMessageRoundRestart(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerRoundRestart& msg);
    void onServerMessageRoundState(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerRoundState& msg);
    void onServerMessagePlayerDisconnected(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerPlayerDisconnected& msg);
    void onServerMessageRoundStart(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerRoundStart& msg);
    void onServerMessagePong(
        std::unique_ptr<Connection>& serverConnection,
        const MessageServerPong& msg);
};

extern Network gNetwork;
