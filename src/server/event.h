#ifndef OPEN_WEB_EVENT_H
#define OPEN_WEB_EVENT_H

#include "../platform/platform.h"

typedef struct eventHttpBytesSent {
    SOCKET *clientSocket;
    off_t bytesSent;
} eventHttpBytesSent;

typedef struct eventHttpFinish {
    SOCKET *clientSocket;
} eventHttpFinish;

typedef struct eventHttpRespond {
    SOCKET *clientSocket;
    short *response;
    const char *path;
    char *type;
} eventHttpRespond;

typedef struct eventSocketOpen {
    SOCKET *clientSocket;
} eventSocketOpen;

typedef struct eventSocketClose {
    SOCKET *clientSocket;
} eventSocketClose;

void eventHttpRespondSetCallback(void (*callback)(eventHttpRespond *));

void eventHttpRespondInvoke(SOCKET *clientSocket, const char *path, char type, short respond);

#endif /* OPEN_WEB_EVENT_H */
