#ifndef OPEN_WEB_EVENT_H
#define OPEN_WEB_EVENT_H

#include "../platform/platform.h"

typedef struct eventHttpRespond {
    SOCKET *clientSocket;
    short *response;
    const char *path;
    char *type;
} eventHttpRespond;

void eventHttpRespondSetCallback(void (*callback)(eventHttpRespond *));

void eventHttpFinishSetCallback(void (*callback)(eventHttpRespond *));

void eventSocketAcceptSetCallback(void (*callback)(SOCKET *));

void eventSocketCloseSetCallback(void (*callback)(SOCKET *));

void eventHttpRespondInvoke(SOCKET *clientSocket, const char *path, char type, short respond);

void eventHttpFinishInvoke(SOCKET *clientSocket, const char *path, char type, short respond);

void eventSocketAcceptInvoke(SOCKET *clientSocket);

void eventSocketCloseInvoke(SOCKET *clientSocket);

#endif /* OPEN_WEB_EVENT_H */
