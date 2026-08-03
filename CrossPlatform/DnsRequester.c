#include "CommonHeader.h"
#include "DnsRequester.h"
#include <stdint.h>

char* ReadName(uint8_t* ptr, int* curr)
{
    int len = ptr[0];
    char* result = malloc(len + 1);
    memset(result, 0, len + 1);
    for (int i = 0; i < len; i++)
    {
        ptr++;
        result[i] = *ptr;
    }
    if(curr)
        *curr += len + 1;
    return result;
}

void StartDnsRequestHandler()
{
    Init();
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo("8.8.8.8", "53", &hints, &result) != 0)
        goto failed;
    
    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    char request[] = {
        0XDE, 0XAD,
        0X01, 0X00,
        0X00, 0X01,
        0X00, 0X00,
        0X00, 0X00,
        0X00, 0X00,
        //end header
        0x06,'g','o','o','g','l','e',
        0x03,'c','o','m',
        0x00,
        0x00, 0x01,
        0x00, 0x01,
    };
    sendto(sock, request, sizeof(request), 0, result->ai_addr, result->ai_addrlen);

    uint8_t resp[1024];
    memset(resp, 0, sizeof(resp));

    int rec = recv(sock, resp, sizeof(resp), 0);
    
    if (rec < 12)
        goto cleanStuff;
    int curr = 0;
    printf("ID:%x\n",*(UINT16*) & resp[curr]);
    curr += 2;
    printf("QR bit (0 - query): %d\n", resp[curr] >> 7);
    int opcode = (resp[curr] >> 3)&0x7;
    printf("OPCODE: %d\n", opcode);
    int AA = (resp[curr] >> 2) & 0x1;
    int TC = (resp[curr] >> 1) & 0x1;
    int RD = resp[curr] & 0x1;
    printf("AA (Is authoritative answer): %d\n", AA);
    printf("TC (Is truncated): %d\n", TC);
    printf("RD (Is recursion desired): %d\n", RD);
    curr++;
    int RA = (resp[curr] & 0x80) >> 7;
    printf("RA (Is recursion available): %d\n", RA);
    int Z = (resp[curr] & 0x70) >> 4;
    printf("Z (Reserved zeros): %d\n", Z);
    int RCODE = resp[curr] & 0xF;
    printf("RCODE (Result code): %d\n", RCODE);
    curr++;
    int QDCOUNT = ntohs(*(uint16_t*) &resp[curr]);
    curr += 2;
    int ANCOUNT = ntohs(*(uint16_t*)&resp[curr]);
    curr += 2;
    int NSCOUNT = ntohs(*(uint16_t*)&resp[curr]);
    curr += 2;
    int ARCOUNT = ntohs(*(uint16_t*)&resp[curr]);
    curr += 2;
    printf("QDCOUNT (Question count): %d\n", QDCOUNT);
    printf("ANCOUNT (Answer count): %d\n", ANCOUNT);
    printf("NSCOUNT (Name servers count): %d\n", NSCOUNT);
    printf("ARCOUNT (Additional information count): %d\n", ARCOUNT);
    printf("\tQuestions:\n");
    for(int i=0;i< QDCOUNT;i++)
    {
        while (resp[curr] != 0)
        {
            char* resName;
            if (resp[curr] & 0xC0)
            {
                //ptr
                int ptrValue = ntohs(*(uint16_t*)&resp[curr]);
                ptrValue &= 0x3FFF;
                while (resp[ptrValue] != 0)
                {
                    resName = ReadName(&resp[ptrValue], &ptrValue);
                    printf("Name: %s\n", resName);
                    free(resName);
                }
                curr += 1;
                break;

            }
            else
            {
                resName = ReadName(&resp[curr], NULL);
                curr += resp[curr] + 1;
                printf("Name: %s\n", resName);
                free(resName);
            }

        }
        curr++;
        printf("QTYPE: %d\n", ntohs(*(uint16_t*)&resp[curr]));
        curr += 2;
        printf("QCLASS: %d\n", ntohs(*(uint16_t*)&resp[curr]));
        curr += 2;
        printf("\n");
    }
    printf("\tAnswers:\n");
    for (int i = 0; i < ANCOUNT; i++)
    {
        while (resp[curr]!=0)
        {
            char* resName;
            if (resp[curr] & 0xC0)
            {
                //ptr
                int ptrValue = ntohs(*(uint16_t*)&resp[curr]);
                ptrValue &= 0x3FFF;
                while (resp[ptrValue] != 0)
                {
                    resName = ReadName(&resp[ptrValue], &ptrValue);
                    printf("Name: %s\n", resName);
                    free(resName);
                }
                curr += 1;
                break;
                
            }
            else
            {
                resName = ReadName(&resp[curr], NULL);
                curr += resp[curr] + 1;
                printf("Name: %s\n", resName);
                free(resName);
            }
            
        }
        curr++;
        printf("QTYPE: %d\n", ntohs(*(uint16_t*)&resp[curr]));
        curr += 2;
        printf("QCLASS: %d\n", ntohs(*(uint16_t*)&resp[curr]));
        curr += 2;

        printf("TTL: %d\n", ntohl(*(uint32_t*)&resp[curr]));
        curr += 4;
        int RDLENGTH = ntohs(*(uint16_t*)&resp[curr]);
        printf("RDLENGTH: %d\n", RDLENGTH);
        curr += 2;
        printf("RDATA: ");
        for (int j = 0; j < RDLENGTH; j++)
        {
            printf("%d.", resp[curr+j]);
        }
        printf("\n");
        printf("\n");
    }
cleanStuff:

    freeaddrinfo(result);

    CLOSESOCKET(sock);

failed:

    Destroy();
}