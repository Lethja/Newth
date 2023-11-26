#include "io.h"

char *socketAddressCreateSocket(SocketAddress *self, SOCKET *sock) {
    if ((*sock = socket(self->sock.sa_family, SOCK_STREAM, 0)) == -1)
        return strerror(platformSocketGetLastError());

    return NULL;
}
