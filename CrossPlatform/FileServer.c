#include "FileServer.h"

//http://127.0.0.1:8080/
void CreateWebFileServer()
{
	Init();
	//start server
	struct addrinfo hints, *resAddr;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(0, "8080", &hints, &resAddr);
	

	server = socket(resAddr->ai_family, resAddr->ai_socktype, resAddr->ai_protocol);
	if (!ISVALIDSOCKET(server))
		return;

	bind(server, resAddr->ai_addr, resAddr->ai_addrlen);

	freeaddrinfo(resAddr);

	listen(server, 10);

	while (1)
	{
		fd_set set = RunSelect();
		if (FD_ISSET(server, &set))
		{
			//new client
			struct sockaddr_storage clientAddr;
			socklen_t addrLen = sizeof(clientAddr);
			SOCKET clientSocket = accept(server, (struct sockaddr*) &clientAddr, &addrLen);
			struct client* newOne = malloc(sizeof(struct client));
			if (ISVALIDSOCKET(clientSocket) && newOne)
			{
				memset(newOne, 0, sizeof(struct client));
				newOne->socket = clientSocket;
				newOne->addr = clientAddr;
				newOne->addrLen = addrLen;

				newOne->next = client_list;

				client_list = newOne;
			}
			else
				CLOSESOCKET(clientSocket);
		}

		struct client* cl = client_list;
		while (cl)
		{
			struct client* next = cl->next; //cause can be lost due to dropping clients;

			if (!FD_ISSET(cl->socket, &set))
			{
				goto cont;
			}

			//client has something to send
			cl->received = recv(cl->socket, cl->inputBuffer, MAX_MESSAGE, 0);

			if (cl->received > 0)
			{
				if (cl->received > MAX_MESSAGE)
				{
					SendWholeMessage(cl->socket, err400);
					goto cont;
				}
				//search for header end
				if (!strstr(cl->inputBuffer, "\r\n\r\n"))
					goto cont; //keep reading

				//check if header is supported
				if (strncmp(cl->inputBuffer, "GET /", 5))
				{
					SendWholeMessage(cl->socket, err400);
					goto cont;
				}

				//serve resource
				char* pathStart = cl->inputBuffer + 4;
				char* pathEnd = strstr(pathStart, " ");
				if(!pathEnd)
				{
					SendWholeMessage(cl->socket, err400);
					goto cont;
				}
				*pathEnd = 0;

				SendFile(cl->socket, pathStart);
				drop(cl->socket);
			}
			else
			{
				drop(cl->socket);
			}

		cont:
			cl = next;
			continue;
		}
	}

	Destroy();
}

void SendWholeMessage(SOCKET target, const char* message)
{
	send(target, message, strlen(message), 0);
}

void drop(SOCKET socket)
{
	CLOSESOCKET(socket);
	struct client** cl = &client_list;
	while (*cl)
	{
		if ((*cl)->socket == socket)
		{
			struct client* toFree = *cl;
			*cl = (*cl)->next;
			free(toFree);
			return;
		}
		cl = &(*cl)->next;
	}
}

void SendFile(SOCKET client, char* path)
{
	if (strcmp(path, "/") == 0)
		path = "index.html";

	if (strstr(path, ".."))
	{
		SendWholeMessage(client, err404);
		return;
	}
	path++;

	FILE* fp = fopen(path, "rb");
	if (!fp)
	{
		SendWholeMessage(client, err404);
		return;
	}

	fseek(fp, 0L, SEEK_END);
	size_t fileSize = ftell(fp);
	rewind(fp);

	char* contentType = get_content_type(path);

	char buffer[1024];
	sprintf(buffer, "HTTP/1.1 200 OK\r\n");
	send(client, buffer, strlen(buffer), 0);

	sprintf(buffer, "Connection: close\r\n");
	send(client, buffer, strlen(buffer), 0);

	sprintf(buffer, "Content-Type: %s\r\n", contentType);
	send(client, buffer, strlen(buffer), 0);

	sprintf(buffer, "Content-Length: %zu\r\n\r\n", fileSize);
	send(client, buffer, strlen(buffer), 0);

	int beenRead = fread(buffer, 1, 1024, fp);
	while (beenRead)
	{
		send(client, buffer, beenRead, 0);
		beenRead = fread(buffer, 1, 1024, fp);
	}
	fclose(fp);
	
}

fd_set RunSelect()
{
	SOCKET max;
	fd_set set;
	FD_ZERO(&set);
	FD_SET(server, & set);
	max = server;
	struct client* cl = client_list;
	while (cl)
	{
		FD_SET(cl->socket , &set);
		if (cl->socket > max)
			max = cl->socket;
		cl = cl->next;
	}
	++cl;
	select(max, &set, 0, 0, 0);
	return set;
}


const char* get_content_type(const char* path)
{
	const char* last_dot = strrchr(path, '.');
	if (last_dot)
	{
		if (strcmp(last_dot, ".css") == 0) return "text/css";
		if (strcmp(last_dot, ".csv") == 0) return "text/csv";
		if (strcmp(last_dot, ".gif") == 0) return "image/gif";
		if (strcmp(last_dot, ".htm") == 0) return "text/html";
		if (strcmp(last_dot, ".html") == 0) return "text/html";
		if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
		if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
		if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
		if (strcmp(last_dot, ".js") == 0) return "application/javascript";
		if (strcmp(last_dot, ".json") == 0) return "application/json";
		if (strcmp(last_dot, ".png") == 0) return "image/png";
		if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
		if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
		if (strcmp(last_dot, ".txt") == 0) return "text/plain";
	}

	return "application/octet-stream";
}