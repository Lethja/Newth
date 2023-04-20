#include "platform/platform.h"
#include "server/event.h"
#include "server/http.h"
#include "server/routine.h"

#include <stdlib.h>

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

#pragma clang diagnostic pop

void shutdownCrash(int signal) {
    switch (signal) {
        default: /* Close like normal unless something has gone catastrophically wrong */
            platformCloseBindSockets(&currentSockets, globalMaxSocket);
            platformIpStackExit();
            printf("Emergency shutdown: %d\n", signal);
            /* Fallthrough */
        case SIGABRT:
        case SIGSEGV:
            exit(1);
    }
}

void shutdownProgram(int signal) {
    if (signal == SIGINT)
        printf("\n"); /* Put next message on a different line from ^C */
    platformCloseBindSockets(&currentSockets, globalMaxSocket);
    platformIpStackExit();
    exit(0);
}

char handleDir(SOCKET clientSocket, char *webPath, char *absolutePath, char type, PlatformFileStat *st) {
    char buf[BUFSIZ];
    SocketBuffer socketBuffer = socketBufferNew(clientSocket);
    DIR *dir = platformDirOpen(absolutePath);

    if (dir == NULL)
        return httpHeaderHandleError(&socketBuffer, webPath, httpGet, 404);

    /* Headers */
    httpHeaderWriteResponse(&socketBuffer, 200);
    httpHeaderWriteDate(&socketBuffer);
    httpHeaderWriteLastModified(&socketBuffer, st);
    httpHeaderWriteContentType(&socketBuffer, "text/html", "");
    httpHeaderWriteChunkedEncoding(&socketBuffer);
    httpHeaderWriteEnd(&socketBuffer);
    eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, type, 200);

    if (type == httpHead) {
        if (socketBufferFlush(&socketBuffer))
            goto handleDirAbort;
    }

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
        platformDirClose(dir);

    return 1;
}

char handleFile(SOCKET clientSocket, const char *header, char *webPath, char *absolutePath, char httpType,
                PlatformFileStat *st) {
    SocketBuffer socketBuffer = socketBufferNew(clientSocket);
    FILE *fp = fopen(absolutePath, "rb");
    off_t start, finish;
    char e;

    if (fp == NULL) {
        perror("Error in opening file");
        return 1;
    }

    start = finish = 0;
    e = httpHeaderReadRange(header, &start, &finish, &st->st_size);

    /* Headers */
    httpHeaderWriteResponse(&socketBuffer, e ? 200 : 206);
    httpHeaderWriteDate(&socketBuffer);
    httpHeaderWriteFileName(&socketBuffer, absolutePath);
    httpHeaderWriteLastModified(&socketBuffer, st);
    httpHeaderWriteAcceptRanges(&socketBuffer);

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
    eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, httpType, e ? 200 : 206);

    if (httpType == httpHead)
        return 0;

    /* Body */
    if (st->st_size < BUFSIZ) {
        if (httpBodyWriteFile(&socketBuffer, fp, start, e ? st->st_size : finish))
            goto handleFileAbort;
        fclose(fp);
        eventHttpFinishInvoke(&socketBuffer.clientSocket, webPath, httpType, 0);
        return 0;
    } else
        FileRoutineArrayAdd(&globalFileRoutineArray,
                            FileRoutineNew(clientSocket, fp, start, e ? st->st_size : finish, webPath));
    return 0;

    handleFileAbort:
    if (fp)
        fclose(fp);
    return 1;
}

char handlePath(SOCKET clientSocket, const char *header, char *webPath) {
    PlatformFileStat st;
    PlatformTimeStruct tm;
    char absolutePath[FILENAME_MAX], e = 0;

    if (platformPathWebToSystem(globalRootPath, webPath, absolutePath))
        goto handlePathNotFound;

    if (platformFileStat(absolutePath, &st)) {
        perror(absolutePath);
        goto handlePathNotFound;
    }

    e = httpClientReadType(header);

    if (!httpHeaderReadIfModifiedSince(header, &tm)) {
        PlatformTimeStruct mt;
        if (!platformGetTimeStruct(&st.st_mtime, &mt)) {
            if (platformTimeStructEquals(&tm, &mt)) {
                SocketBuffer socketBuffer = socketBufferNew(clientSocket);

                httpHeaderWriteResponse(&socketBuffer, 304);
                httpHeaderWriteDate(&socketBuffer);
                httpHeaderWriteEnd(&socketBuffer);
                eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, e, 304);

                if (socketBufferFlush(&socketBuffer))
                    return 1;

                return 0;
            }
        }
    }

    if (platformFileStatIsDirectory(&st))
        e = handleDir(clientSocket, webPath, absolutePath, e, &st);
    else if (platformFileStatIsFile(&st))
        e = handleFile(clientSocket, header, webPath, absolutePath, e, &st);

    return e;

    handlePathNotFound:
    {
        SocketBuffer socketBuffer = socketBufferNew(clientSocket);
        return httpHeaderHandleError(&socketBuffer, webPath, e, 404);
    }
}

char handleConnection(SOCKET clientSocket) {
    char r = 0;
    char buffer[BUFSIZ];
    size_t bytesRead, messageSize = 0;
    char *uriPath;

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
            eventHttpRespondInvoke(&socketBuffer.clientSocket, "", httpUnknown, 431);

            return 1;
        }

        if (buffer[messageSize - 1] == '\n')
            break;
    }

    if (messageSize == 0)
        return 1;

    buffer[messageSize - 1] = 0;

    uriPath = httpClientReadUri(buffer);
    if (uriPath) {
        if (uriPath[0] != '/')
            r = 1;
        else
            r = handlePath(clientSocket, buffer, uriPath);

        free(uriPath);
    }

    return r;
}

static void printSocketAccept(SOCKET *sock) { /* NOLINT(readability-non-const-parameter) */
    struct sockaddr_storage ss;
    socklen_t sockLen = sizeof(ss);
    sa_family_t family;
    char ip[INET6_ADDRSTRLEN];
    unsigned short port;
    getpeername(*sock, (struct sockaddr *) &ss, &sockLen);
    platformGetIpString((struct sockaddr *) &ss, ip, &family);
    port = platformGetPort((struct sockaddr *) &ss);
    if (family == AF_INET)
        printf("TSYN:%s:%u\n", ip, port);
    else if (family == AF_INET6)
        printf("TSYN:[%s]:%u\n", ip, port);
}

static void printSocketClose(SOCKET *sock) { /* NOLINT(readability-non-const-parameter) */
    struct sockaddr_storage ss;
    socklen_t sockLen = sizeof(ss);
    sa_family_t family;
    char ip[INET6_ADDRSTRLEN];
    unsigned short port;
    getpeername(*sock, (struct sockaddr *) &ss, &sockLen);
    platformGetIpString((struct sockaddr *) &ss, ip, &family);
    port = platformGetPort((struct sockaddr *) &ss);
    if (family == AF_INET)
        printf("TRST:%s:%u\n", ip, port);
    else if (family == AF_INET6)
        printf("TRST:[%s]:%u\n", ip, port);
}

static void printHttpEvent(eventHttpRespond *event) {
    struct sockaddr_storage sock;
    socklen_t sockLen = sizeof(sock);
    sa_family_t family;
    unsigned short port;
    char type;
    char ip[INET6_ADDRSTRLEN];

    switch (*event->type) {
        case httpGet:
            type = 'G';
            break;
        case httpHead:
            type = 'H';
            break;
        case httpPost:
            type = 'P';
            break;
        default:
            type = '?';
            break;
    }

    getpeername(*event->clientSocket, (struct sockaddr *) &sock, &sockLen);
    platformGetIpString((struct sockaddr *) &sock, ip, &family);
    port = platformGetPort((struct sockaddr *) &sock);
    if (family == AF_INET)
        printf("%c%03d:%s:%u/%s\n", type, *event->response, ip, port, event->path);
    else if (family == AF_INET6)
        printf("%c%03d:[%s]:%u/%s\n", type, *event->response, ip, port, event->path);
}

unsigned short getPort(const SOCKET *listenSocket) {
    struct sockaddr_storage sock;
    socklen_t sockLen = sizeof(sock);
    if (getsockname(*listenSocket, (struct sockaddr *) &sock, &sockLen) == -1) {
        perror("Unable to get socket port");
        return 0;
    }

    if (sock.ss_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *) &sock;
        return ntohs(addr->sin_port);
    } else if (sock.ss_family == AF_INET6) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &sock;
        return ntohs(addr->sin6_port);
    }
    return 0;
}

static void printAdapterInformation(char *protocol, sa_family_t family, unsigned short port) {
    AdapterAddressArray *adapters = platformGetAdapterInformation(family);
    size_t i, j;
    for (i = 0; i < adapters->size; ++i) {
        printf("%s:\n", adapters->adapter[i].name);
        for (j = 0; j < adapters->adapter[i].addresses.size; ++j) {
            if (!adapters->adapter[i].addresses.array[j].type)
                printf("\t%s://%s:%u\n", protocol, adapters->adapter[i].addresses.array[j].address, port);
            else
                printf("\t%s://[%s]:%u\n", protocol, adapters->adapter[i].addresses.array[j].address, port);
        }
    }

    platformFreeAdapterInformation(adapters);
}

int main(int argc, char **argv) {
    fd_set readySockets;

    platformConnectSignals(noAction, shutdownCrash, shutdownProgram);

    if (argc > 1) {
        globalRootPath = platformGetRootPath(argv[1]);
    } else {
        char *buf = malloc(BUFSIZ + 1), *test = getcwd(buf, BUFSIZ);

        buf[BUFSIZ] = '\0';
        if (test)
            globalRootPath = platformGetRootPath(test);

        free(buf);
    }

    printf("Root Path: %s\n\n", globalRootPath);

    {
        sa_family_t family;
        char *ports;

        /* Get the list of ports then try to bind one */
        ports = getenv("TH_HTTP_PORT");
        platformArgvGetFlag(argc, argv, 'p', "port", &ports);
        if (!ports)
            ports = "0";

        /* Choose IP standards to run on the socket */
        if (platformArgvGetFlag(argc, argv, '4', "ipv4", NULL))
            family = AF_INET;
        else if (platformArgvGetFlag(argc, argv, '6', "ipv6", NULL))
            family = AF_INET6;
        else if (platformOfficiallySupportsIpv6() || platformArgvGetFlag(argc, argv, '\0', "dual-stack", NULL))
            family = AF_UNSPEC;
        else
            family = AF_INET;

        if (platformIpStackInit()) {
            perror("Unable to network stack");
            return 1;
        }

        if (platformServerStartup(&globalServerSocket, family, ports)) {
            perror("Unable to start server");
            platformIpStackExit();
            return 1;
        }

        printAdapterInformation("http", family, getPort(&globalServerSocket));
    }

    globalMaxSocket = globalServerSocket;
    FD_ZERO(&currentSockets);
    FD_SET(globalServerSocket, &currentSockets);

    globalFileRoutineArray.size = globalDirRoutineArray.size = 0;
    globalFileRoutineArray.array = globalDirRoutineArray.array = NULL;

    eventHttpRespondSetCallback(printHttpEvent);
    eventHttpFinishSetCallback(printHttpEvent);
    eventSocketAcceptSetCallback(printSocketAccept);
    eventSocketCloseSetCallback(printSocketClose);

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
                        eventSocketCloseInvoke(&i);
                        CLOSE_SOCKET(i);
                        FD_CLR(i, &currentSockets);
                    }
                }
            }
        }
    }
}
