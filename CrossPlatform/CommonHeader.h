#if defined(_WIN32)
	#pragma comment(lib, "ws2_32.lib")
	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0600
	#endif
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <conio.h>  //_kbhit
	#include <winsock2.h>

	#define ISVALIDSOCKET(x) ((x) != INVALID_SOCKET)
	#define CLOSESOCKET(x) (closesocket(x))
	#define GETERRORCODE (WSAGetLastError())
	#define GETLASTERROR GETERRORCODE
#else
	#define _GNU_SOURCE
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>

	#define ISVALIDSOCKET(x) ((x) > 0)
	#define CLOSESOCKET(x) (close(x))
	#define GETERRORCODE (errno)
	#define SOCKET int
#endif

void Init();

void Destroy();