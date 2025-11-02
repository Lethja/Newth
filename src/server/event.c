#include "event.h"

void (*eventHttpRespondCallback)(eventHttpRespond *) = NULL;

void (*eventHttpFinishCallback)(eventHttpRespond *) = NULL;

void (*eventSocketAcceptCallback)(SOCKET *) = NULL;

void (*eventSocketCloseCallback)(SOCKET *) = NULL;

void eventHttpRespondSetCallback(void (*callback)(eventHttpRespond *)) {
	eventHttpRespondCallback = callback;
}

void eventHttpFinishSetCallback(void (*callback)(eventHttpRespond *)) {
	eventHttpFinishCallback = callback;
}

void eventSocketAcceptSetCallback(void (*callback)(SOCKET *)) {
	eventSocketAcceptCallback = callback;
}

void eventSocketCloseSetCallback(void (*callback)(SOCKET *)) {
	eventSocketCloseCallback = callback;
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
#ifdef TH_TCP_NEVER_REUSE
	eventSocketCloseCallback(clientSocket);
	CLOSE_SOCKET(*clientSocket);
#endif
}

void eventSocketAcceptInvoke(SOCKET *clientSocket) {
	if (eventSocketAcceptCallback)
		eventSocketAcceptCallback(clientSocket);
}

void eventSocketCloseInvoke(SOCKET *clientSocket) {
	if (eventSocketCloseCallback)
		eventSocketCloseCallback(clientSocket);
}
