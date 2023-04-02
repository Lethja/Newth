#include "event.h"

void (*eventHttpRespondCallback)(eventHttpRespond *) = NULL;

void (*eventHttpFinishCallback)(eventHttpRespond *) = NULL;

void eventHttpRespondSetCallback(void (*callback)(eventHttpRespond *)) {
    eventHttpRespondCallback = callback;
}

void eventHttpFinishSetCallback(void (*callback)(eventHttpRespond *)) {
    eventHttpFinishCallback = callback;
}

void eventHttpRespondInvoke(SOCKET *clientSocket, const char *path, char type, short respond) {
    if (eventHttpRespondCallback) {
        eventHttpRespond st;
        st.clientSocket = clientSocket, st.type = &type, st.response = &respond, st.path =
                path[0] == '/' ? path + 1 : path;
        eventHttpRespondCallback(&st);
    }
}

void eventHttpFinishInvoke(SOCKET *clientSocket, const char *path, char type, short respond) {
    if (eventHttpFinishCallback) {
        eventHttpRespond st;
        st.clientSocket = clientSocket, st.type = &type, st.response = &respond, st.path =
                path[0] == '/' ? path + 1 : path;
        eventHttpFinishCallback(&st);
    }
}
