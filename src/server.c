#include "platform/platform.h"
#include "server/http.h"
#include "server/routine.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>

SOCKET globalServerSocket;
SOCKET globalMaxSocket = 0;
fd_set currentSockets;

char *globalRootPath = NULL;
RoutineArray globalFileRoutineArray;
RoutineArray globalDirRoutineArray;
struct timeval globalSelectSleep;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

void noAction(int signal) {}

void shutdownCrash(int signal) {
    platformCloseBindSockets(&currentSockets, globalMaxSocket);
    printf("Emergency shutdown: %d\n", signal);
    exit(1);
}

void shutdownProgram(int signal) {
    platformCloseBindSockets(&currentSockets, globalMaxSocket);
    printf("Graceful shutdown: %d\n", signal);
    exit(0);
}

#pragma clang diagnostic pop

char handleDir(SOCKET clientSocket, char *realPath, struct stat *st) {
    char *webPath = realPath + strlen(globalRootPath);
    char buf[BUFSIZ];
    SocketBuffer socketBuffer = socketBufferNew(clientSocket);
    DIR *dir = opendir(realPath);

    if (dir == NULL)
        return httpHeaderHandleError(&socketBuffer, 404);

    /* Headers */
    httpHeaderWriteResponse(&socketBuffer, 200);
    httpHeaderWriteDate(&socketBuffer);
    httpHeaderWriteLastModified(&socketBuffer, st);
    httpHeaderWriteContentType(&socketBuffer, "text/html", "");
    httpHeaderWriteChunkedEncoding(&socketBuffer);
    httpHeaderWriteEnd(&socketBuffer);
    htmlHeaderWrite(buf, webPath[0] == '\0' ? "/" : webPath);

    if (httpBodyWriteChunk(&socketBuffer, buf) || socketBufferFlush(&socketBuffer))
        goto handleDirAbort;

    buf[0] = '\0';
    htmlBreadCrumbWrite(buf, webPath[0] == '\0' ? "/" : webPath);

    if (httpBodyWriteChunk(&socketBuffer, buf))
        goto handleDirAbort;

    buf[0] = '\0';
    htmlListStart(buf);

    if (httpBodyWriteChunk(&socketBuffer, buf) || socketBufferFlush(&socketBuffer))
        goto handleDirAbort;

    DirectoryRoutineArrayAdd(&globalDirRoutineArray, DirectoryRoutineNew(clientSocket, dir, webPath, globalRootPath));

    return 0;

    handleDirAbort:
    if (dir)
        closedir(dir);

    return 1;
}

char handleFile(SOCKET clientSocket, const char *header, char *path, struct stat *st) {
    SocketBuffer socketBuffer = socketBufferNew(clientSocket);
    FILE *fp = fopen(path, "rb");
    off_t start, finish;
    char e;

    if (fp == NULL) {
        perror("Error in opening file");
        return 1;
    }

    start = finish = 0;
    e = httpHeaderReadRange(header, &start, &finish);

    /* Headers */
    httpHeaderWriteResponse(&socketBuffer, e ? 200 : 206);
    httpHeaderWriteDate(&socketBuffer);
    httpHeaderWriteFileName(&socketBuffer, path);
    httpHeaderWriteLastModified(&socketBuffer, st);

    if (e)
        httpHeaderWriteContentLengthSt(&socketBuffer, st);
    else {
        /* Handle the end of the byte range being out of range or unspecified */
        if (finish >= st->st_size || finish == 0)
            finish = st->st_size;

        httpHeaderWriteContentLength(&socketBuffer, finish - start);
        httpHeaderWriteRange(&socketBuffer, start, finish, st->st_size);
    }

    httpHeaderWriteEnd(&socketBuffer);
    socketBufferFlush(&socketBuffer);

    /* Body */
    if (st->st_size < BUFSIZ) {
        if (httpBodyWriteFile(&socketBuffer, fp, start, e ? st->st_size : finish))
            goto handleFileAbort;
        fclose(fp);
        return 0;
    } else
        FileRoutineArrayAdd(&globalFileRoutineArray, FileRoutineNew(clientSocket, fp, start, e ? finish : st->st_size));
    return 0;

    handleFileAbort:
    if (fp)
        fclose(fp);
    return 1;
}

char handlePath(SOCKET clientSocket, const char *header, char *path) {
    struct stat st;
    struct tm tm;
    char *absolutePath = NULL, e = 0;
    int r;
    size_t lenA, lenB;

    if (path[0] == '\0')
        absolutePath = globalRootPath;
    else {
        char *combinePath = platformPathCombine(globalRootPath, path);
        if (!(absolutePath = realpath(combinePath, NULL))) {
            free(combinePath);
            goto handlePathNotFound;
        }
        free(combinePath);
    }

    lenA = strlen(globalRootPath), lenB = strlen(absolutePath);

    if (lenA > lenB)
        goto handlePathNotFound;

    FORCE_BACKWARD_SLASH(globalRootPath);

    r = memcmp(globalRootPath, absolutePath, lenA);

    if (r)
        goto handlePathNotFound;

#ifdef _WIN32
    if(absolutePath[lenB - 1] == '\\')
        absolutePath[lenB - 1] = '\0';
#endif /* _WIN32 */

    r = stat(absolutePath, &st);
    if (r) {
        perror(absolutePath);
        goto handlePathNotFound;
    }

    FORCE_FORWARD_SLASH(absolutePath);

    if (!httpHeaderReadIfModifiedSince(header, &tm)) {
        struct tm mt;
        mt = *gmtime(&st.st_mtime);

        if (tm.tm_year == mt.tm_year && tm.tm_mon == mt.tm_mon && tm.tm_mday == mt.tm_mday &&
            tm.tm_hour == mt.tm_hour && tm.tm_min == mt.tm_min && tm.tm_sec == mt.tm_sec) {
            SocketBuffer socketBuffer = socketBufferNew(clientSocket);

            if (absolutePath != globalRootPath)
                free(absolutePath);

            httpHeaderWriteResponse(&socketBuffer, 304);
            httpHeaderWriteDate(&socketBuffer);
            httpHeaderWriteEnd(&socketBuffer);

            if (socketBufferFlush(&socketBuffer))
                return 1;

            return 0;
        }
    }

    if (S_ISDIR(st.st_mode))
        e = handleDir(clientSocket, absolutePath, &st);
    else if (S_ISREG(st.st_mode))
        e = handleFile(clientSocket, header, absolutePath, &st);

    if (absolutePath != globalRootPath)
        free(absolutePath);

    return e;

    handlePathNotFound:
    if (absolutePath && absolutePath != globalRootPath)
        free(absolutePath);

    {
        SocketBuffer socketBuffer = socketBufferNew(clientSocket);
        return httpHeaderHandleError(&socketBuffer, 404);
    }
}

char handleConnection(SOCKET clientSocket) {
    char r = 0, ip[INET6_ADDRSTRLEN];
    char buffer[BUFSIZ];
    size_t bytesRead, messageSize = 0;
    char *uriPath;
    struct sockaddr_storage sock;
    socklen_t sockLen = sizeof(sock);

    /* TODO: Make IP callback it's own function */
    getpeername(clientSocket, (struct sockaddr *) &sock, &sockLen);
    platformGetIpString((struct sockaddr *) &sock, ip);
    printf("%s\n", ip);

    while ((bytesRead = recv(clientSocket, buffer + messageSize, (int) (sizeof(buffer) - messageSize - 1), 0))) {
        if (bytesRead == -1)
            return 1;

        messageSize += bytesRead;
        if (messageSize >= BUFSIZ - 1) {
            SocketBuffer socketBuffer = socketBufferNew(clientSocket);

            httpHeaderWriteResponse(&socketBuffer, 431);
            httpHeaderWriteDate(&socketBuffer);
            httpHeaderWriteContentLength(&socketBuffer, 0);
            httpHeaderWriteEnd(&socketBuffer);
            socketBufferFlush(&socketBuffer);

            return 1;
        }

        if (buffer[messageSize - 1] == '\n')
            break;
    }

    if (messageSize == 0)
        return 1;

    buffer[messageSize - 1] = 0;

#ifndef NDEBUG
    printf("HTTP HEAD REQUEST:\n%s\n", buffer);
    fflush(stdout);
#endif

    uriPath = httpClientReadUri(buffer);
    if (uriPath) {
        if (uriPath[0] != '/')
            r = 1;
        else
            r = handlePath(clientSocket, buffer, uriPath + 1);

        free(uriPath);
    }

    return r;
}

static inline void setRootPath(char *path) {
    char *test = realpath(path, NULL);
    if (!test) {
        printf("No such directory \"%s\"\n", path);
        exit(1);
    }

    globalRootPath = test;
}

static void printAdapterInformation(void) {
    AdapterAddressArray *adapters = platformGetAdapterInformation();
    size_t i, j;
    for (i = 0; i < adapters->size; ++i) {
        printf("%s:\n", adapters->adapter[i].name);
        for (j = 0; j < adapters->adapter[i].addresses.size; ++j) {
            if (!adapters->adapter[i].addresses.array[j].type)
                printf("\thttp://%s:%d\n", adapters->adapter[i].addresses.array[j].address, SERVER_PORT);
            else
                printf("\thttp://[%s]:%d\n", adapters->adapter[i].addresses.array[j].address, SERVER_PORT);
        }
    }

    platformFreeAdapterInformation(adapters);
}

int main(int argc, char **argv) {
    fd_set readySockets;

    platformConnectSignals(noAction, shutdownCrash, shutdownProgram);

    if (argc > 1) {
        setRootPath(argv[1]);
    } else {
        char *buf = malloc(BUFSIZ + 1), *test = getcwd(buf, BUFSIZ);

        buf[BUFSIZ] = '\0';
        if (test)
            setRootPath(test);

        free(buf);
    }

    if (platformServerStartup(&globalServerSocket, SERVER_PORT)) {
        perror("Unable to start server");
        return 1;
    }

    globalMaxSocket = globalServerSocket;
    FD_ZERO(&currentSockets);
    FD_SET(globalServerSocket, &currentSockets);

    globalFileRoutineArray.size = globalDirRoutineArray.size = 0;
    globalFileRoutineArray.array = globalDirRoutineArray.array = NULL;

    printAdapterInformation();

    while (1) {
        SOCKET i;

        for (i = 0; i < globalDirRoutineArray.size; i++) {
            DirectoryRoutine *directoryRoutine = (DirectoryRoutine *) globalDirRoutineArray.array;
            if (!DirectoryRoutineContinue(&directoryRoutine[i])) {
                DirectoryRoutineArrayDel(&globalDirRoutineArray, &directoryRoutine[i]);
            }
        }

        for (i = 0; i < globalFileRoutineArray.size; i++) {
            FileRoutine *fileRoutine = (FileRoutine *) globalFileRoutineArray.array;
            if (!FileRoutineContinue(&fileRoutine[i])) {
                FileRoutineArrayDel(&globalFileRoutineArray, &fileRoutine[i]);
            }
        }

        globalSelectSleep.tv_usec = 0;
        globalSelectSleep.tv_sec = globalFileRoutineArray.size || globalDirRoutineArray.size ? 0 : 60;

        readySockets = currentSockets;

        if (select(globalMaxSocket + 1, &readySockets, NULL, NULL, &globalSelectSleep) < 0) {
            perror("Select");
            exit(1);
        }

        for (i = 0; i <= globalMaxSocket; i++) {
            if (FD_ISSET(i, &readySockets)) {
                if (i == globalServerSocket) {
                    SOCKET clientSocket = platformAcceptConnection(globalServerSocket);
                    if (clientSocket > globalMaxSocket)
                        globalMaxSocket = clientSocket;

                    FD_SET(clientSocket, &currentSockets);
                } else {
                    if (handleConnection(i)) {
                        CLOSE_SOCKET(i);
                        FD_CLR(i, &currentSockets);
                    }
                }
            }
        }
    }
}
