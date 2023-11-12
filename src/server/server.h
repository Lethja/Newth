#ifndef NEW_TH_SERVER_H
#define NEW_TH_SERVER_H

#include "../platform/platform.h"
#include "event.h"
#include "http.h"
#include "routine.h"

#include <stdlib.h>

extern char *globalRootPath;

extern RoutineArray globalRoutineArray;

extern SOCKET *serverListenSocket;

extern volatile int serverRun;

extern void serverSetup(SOCKET *sockets);

extern SOCKET *serverGetListenSocketArray(void);

extern SOCKET *serverGetReadSocketArray(void);

extern SOCKET *serverGetWriteSocketArray(void);

extern char serverDeferredSocketAdd(SOCKET socket);

extern char serverDeferredSocketExists(SOCKET socket);

extern void serverDeferredSocketRemove(SOCKET socket);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

extern void noAction(int signal);

#pragma clang diagnostic pop

extern char handleDir(SOCKET clientSocket, char *webPath, char *absolutePath, char type, PlatformFileStat *st);

extern char handleFile(SOCKET clientSocket, const char *header, char *webPath, char *absolutePath, char httpType,
                       PlatformFileStat *st);

extern char handlePath(SOCKET clientSocket, const char *header, char *webPath);

extern char handleConnection(SOCKET clientSocket);

extern unsigned short getPort(const SOCKET *listenSocket);

extern void serverTick(void);

extern void serverPoke(void);

/**
 * Free any memory platformServerStartup allocated
 */
extern void serverFreeResources(void);

#endif /* NEW_TH_SERVER_H */
