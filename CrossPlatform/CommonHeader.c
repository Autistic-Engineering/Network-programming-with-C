#include "CommonHeader.h"

void Init()
{
#if defined(_WIN32)
	WSADATA d;
	WSAStartup(MAKEWORD(2, 2), &d);
#endif
}

void Destroy()
{
#if defined(_WIN32)
	WSACleanup();
#endif
}