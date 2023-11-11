#include "../../platform/platform.h"
#include "../server.h"

#include <sys/poll.h>

/* TODO: Implement poll() version of serverTick() */

SOCKET *serverGetListenSocketArray(void) { return (int *) -1; }

SOCKET *serverGetReadSocketArray(void) { return (int *) -1; }

SOCKET *serverGetWriteSocketArray(void) { return (int *) -1; }

void serverSetup(SOCKET *sockets) {}

void serverTick(void) {}
