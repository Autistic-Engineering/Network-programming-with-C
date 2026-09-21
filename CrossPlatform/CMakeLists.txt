project(CrossPlatformApp C)
find_package(OpenSSL REQUIRED)
add_executable(cross_app shared_main.c OpenSslClient.c TcpServerClient.c menu.c UdpToUpperServerClient.c CommonHeader.c DnsRequester.c WebRequester.c FileServer.c MailClient.c)

target_include_directories(cross_app PRIVATE ${OPENSSL_INCLUDE_DIR})

target_link_libraries(cross_app PRIVATE OpenSSL::SSL OpenSSL::Crypto)