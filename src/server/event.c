#include "event.h"

void (*eventHttpRespondCallback)(eventHttpRespond *) = NULL;

void eventHttpRespondSetCallback(void (*callback)(eventHttpRespond *)) {
    eventHttpRespondCallback = callback;
}

void eventHttpRespondInvoke(SOCKET *clientSocket, const char *path, char type, short respond) {
    if (eventHttpRespondCallback) {
        eventHttpRespond st;
        st.clientSocket = clientSocket, st.type = &type, st.response = &respond, st.path =
                path[0] == '/' ? path + 1 : path;
        eventHttpRespondCallback(&st);
    }
}
