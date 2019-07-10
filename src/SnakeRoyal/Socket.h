#pragma once

#include <memory>
#include <string>
#include <vector>

enum class SocketStatus
{
    CLOSED,
    RESOLVING,
    CONNECTING,
    CONNECTED,
    LISTENING,
};

enum class SocketReadStatus
{
    SUCCESS,
    FAIL,
    DISCONNECTED,
};

class ITcpSocket
{
public:
    virtual ~ITcpSocket() = default;

    virtual SocketStatus GetStatus() const abstract;
    virtual const char* GetError() const abstract;
    virtual const char* GetHostName() const abstract;

    virtual void Listen(uint16_t port) abstract;
    virtual void Listen(const std::string& address, uint16_t port) abstract;
    virtual std::unique_ptr<ITcpSocket> Accept() abstract;

    virtual void Connect(const std::string& address, uint16_t port) abstract;
    virtual void ConnectAsync(
        const std::string& address, uint16_t port) abstract;

    virtual size_t SendData(const void* buffer, size_t size) abstract;
    virtual SocketReadStatus ReceiveData(
        void* buffer, size_t size, size_t* sizeReceived) abstract;

    virtual void Disconnect() abstract;
    virtual void Close() abstract;
};

bool InitializeWSA();
void DisposeWSA();

std::unique_ptr<ITcpSocket> CreateTcpSocket();
