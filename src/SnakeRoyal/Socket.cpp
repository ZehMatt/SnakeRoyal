#include <chrono>
#include <cmath>
#include <cstring>
#include <future>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define LAST_SOCKET_ERROR() WSAGetLastError()
#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif
#define FLAG_NO_PIPE 0

#include "Socket.h"

static constexpr auto CONNECT_TIMEOUT = std::chrono::milliseconds(3000);
static bool _wsaInitialised = false;

class SocketException : public std::runtime_error
{
public:
    explicit SocketException(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class Socket
{
protected:
    static bool ResolveAddress(
        const std::string& address,
        uint16_t port,
        sockaddr_storage* ss,
        socklen_t* ss_len)
    {
        return ResolveAddress(AF_UNSPEC, address, port, ss, ss_len);
    }

    static bool ResolveAddressIPv4(
        const std::string& address,
        uint16_t port,
        sockaddr_storage* ss,
        socklen_t* ss_len)
    {
        return ResolveAddress(AF_INET, address, port, ss, ss_len);
    }

    static bool SetNonBlocking(SOCKET socket, bool on)
    {
#ifdef _WIN32
        u_long nonBlocking = on;
        return ioctlsocket(socket, FIONBIO, &nonBlocking) == 0;
#else
        int32_t flags = fcntl(socket, F_GETFL, 0);
        return fcntl(
                   socket, F_SETFL,
                   on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK))
               == 0;
#endif
    }

    static bool SetOption(SOCKET socket, int32_t a, int32_t b, bool value)
    {
        int32_t ivalue = value ? 1 : 0;
        return setsockopt(socket, a, b, (const char*)&ivalue, sizeof(ivalue))
               == 0;
    }

private:
    static bool ResolveAddress(
        int32_t family,
        const std::string& address,
        uint16_t port,
        sockaddr_storage* ss,
        socklen_t* ss_len)
    {
        std::string serviceName = std::to_string(port);

        addrinfo hints = {};
        hints.ai_family = family;
        if (address.empty())
        {
            hints.ai_flags = AI_PASSIVE;
        }

        addrinfo* result = nullptr;
        int errorcode = getaddrinfo(
            address.empty() ? nullptr : address.c_str(), serviceName.c_str(),
            &hints, &result);
        if (errorcode != 0)
        {
            return false;
        }
        if (result == nullptr)
        {
            return false;
        }
        else
        {
            std::memcpy(ss, result->ai_addr, result->ai_addrlen);
            *ss_len = (socklen_t)result->ai_addrlen;
            freeaddrinfo(result);
            return true;
        }
    }
};

class TcpSocket final : public ITcpSocket, protected Socket
{
private:
    SocketStatus _status = SocketStatus::CLOSED;
    uint16_t _listeningPort = 0;
    SOCKET _socket = INVALID_SOCKET;

    std::string _hostName;
    std::future<void> _connectFuture;
    std::string _error;

public:
    TcpSocket() = default;

    ~TcpSocket() override
    {
        if (_connectFuture.valid())
        {
            _connectFuture.wait();
        }
        CloseSocket();
    }

    SocketStatus GetStatus() const override
    {
        return _status;
    }

    const char* GetError() const override
    {
        return _error.empty() ? nullptr : _error.c_str();
    }

    void Listen(uint16_t port) override
    {
        Listen("", port);
    }

    void Listen(const std::string& address, uint16_t port) override
    {
        if (_status != SocketStatus::CLOSED)
        {
            throw std::runtime_error("Socket not closed.");
        }

        sockaddr_storage ss{};
        socklen_t ss_len;
        if (!ResolveAddress(address, port, &ss, &ss_len))
        {
            throw SocketException("Unable to resolve address.");
        }

        // Create the listening socket
        _socket = socket(ss.ss_family, SOCK_STREAM, IPPROTO_TCP);
        if (_socket == INVALID_SOCKET)
        {
            throw SocketException("Unable to create socket.");
        }

        // Turn off IPV6_V6ONLY so we can accept both v4 and v6 connections
        int32_t value = 0;
        if (setsockopt(
                _socket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&value,
                sizeof(value))
            != 0)
        {
            printf("IPV6_V6ONLY failed. %d\n", LAST_SOCKET_ERROR());
        }

        value = 1;
        if (setsockopt(
                _socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&value,
                sizeof(value))
            != 0)
        {
            printf("SO_REUSEADDR failed. %d\n", LAST_SOCKET_ERROR());
        }

        try
        {
            // Bind to address:port and listen
            if (bind(_socket, (sockaddr*)&ss, ss_len) != 0)
            {
                throw SocketException("Unable to bind to socket.");
            }
            if (listen(_socket, SOMAXCONN) != 0)
            {
                throw SocketException("Unable to listen on socket.");
            }

            if (!SetNonBlocking(_socket, true))
            {
                throw SocketException("Failed to set non-blocking mode.");
            }
        }
        catch (const std::exception&)
        {
            CloseSocket();
            throw;
        }

        _listeningPort = port;
        _status = SocketStatus::LISTENING;
    }

    std::unique_ptr<ITcpSocket> Accept() override
    {
        if (_status != SocketStatus::LISTENING)
        {
            throw std::runtime_error("Socket not listening.");
        }
        struct sockaddr_storage client_addr
        {
        };
        socklen_t client_len = sizeof(struct sockaddr_storage);

        std::unique_ptr<ITcpSocket> tcpSocket;
        SOCKET socket = accept(
            _socket, (struct sockaddr*)&client_addr, &client_len);
        if (socket == INVALID_SOCKET)
        {
            if (LAST_SOCKET_ERROR() != EWOULDBLOCK)
            {
                printf("Failed to accept client.\n");
            }
        }
        else
        {
            if (!SetNonBlocking(socket, true))
            {
                closesocket(socket);
                printf("Failed to set non-blocking mode.\n");
            }
            else
            {
                char hostName[NI_MAXHOST];
                int32_t rc = getnameinfo(
                    (struct sockaddr*)&client_addr, client_len, hostName,
                    sizeof(hostName), nullptr, 0,
                    NI_NUMERICHOST | NI_NUMERICSERV);
                SetOption(socket, IPPROTO_TCP, TCP_NODELAY, true);
                if (rc == 0)
                {
                    tcpSocket = std::unique_ptr<ITcpSocket>(
                        new TcpSocket(socket, hostName));
                }
                else
                {
                    tcpSocket = std::unique_ptr<ITcpSocket>(
                        new TcpSocket(socket, ""));
                }
            }
        }
        return tcpSocket;
    }

    void Connect(const std::string& address, uint16_t port) override
    {
        if (_status != SocketStatus::CLOSED)
        {
            throw std::runtime_error("Socket not closed.");
        }

        try
        {
            // Resolve address
            _status = SocketStatus::RESOLVING;

            sockaddr_storage ss{};
            socklen_t ss_len;
            if (!ResolveAddress(address, port, &ss, &ss_len))
            {
                throw SocketException("Unable to resolve address.");
            }

            _status = SocketStatus::CONNECTING;
            _socket = socket(ss.ss_family, SOCK_STREAM, IPPROTO_TCP);
            if (_socket == INVALID_SOCKET)
            {
                throw SocketException("Unable to create socket.");
            }

            SetOption(_socket, IPPROTO_TCP, TCP_NODELAY, true);
            if (!SetNonBlocking(_socket, true))
            {
                throw SocketException("Failed to set non-blocking mode.");
            }

            // Connect
            int32_t connectResult = connect(_socket, (sockaddr*)&ss, ss_len);
            if (connectResult != SOCKET_ERROR
                || (LAST_SOCKET_ERROR() != EINPROGRESS
                    && LAST_SOCKET_ERROR() != EWOULDBLOCK))
            {
                throw SocketException("Failed to connect.");
            }

            auto connectStartTime = std::chrono::system_clock::now();

            int32_t error = 0;
            socklen_t len = sizeof(error);
            if (getsockopt(_socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len)
                != 0)
            {
                throw SocketException(
                    "getsockopt failed with error: "
                    + std::to_string(LAST_SOCKET_ERROR()));
            }
            if (error != 0)
            {
                throw SocketException(
                    "Connection failed: " + std::to_string(error));
            }

            do
            {
                // Sleep for a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                fd_set writeFD;
                FD_ZERO(&writeFD);
                FD_SET(_socket, &writeFD);
                timeval timeout{};
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                if (select(
                        (int32_t)(_socket + 1), nullptr, &writeFD, nullptr,
                        &timeout)
                    > 0)
                {
                    error = 0;
                    len = sizeof(error);
                    if (getsockopt(
                            _socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len)
                        != 0)
                    {
                        throw SocketException(
                            "getsockopt failed with error: "
                            + std::to_string(LAST_SOCKET_ERROR()));
                    }
                    if (error == 0)
                    {
                        _status = SocketStatus::CONNECTED;
                        return;
                    }
                }
            } while ((std::chrono::system_clock::now() - connectStartTime)
                     < CONNECT_TIMEOUT);

            // Connection request timed out
            throw SocketException("Connection timed out.");
        }
        catch (const std::exception&)
        {
            CloseSocket();
            throw;
        }
    }

    void ConnectAsync(const std::string& address, uint16_t port) override
    {
        if (_status != SocketStatus::CLOSED)
        {
            throw std::runtime_error("Socket not closed.");
        }

        auto saddress = std::string(address);
        std::promise<void> barrier;
        _connectFuture = barrier.get_future();
        auto thread = std::thread(
            [this, saddress, port](std::promise<void> barrier2) -> void {
                try
                {
                    Connect(saddress.c_str(), port);
                }
                catch (const std::exception& ex)
                {
                    _error = std::string(ex.what());
                }
                barrier2.set_value();
            },
            std::move(barrier));
        thread.detach();
    }

    void Disconnect() override
    {
        if (_status == SocketStatus::CONNECTED)
        {
            shutdown(_socket, SHUT_RDWR);
        }
    }

    size_t SendData(const void* buffer, size_t size) override
    {
        if (_status != SocketStatus::CONNECTED)
        {
            throw std::runtime_error("Socket not connected.");
        }

        size_t totalSent = 0;
        do
        {
            const char* bufferStart = (const char*)buffer + totalSent;
            size_t remainingSize = size - totalSent;
            int32_t sentBytes = send(
                _socket, bufferStart, (int32_t)remainingSize, FLAG_NO_PIPE);
            if (sentBytes == SOCKET_ERROR)
            {
                return totalSent;
            }
            totalSent += sentBytes;
        } while (totalSent < size);
        return totalSent;
    }

    SocketReadStatus ReceiveData(
        void* buffer, size_t size, size_t* sizeReceived) override
    {
        if (_status != SocketStatus::CONNECTED)
        {
            throw std::runtime_error("Socket not connected.");
        }

        int32_t readBytes = recv(_socket, (char*)buffer, (int32_t)size, 0);
        if (readBytes == 0)
        {
            *sizeReceived = 0;
            return SocketReadStatus::DISCONNECTED;
        }
        else if (readBytes == SOCKET_ERROR)
        {
            *sizeReceived = 0;
            if (LAST_SOCKET_ERROR() != EWOULDBLOCK)
            {
                return SocketReadStatus::DISCONNECTED;
            }
            else
            {
                return SocketReadStatus::FAIL;
            }
        }
        else
        {
            *sizeReceived = readBytes;
            return SocketReadStatus::SUCCESS;
        }
    }

    void Close() override
    {
        if (_connectFuture.valid())
        {
            _connectFuture.wait();
        }
        CloseSocket();
    }

    const char* GetHostName() const override
    {
        return _hostName.empty() ? nullptr : _hostName.c_str();
    }

private:
    explicit TcpSocket(SOCKET socket, const std::string& hostName)
    {
        _socket = socket;
        _hostName = hostName;
        _status = SocketStatus::CONNECTED;
    }

    void CloseSocket()
    {
        if (_socket != INVALID_SOCKET)
        {
            closesocket(_socket);
            _socket = INVALID_SOCKET;
        }
        _status = SocketStatus::CLOSED;
    }
};

bool InitializeWSA()
{
    if (!_wsaInitialised)
    {
        WSADATA wsa_data{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            printf("Unable to initialise winsock.\n");
            return false;
        }
        _wsaInitialised = true;
    }
    return _wsaInitialised;
}

void DisposeWSA()
{
    if (_wsaInitialised)
    {
        WSACleanup();
        _wsaInitialised = false;
    }
}

std::unique_ptr<ITcpSocket> CreateTcpSocket()
{
    return std::make_unique<TcpSocket>();
}
