#include "OpenSslClient.h"
#include <stdio.h>
#include <openssl/ssl.h>

void TestOpenSslPresent(){
    printf("OpenSSL version:%s\n", OpenSSL_version(SSLEAY_VERSION));
}