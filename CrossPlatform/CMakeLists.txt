project(CrossPlatformApp C)
add_executable(cross_app shared_main.c TcpServerClient.c menu.c UdpToUpperServerClient.c CommonHeader.c DnsRequester.c WebRequester.c)