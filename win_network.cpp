/****************************************************************
	xerxes engine
	win_network.cpp
 ****************************************************************/

/* network.cpp by AegisKnight, aka Chad Austin */
#include "win_network.h"

static void InitializeNetwork()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 0), &wsadata);
}


////////////////////////////////////////////////////////////////////////////////
// NetworkException ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

NetworkException::NetworkException(const char* message)
: m_message(message)
{
}

////////////////////////////////////////////////////////////////////////////////

const char*
NetworkException::what() const throw()
{
    return m_message.c_str();
}

////////////////////////////////////////////////////////////////////////////////
// ServerSocket ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ServerSocket::ServerSocket(int port)
{
    InitializeNetwork();

    // create the socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        throw NetworkException("socket() failed");
    }

    // make it nonblocking
    unsigned long ul = 1;
    if (ioctlsocket(m_socket, FIONBIO, &ul) != 0) {
        closesocket(m_socket);
        throw NetworkException("ioctlsocket() failed");
    }

    // bind the socket to an address
    sockaddr_in name;
    memset(&name, 0, sizeof(name));
    name.sin_family      = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port        = htons(port);
    if (bind(m_socket, (sockaddr*)&name, sizeof(name)) != 0) {
        closesocket(m_socket);
        throw NetworkException("bind() failed");
    }

    // now listen on it
    if (listen(m_socket, 1) != 0) {
        closesocket(m_socket);
        throw NetworkException("listen() failed");
    }
}

////////////////////////////////////////////////////////////////////////////////

ServerSocket::~ServerSocket()
{
    closesocket(m_socket);
}

////////////////////////////////////////////////////////////////////////////////

Socket*
ServerSocket::accept()
{
    if (m_socket == INVALID_SOCKET) {
        return NULL;
    }

    sockaddr_in addr;
    int addrlen = sizeof(addr);
    SOCKET s = ::accept(m_socket, (sockaddr*)&addr, &addrlen);
    if (s == INVALID_SOCKET) {
        return NULL;
    }

    return new Socket(s);
}

////////////////////////////////////////////////////////////////////////////////
// Socket //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Socket::Socket(const char* address, int port)
{
    InitializeNetwork();
	
    // resolve address
    hostent* hostptr = gethostbyname(address);
    if (hostptr == NULL) {
        throw NetworkException("gethostbyname() failed");
    }

    // create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        throw NetworkException("socket() failed");
    }

    // connect
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr   = *(in_addr*)(hostptr->h_addr);
    addr.sin_port   = htons(port);
    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(m_socket);
        throw NetworkException("connect() failed");
    }

	// set nonblocking
	u_long argp = 1L;
	int result = ioctlsocket(m_socket, FIONBIO, &argp);

	// set tcp_nodelay
	char argc = 1;
	setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &argc, 1);
}

////////////////////////////////////////////////////////////////////////////////

Socket::~Socket()
{
    closesocket(m_socket);
}

////////////////////////////////////////////////////////////////////////////////

int
Socket::write(int size, const void* bytes)
{
    return send(m_socket, (char*)bytes, size, 0);
}

////////////////////////////////////////////////////////////////////////////////

int
Socket::read(int size, void* bytes)
{
    return recv(m_socket, (char*)bytes, size, 0);
}

////////////////////////////////////////////////////////////////////////////////

Socket::Socket(SOCKET socket)
: m_socket(socket)
{
}

////////////////////////////////////////////////////////////////////////////////
