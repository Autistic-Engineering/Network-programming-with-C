#include "menu.h"

int PrintMenu(){
    int num;
    while(1)
    {
        system("clear");
        printf("Pick what to start\n");
        printf("1 - Print interfaces and their addresses\n");
        printf("2 - Dummy tcp client\n");
        printf("3 - Dummy tcp server\n");
        printf("4 - Udp toUpper server\n");
        printf("5 - Udp client\n");
        printf("6 - DNS client\n");
        printf("7 - Web requester\n");
        printf("8 - Web file sender\n");
        printf("9 - Mail client\n");   
        printf("10 - OpenSSL client\n");   
        scanf("%d", &num);
        if(num>10 || num <1)
            continue;
        break;
    }
    while (getchar() != '\n'); 
    return num;
}