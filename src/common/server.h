#ifndef NEW_TH_SERVER_H
#define NEW_TH_SERVER_H

#include "../platform/platform.h"
#include "../server/event.h"
#include "../server/http.h"
#include "../server/routine.h"

#include <stdlib.h>

extern char *globalRootPath;

extern fd_set serverCurrentSockets;

extern RoutineArray globalFileRoutineArray;
extern RoutineArray globalDirRoutineArray;

extern SOCKET serverMaxSocket;
extern SOCKET serverListenSocket;

extern volatile int serverRun;

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

#endif /* NEW_TH_SERVER_H */
