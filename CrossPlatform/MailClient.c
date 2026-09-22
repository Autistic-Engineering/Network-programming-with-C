#include "MailClient.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h> //isdigit
#include <time.h>

#define MAX_INPUT 128
#define SIZE 1024

void sendFormatted(SOCKET socket, const char* text, ...){
    char buffer[SIZE];
    va_list args;
    va_start(args, text);
    vsprintf(buffer, text, args);
    va_end(args);

    send(socket, buffer, strlen(buffer), 0);
    printf("Client:%s\n", buffer);
}

/// @brief if message is complete it returns the converted last occurence of RES_CODE
int ParseMultilineResponse(const char* resp){
    const char* curr = resp;
    if(!curr[0] || !curr[1] || !curr[2])
        return 0;
    for(;curr[3];++curr){
        //int the line start
        if(curr == resp || curr[-1] == '\n'){
            //3 digits
            if(isdigit(curr[0]) && isdigit(curr[1]) && isdigit(curr[2])){
                //is '-' or ' ' 
                if(curr[3] != '-'){
                    //if end received
                    if(strstr(curr, "\r\n")){
                        return strtol(curr, 0, 10);
                    }
                }
            }
        }
    }
    return 0;
}

void WaitResponse(SOCKET socket, int expected){
    char resp[SIZE + 1];
    char* curr = resp;
    char* end = resp + SIZE;
    
    int code = 0;
    while(code == 0){
        int received = recv(socket, curr, end-curr, 0);
        if(received < 1){
            printf("Failed to receive response\n");
            exit(1);
        }

        curr+=received;
        *curr = 0;
        if(curr == end){
            printf("Response too large\n");
            exit(1);
        }

        code = ParseMultilineResponse(resp);
    }
    if(code != expected)
    {
        printf("Got a not expected response: %s\n", resp);
        exit(1);
    }

    printf("Server:%s\n", resp);

    return;
}

SOCKET connectToHost(const char* hostname, const char* port){
    SOCKET newSocket;
    struct addrinfo requirement_REQ, *PointertoAddrInfo_PAI;
    memset(&requirement_REQ, 0, sizeof(requirement_REQ));
    requirement_REQ.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(hostname, port, &requirement_REQ, &PointertoAddrInfo_PAI) != 0)
    {
        printf("Failed to get address info for a socket init\n");
        exit(1);
    }
    newSocket = socket(PointertoAddrInfo_PAI->ai_family, PointertoAddrInfo_PAI->ai_socktype, PointertoAddrInfo_PAI->ai_protocol);
    if(!ISVALIDSOCKET(newSocket)){
        printf("Failed to create a socket\n");
        exit(1);
    }
    if(connect(newSocket, PointertoAddrInfo_PAI->ai_addr, PointertoAddrInfo_PAI->ai_addrlen) !=0){
        printf("Failed to connect to an endpoint\n");
        exit(1);
    }
    freeaddrinfo(PointertoAddrInfo_PAI);
    return newSocket;
}

void RunMailClient(){
    printf("Enter the smtp server u need\n");
    char hostname[MAX_INPUT], port[MAX_INPUT];
    getInput("Server: ", hostname);
    getInput("Port: ", port);

    SOCKET socket = connectToHost(hostname, port);
    //server initiates the SMTP dialog
    WaitResponse(socket, 220);

    //client identifies itself
    sendFormatted(socket, "HELO Bibidon\r\n");
    WaitResponse(socket, 250);

    char sender[MAX_INPUT], receiver[MAX_INPUT], subject[MAX_INPUT], date[MAX_INPUT];
    getInput("From: ", sender);
    sendFormatted(socket, "MAIL FROM:<%s>\r\n", sender);
    WaitResponse(socket, 250);   
    
    getInput("To: ", receiver);
    sendFormatted(socket, "RCPT TO:<%s>\r\n", receiver);
    WaitResponse(socket, 250); 

    //start body transmition
    sendFormatted(socket, "DATA\r\n");
    WaitResponse(socket, 354);

    getInput("mail subject: ", subject);
    sendFormatted(socket, "From:<%s>\r\n", sender);
    sendFormatted(socket, "To:<%s>\r\n", receiver);
    sendFormatted(socket, "Subject:%s\r\n", subject);

    //get time
    time_t timer;
    time(&timer);

    struct tm *timeinfo;
    timeinfo = gmtime(&timer);

    strftime(date, MAX_INPUT, "%a, %d %b %Y %H:%M:%S +0000", timeinfo);
    sendFormatted(socket,"Date: %s\r\n", date);

    //delineate the mail body
    sendFormatted(socket, "\r\n");

    //now the body

    sendFormatted(socket, "Here is my mail body my friend");
    sendFormatted(socket, "\r\n.\r\n");
    WaitResponse(socket, 250); 

    sendFormatted(socket, "QUIT\r\n");
    WaitResponse(socket, 221); 

    CLOSESOCKET(socket);
}