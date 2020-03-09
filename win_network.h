#ifndef NETWORK_HPP
#define NETWORK_HPP


#include <winsock2.h>
#include <string>
#include <exception>

class NetworkException : public std::exception
{
public:
    NetworkException(const char* message);
    virtual const char* what() const throw();

private:
    std::string m_message;
};


class Socket;  // forward declaration


class ServerSocket
{
public:
    ServerSocket(int port);
    ~ServerSocket();

    Socket* accept();

private:
    SOCKET m_socket;
};


class Socket
{
    friend ServerSocket;

public:
    Socket(const char* address, int port);
    ~Socket();

    int write(int size, const void* bytes);
    int read(int size, void* bytes);

private:
    Socket(SOCKET socket);

private:
    SOCKET m_socket;
};


#endif
