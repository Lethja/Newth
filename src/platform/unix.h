#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LINE 4096
#define SA struct sockaddr
#define SERVER_PORT 8080

__attribute__((unused)) static inline char *pathCombine(char *path1, char *path2) {
    const char pathDivider = '/';
    size_t a = strlen(path1), b = strlen(path2), path2Jump = 1;
    char *path1End = strrchr(path1, pathDivider), *returnPath;

    if (path1End != path1 + a)
        path2Jump++;

    returnPath = malloc(a + b + path2Jump);
    memcpy(returnPath, path1, a);
    returnPath[a + 1] = pathDivider;
    memcpy(returnPath + a + path2Jump, path2, b + 1);

    return returnPath;
}
