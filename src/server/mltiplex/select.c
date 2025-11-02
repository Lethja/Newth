#include "../../platform/platform.h"
#include "../server.h"

#ifndef _WIN32

#include <sys/select.h>

#endif

static fd_set serverListenSockets;
static fd_set serverReadSockets;
static fd_set serverWriteSockets;
static SOCKET serverMaxSocket = 0;

static SOCKET GetFdSetSize(fd_set *set) {
	SOCKET i, r = 0;
	for (i = 0; i <= serverMaxSocket; ++i) {
		if (FD_ISSET(i, set))
			++r;
	}
	return r;
}

static SOCKET *GetFdSetAsArray(fd_set *set) {
	SOCKET size = GetFdSetSize(&serverReadSockets), *r = malloc(sizeof(SOCKET) * (size + 1)), i, j = 1;
	if (!r)
		return NULL;

	r[0] = size;
	for (i = 0; i <= serverMaxSocket; ++i) {
		if (FD_ISSET(i, set)) {
			r[j] = i;
			++j;
		}
	}

	return r;
}

static void fdSetToSocketArray(const SOCKET *array, fd_set *fdSet) {
	if (array && fdSet) {
		SOCKET i, max = array[0] + 1;
		for (i = 1; i < max; ++i)
			FD_SET(array[i], fdSet);
	}
}

SOCKET *serverGetListenSocketArray(void) {
	return GetFdSetAsArray(&serverListenSockets);
}

SOCKET *serverGetReadSocketArray(void) {
	return GetFdSetAsArray(&serverReadSockets);
}

SOCKET *serverGetWriteSocketArray(void) {
	return GetFdSetAsArray(&serverWriteSockets);
}

char serverDeferredSocketAdd(SOCKET socket) {
	FD_SET(socket, &serverWriteSockets);
	return 0;
}

void serverDeferredSocketRemove(SOCKET socket) {
	FD_CLR(socket, &serverWriteSockets);
}

char serverDeferredSocketExists(SOCKET socket) {
	return FD_ISSET(socket, &serverWriteSockets);
}

void serverSetup(SOCKET *sockets) {
	SOCKET i;
	FD_ZERO(&serverReadSockets);
	FD_ZERO(&serverWriteSockets);

	fdSetToSocketArray(sockets, &serverListenSockets);
	fdSetToSocketArray(sockets, &serverReadSockets);
	for (i = 1; i <= sockets[0]; ++i) {
		if (sockets[i] > serverMaxSocket)
			serverMaxSocket = sockets[i];
	}
}

void serverTick(void) {
	SOCKET i;
	fd_set readyToReadSockets, readyToWriteSockets;
	struct timeval globalSelectSleep;

	while (serverRun) {
		if (globalRoutineArray.size)
			RoutineTick(&globalRoutineArray);

		globalSelectSleep.tv_usec = 0;
		globalSelectSleep.tv_sec = globalRoutineArray.size ? 0 : PLATFORM_SELECT_MAX_TIMEOUT;

		readyToReadSockets = serverReadSockets;
		readyToWriteSockets = serverWriteSockets;

		if (select((int) serverMaxSocket + 1, &readyToReadSockets, &readyToWriteSockets, NULL, &globalSelectSleep) < 0)
			exit(1);


		for (i = 0; i <= serverMaxSocket; ++i) {
			if (FD_ISSET(i, &readyToReadSockets)) {
				if (FD_ISSET(i, &serverListenSockets)) {
					SOCKET clientSocket = platformAcceptConnection(i);
					if (clientSocket > serverMaxSocket)
						serverMaxSocket = clientSocket;

					FD_SET(clientSocket, &serverReadSockets);
				} else {
					if (handleConnection(i)) {
						eventSocketCloseInvoke(&i);
						RoutineArrayDelSocket(&globalRoutineArray, i);
						CLOSE_SOCKET(i);
						FD_CLR(i, &serverReadSockets);
						FD_CLR(i, &serverWriteSockets);
					}
				}
			}

			if (FD_ISSET(i, &readyToWriteSockets))
				FD_CLR(i, &serverWriteSockets);
		}
#ifdef PLATFORM_SHOULD_EXIT
		serverRun = PLATFORM_SHOULD_EXIT();
#endif
	}
}
