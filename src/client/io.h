#ifndef NEW_DL_IO_H
#define NEW_DL_IO_H

#include "../platform/platform.h"

typedef union SocketAddress {
    struct sockaddr sock;
    struct sockaddr_in ipv4;
#ifdef ENABLE_IPV6
    struct sockaddr_in6 ipv6;
#endif
    struct sockaddr_storage storage;
} SocketAddress;

extern char *socketAddressCreateSocket(SocketAddress *self, SOCKET *sock);

#endif /* NEW_DL_IO_H */
