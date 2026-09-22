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

void getInput(const char* parameterName, char* buffer){
    buffer[0] = 0;
    printf("Enter %s: ", parameterName);
    fgets(buffer, 1024, stdin);
    size_t entered = strlen(buffer);
    if(entered>0)
        buffer[entered-1] = 0;
}