#include "FileServer.h"
#include <string.h>

#include <signal.h>
//http://127.0.0.1:8080/
volatile sig_atomic_t keepRunning = 1;
void handleDrop(int sig){
	if(sig == SIGINT){
		printf("GOT CTRL + C\n");
		keepRunning = 0;
	}
}

void CreateWebFileServer()
{
	signal(SIGINT, handleDrop);
	#pragma region certificates setup
	OPENSSL_init();
	//OpenSSL_add_all_algorithms(); //questions
	OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, NULL);
	//SSL_load_error_strings();
	OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);

	SSL_CTX* sslContext = SSL_CTX_new(TLS_server_method());
	if(!sslContext){
		printf("Failed to initialize ssl context\n");
		ERR_print_errors_fp(stderr);
		exit(111);
	}

	if(SSL_CTX_use_certificate_file(sslContext, "/home/rico/myTestCert/cert.pem", SSL_FILETYPE_PEM) == 0){
		printf("Failed to apply certificate file\n");
		ERR_print_errors_fp(stderr);
		exit(111);
	}
	if(SSL_CTX_use_PrivateKey_file(sslContext, "/home/rico/myTestCert/key.pem", SSL_FILETYPE_PEM) == 0){
		printf("Failed to apply private key file\n");
		ERR_print_errors_fp(stderr);
		exit(111);
	}
	#pragma endregion


	Init();
	//start server
	printf("Starting a listener!\n");
	struct addrinfo hints, *resAddr;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if(getaddrinfo(0, "8080", &hints, &resAddr)!=0){
		printf("Failed to get the listening address info\n");
		exit(1);
	}
	

	server = socket(resAddr->ai_family, resAddr->ai_socktype, resAddr->ai_protocol);
	if (!ISVALIDSOCKET(server))
		return;

	bind(server, resAddr->ai_addr, resAddr->ai_addrlen);

	freeaddrinfo(resAddr);

	if(listen(server, 10)!=0)
	{
		printf("Failed to initiate lising\n");
		exit(1);
	}
	printf("Lesten cycle begins!\n");
	while (keepRunning)
	{
		fd_set set = RunSelect();
		if (FD_ISSET(server, &set))
		{
			//new client
			struct sockaddr_storage clientAddr;
			socklen_t addrLen = sizeof(clientAddr);
			SOCKET clientSocket = accept(server, (struct sockaddr*) &clientAddr, &addrLen);
			struct timeval timeout = {.tv_sec = 2, .tv_usec = 0};
			setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			struct client* newOne = malloc(sizeof(struct client));
			if (ISVALIDSOCKET(clientSocket) && newOne)
			{
				memset(newOne, 0, sizeof(struct client));
				newOne->socket = clientSocket;
				newOne->addr = clientAddr;
				newOne->addrLen = addrLen;

				newOne->next = client_list;

				newOne->ssl = SSL_new(sslContext);
				

				if(SSL_set_fd(newOne->ssl, newOne->socket) == 0){
					printf("Failed to set FD for client\n");
					ERR_print_errors_fp(stderr);
					break;
				}
				if(SSL_accept(newOne->ssl) != 1){
					printf("Failed to accept TLS client, dropping it\n");
					ERR_print_errors_fp(stderr);
					SSL_shutdown(newOne->ssl);
					SSL_free(newOne->ssl);
					CLOSESOCKET(clientSocket);
					free(newOne);
				}
				else{
					client_list = newOne;
					printf("New client got connected!\n");
				}
					
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
			cl->received = SSL_read(cl->ssl, cl->inputBuffer, MAX_MESSAGE);

			if (cl->received > 0)
			{
				printf("Got a message:%.*s\n", cl->received, cl->inputBuffer);
				if (cl->received > MAX_MESSAGE)
				{
					printf("Request size exceeded\n");
					SendWholeMessage(cl->ssl, err400);
					goto cont;
				}
				//search for header end
				if (!strstr(cl->inputBuffer, "\r\n\r\n"))
					goto cont; //keep reading

				//check if header is supported
				if (strncmp(cl->inputBuffer, "GET /", 5))
				{
					printf("Unsopported request received\n");
					SendWholeMessage(cl->ssl, err400);
					goto cont;
				}

				//serve resource
				char* pathStart = cl->inputBuffer + 4;
				char* pathEnd = strstr(pathStart, " ");
				if(!pathEnd)
				{
					printf("Found no space at the end of path\n");
					SendWholeMessage(cl->ssl, err400);
					goto cont;
				}
				*pathEnd = 0;

				SendFile(cl->ssl, pathStart);
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
	printf("terminating...\n");
	#pragma region certificates destroy
	CLOSESOCKET(server);
	SSL_CTX_free(sslContext);
	#pragma endregion
	Destroy();
}

void SendWholeMessage(SSL* target, const char* message)
{
	SSL_write(target, message, strlen(message));
}

void drop(SOCKET socket)
{
	struct client** cl = &client_list;
	while (*cl)
	{
		if ((*cl)->socket == socket)
		{
			struct client* toFree = *cl;
			*cl = (*cl)->next;

			SSL_shutdown(toFree->ssl);
			SSL_free(toFree->ssl);
			CLOSESOCKET(socket);

			free(toFree);
			printf("Client session terminated\n");
			return;
		}
		cl = &(*cl)->next;
	}
}

void SendFile(SSL* client, char* path)
{
	if (strcmp(path, "/") == 0)
		path = "index.html";

	if (strstr(path, ".."))
	{
		printf("Cannot handle ..\n");
		SendWholeMessage(client, err404);
		return;
	}

	char homeRelativePath[1024];
	sprintf(homeRelativePath, "/home/rico/%s", path);

	FILE* fp = fopen(homeRelativePath, "rb");
	if (!fp)
	{
		printf("Cannot open file '%s'\n", homeRelativePath);
		SendWholeMessage(client, err404);
		return;
	}

	fseek(fp, 0L, SEEK_END);
	size_t fileSize = ftell(fp);
	rewind(fp);

	const char* contentType = get_content_type(homeRelativePath);

	char buffer[1024];
	sprintf(buffer, "HTTP/1.1 200 OK\r\n");
	SSL_write(client, buffer, strlen(buffer));

	sprintf(buffer, "Connection: close\r\n");
	SSL_write(client, buffer, strlen(buffer));

	sprintf(buffer, "Content-Type: %s\r\n", contentType);
	SSL_write(client, buffer, strlen(buffer));

	sprintf(buffer, "Content-Length: %zu\r\n\r\n", fileSize);
	SSL_write(client, buffer, strlen(buffer));

	int beenRead = fread(buffer, 1, 1024, fp);
	while (beenRead)
	{
		SSL_write(client, buffer, beenRead);
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
	++max;

	struct timeval timeout = {
		.tv_sec = 3,
		.tv_usec = 0
	};

	if(select(max, &set, 0, 0, &timeout) < 0)
	{
		FD_ZERO(&set);
	}
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