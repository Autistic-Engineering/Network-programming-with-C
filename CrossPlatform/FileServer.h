#pragma once

#include "CommonHeader.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_MESSAGE 2047

struct client
{
	SOCKET socket;
	struct sockaddr_storage addr;
	socklen_t addrLen;
	char inputBuffer[MAX_MESSAGE+1];
	int received;

	struct client* next;
};

struct client* client_list;

SOCKET server;

static const char err400[] = "HTTP/1.1 400 Bad Request\r\n"
"Connection: close\r\n"
"Content-Length: 11\r\n\r\n"
"Bad Request";

static const char err404[] = "HTTP/1.1 404 Not Found\r\n"
"Connection: close\r\n"
"Content-Length: 9\r\n\r\n"
"Not Found";

void drop(SOCKET socket);

void SendWholeMessage(SOCKET target, const char* message);

const char* get_content_type(const char* path);
void SendFile(SOCKET client, char* path);
void CreateWebFileServer();
struct fd_set RunSelect();