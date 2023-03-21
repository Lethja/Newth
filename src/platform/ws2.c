#include "ws2.h"

char *platformPathCombine(char *path1, char *path2) {
    const char pathDivider = '/';
    size_t a = strlen(path1), b = strlen(path2), path2Jump = 1;
    char *returnPath;

    if (path1[a - 1] != pathDivider && path2[0] != pathDivider)
        path2Jump++;

    returnPath = malloc(a + b + path2Jump);
    memcpy(returnPath, path1, a);
    if (path2Jump > 1)
        returnPath[a] = pathDivider;

    memcpy(returnPath + a + path2Jump - 1, path2, b + 1);

    return returnPath;
}

void platformCloseBindSockets(fd_set *sockets) {
}

int platformServerStartup(int *listenSocket, short port) {
    return 1;
}

void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int)) {
}
