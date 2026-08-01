#include "CommonHeader.h"
#include "UdpToUpperServerClient.h"

void UdpToUpperServer(){
    printf("Enter a port to bind to:\n");
    char port[10];
    fgets(port, sizeof(port), stdin);
    port[strcspn(port, "\r\n")] = 0;
    struct addrinfo hints, *result;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    getaddrinfo(0, port, &hints, &result);

    int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    bind(sock, result->ai_addr, result->ai_addrlen);

    while(1){
        struct sockaddr_storage clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        char buffer[1024];
        int recieved = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if(recieved < 1)
            break;
        for(int i=0;i<recieved;i++){
            buffer[i] = toupper(buffer[i]);
        }
        sendto(sock, buffer, recieved, 0, (struct sockaddr*)&clientAddr, clientAddrLen);
    }
    close(sock);
}

void UdpClient(){
    printf("Enter an address to send to:\n");
    char ip[100];
    fgets(ip, sizeof(ip), stdin);
    printf("Enter a port to send data to:\n");
    char port[100];
    fgets(port, sizeof(port), stdin);
    ip[strcspn(ip, "\r\n")] = 0;
    port[strcspn(port, "\r\n")] = 0;

    struct addrinfo hints, *result;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    getaddrinfo(ip, port, &hints, &result);
    int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    printf("Type what to send:\n");
    char message[500];
    fgets(message, sizeof(message), stdin);
    message[strcspn(message, "\r\n")] = 0;
    sendto(sock, message, strlen(message), 0, result->ai_addr, result->ai_addrlen);
    int received = recv(sock, message, sizeof(message), 0);
    if(received > 0){
        printf("Message received: %.*s", received, message);
    }
    close(sock);
}