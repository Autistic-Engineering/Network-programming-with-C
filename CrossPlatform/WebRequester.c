#include "WebRequester.h"

void MakeWebRequest()
{
	Init();
	//get user input
	char endpoint[64], port[16], hostName[64], path[128];
	printf("Endpoint to send request:\n");
	fgets(endpoint, sizeof(endpoint), stdin);
	endpoint[strcspn(endpoint, "\r\n")] = 0;

	printf("Port to send request:\n");
	fgets(port, sizeof(port), stdin);
	port[strcspn(port, "\r\n")] = 0;

	printf("Target hostname:\n");
	fgets(hostName, sizeof(hostName), stdin);
	hostName[strcspn(hostName, "\r\n")] = 0;

	printf("Path to request:\n");
	fgets(path, sizeof(path), stdin);
	path[strcspn(path, "\r\n")] = 0;

	//construct message
	char message[4096];
	memset(message, 0, sizeof(message));
	sprintf(message, "GET %s HTTP/1.1\r\n", path);
	sprintf(message+ strlen(message), "Host: %s:%s\r\n", hostName, port);
	sprintf(message+ strlen(message), "Connection: close\r\n");
	sprintf(message + strlen(message), "User-Agent: honpwc web_get 1.0\r\n\r\n");

	//establish connection
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	if (getaddrinfo(endpoint, port, &hints, &result) != 0)
		return;

	SOCKET webClient = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	
	if (!webClient)
		return;

	if (connect(webClient, result->ai_addr, result->ai_addrlen))
		return;

	freeaddrinfo(result);

	//send message
	int sent = send(webClient, message, strlen(message), 0);
	if (sent < 1)
		return;

	//receive answer
	int received = recv(webClient, message, sizeof(message), 0);
	if (received < 1)
		return;

	printf("got a response:\n%.*s", received, message);


	CLOSESOCKET(webClient);
	Destroy();
}