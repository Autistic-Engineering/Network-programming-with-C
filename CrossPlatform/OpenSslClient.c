#include "OpenSslClient.h"
#include <stdio.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX 128
#define MEGA 1024

void TestOpenSslPresent(){
    printf("OpenSSL version:%s\n", OpenSSL_version(SSLEAY_VERSION));
}

void RunOpenSslClient(){
    printf("Starting openSSL client\n");
    OPENSSL_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    printf("Creating SSL context\n");
    SSL_CTX *sslContext = SSL_CTX_new(TLS_client_method()); //what are other methods

    if(!sslContext){
        printf("Failed to intialize SSL context\n");
        exit(1);
    }
    printf("Give me the remote host info\n");

    char hostname[MAX], port[MAX];
    getInput("Host name",hostname);
    getInput("Port",port);
    struct addrinfo hints, *addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(hostname, port, &hints, &addr) != 0)
    {
        printf("Failed to get an address info\n");
        exit(2);
    }

    printf("Resolving remote host info\n");
    char addressBuffer[MEGA], serviceBuffer[MEGA];
    if(getnameinfo(addr->ai_addr, addr->ai_addrlen, addressBuffer, MEGA, serviceBuffer, MEGA, 0) != 0){
        printf("Failed to get an address' name info\n");
        exit(3);
    }

    printf("Resolved hostname: %s;\nResolved service: %s;\n", addressBuffer, serviceBuffer);

    SOCKET tcpLayer = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(!ISVALIDSOCKET(tcpLayer)){
        printf("Failed to create a socket\n");
        exit(4);
    }

    if(connect(tcpLayer, addr->ai_addr, addr->ai_addrlen) != 0){
        printf("Failed to establish the TCP connection\n");
        exit(5);
    }

    freeaddrinfo(addr);
    printf("TCP connection established\n");
    printf("Creating SSL connection object...\n");
    SSL *sslConnection = SSL_new(sslContext);
    if(!sslConnection){
        printf("Failed to intialize an SSL connection object\n");
        exit(6);      
    }

    printf("Setting tls extension for SNI...\n");
    if(!SSL_set_tlsext_host_name(sslConnection, hostname)){
        printf("Failed to set the TLS-extension hostname (SNI)\n");
        exit(7);  
    }

    printf("Setting tls extension for SNI...\n");
    if(!SSL_set_fd(sslConnection, tcpLayer)){
        printf("Failed to bind the system socket to SSL  object\n");
        exit(8);  
    }

    if(!SSL_connect(sslConnection)){
        printf("Failed complete the TLS handshake\n");
        ERR_print_errors_fp(stderr);
        exit(9);  
    }

    printf("TLS connection established!\n\tThe cipher used:%s\n",  SSL_get_cipher(sslConnection));
    X509 *cert = SSL_get_peer_certificate(sslConnection);
    char *tmp;

    tmp = X509_NAME_oneline(X509_get_subject_name(cert), 0,0);
    if(tmp){
        printf("\tThe certificate's subject:%s\n", tmp);
        OPENSSL_free(tmp);
    }
 

    tmp = X509_NAME_oneline(X509_get_issuer_name(cert), 0,0);
    if(tmp){
        printf("\tThe certificate's issuer:%s\n", tmp);
        OPENSSL_free(tmp);
    }
    

    X509_free(cert);

    printf("Sending encrypted HTTP get request...\n");

    char buffer[MEGA];
    sprintf(buffer, "GET / HTTP/1.1\r\n");
    sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "User-Agent: boba\r\n");
    sprintf(buffer + strlen(buffer), "\r\n");

    SSL_write(sslConnection, buffer, strlen(buffer));

    printf("Sent stuff: %s\n", buffer);
    printf("Sending encrypted HTTP get request...\n");
    printf("Received:");
    while(1){
        int bytesRead =SSL_read(sslConnection, buffer, MEGA);
        if(bytesRead < 1){
            printf("Connection was closed\n");
            break;
        }
        printf("%.*s", bytesRead, buffer);
    }
    printf("Terminating SSL session...\n");
    SSL_shutdown(sslConnection);
    printf("Releasing SSL connection resources...\n");
    SSL_free(sslConnection);
    printf("Closing underlying system socket...\n");
    CLOSESOCKET(tcpLayer);
    printf("Releasing SSL context (configuration)...\n");
    SSL_CTX_free(sslContext);
}