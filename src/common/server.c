#include "server.h"

char *globalRootPath = NULL;
volatile int serverRun = 1;

RoutineArray globalRoutineArray;
/* TODO: Store all adapters with correct port numbers (for Haiku) */
/*
AdapterAddressArray adapters;
*/

SOCKET serverMaxSocket;
SOCKET *serverListenSocket;
fd_set serverListenSockets;
fd_set serverReadSockets;
fd_set serverWriteSockets;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

void noAction(int signal) {}

#pragma clang diagnostic pop

char handleDir(SOCKET clientSocket, char *webPath, char *absolutePath, char type, PlatformFileStat *st) {
    char *buf = NULL;
    SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);
    DIR *dir = platformDirOpen(absolutePath);

    if (dir == NULL)
        return httpHeaderHandleError(&socketBuffer, webPath, httpGet, 404);

    /* Headers */
    httpHeaderWriteResponse(&socketBuffer, 200);
    HTTP_HEADER_CONNECTION_NO_REUSE_WRITE(&socketBuffer);
    httpHeaderWriteDate(&socketBuffer);
    httpHeaderWriteLastModified(&socketBuffer, st);
    httpHeaderWriteContentType(&socketBuffer, "text/html", "");
    httpHeaderWriteChunkedEncoding(&socketBuffer);
    httpHeaderWriteEnd(&socketBuffer);
    eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, type, 200);

    if (type == httpHead) {
        if (socketBufferFlush(&socketBuffer) == 0)
            goto handleDirAbort;
    }

    htmlHeaderWrite(&buf, webPath[0] == '\0' ? "/" : webPath);

    if (httpBodyWriteChunk(&socketBuffer, &buf) == 0 || socketBufferFlush(&socketBuffer) == 0)
        goto handleDirAbort;
    free(buf), buf = NULL;

    htmlBreadCrumbWrite(&buf, webPath[0] == '\0' ? "/" : webPath);

    htmlListStart(&buf);

    if (httpBodyWriteChunk(&socketBuffer, &buf) == 0 || socketBufferFlush(&socketBuffer) == 0)
        goto handleDirAbort;

    free(buf);

    RoutineArrayAdd(&globalRoutineArray, DirectoryRoutineNew(socketBuffer, dir, webPath, globalRootPath));

    return 0;

    handleDirAbort:
    if (buf)
        free(buf);

    if (dir)
        platformDirClose(dir);

    return 1;
}

char handleFile(SOCKET clientSocket, const char *header, char *webPath, char *absolutePath, char httpType,
                PlatformFileStat *st) {
    SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);
    PlatformFile fp = platformFileOpen(absolutePath, "rb");
    PlatformFileOffset start, finish;
    char e;

    if (fp == NULL) {
        LINEDBG;
        return 1;
    }

    start = finish = 0;
    e = httpHeaderReadRange(header, &start, &finish, (const PlatformFileOffset *) &st->st_size);

    /* Headers */
    httpHeaderWriteResponse(&socketBuffer, (short) (e ? 200 : 206));
    HTTP_HEADER_CONNECTION_NO_REUSE_WRITE(&socketBuffer);
    httpHeaderWriteDate(&socketBuffer);
    httpHeaderWriteFileName(&socketBuffer, webPath);
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
    eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, httpType, (short) (e ? 200 : 206));

    if (httpType == httpHead)
        return 0;

    /* Body */
    RoutineArrayAdd(&globalRoutineArray, FileRoutineNew(socketBuffer, fp, start, e ? st->st_size : finish, webPath));
    return 0;
}

char handlePath(SOCKET clientSocket, const char *header, char *webPath) {
    PlatformFileStat st;
    PlatformTimeStruct tm;
    char absolutePath[FILENAME_MAX], e = 0;

    if (platformPathWebToSystem(globalRootPath, webPath, absolutePath) || platformFileStat(absolutePath, &st)) {
        LINEDBG;
        goto handlePathNotFound;
    }

    e = httpClientReadType(header);

    if (!httpHeaderReadIfModifiedSince(header, &tm)) {
        PlatformTimeStruct mt;
        if (!platformGetTimeStruct(&st.st_mtime, &mt)) {
            if (platformTimeStructEquals(&tm, &mt)) {
                SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);

                httpHeaderWriteResponse(&socketBuffer, 304);
                HTTP_HEADER_CONNECTION_NO_REUSE_WRITE(&socketBuffer);
                httpHeaderWriteDate(&socketBuffer);
                httpHeaderWriteEnd(&socketBuffer);
                eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, e, 304);

                RoutineArrayAdd(&globalRoutineArray, RoutineNew(socketBuffer, webPath));

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
        SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);
        return httpHeaderHandleError(&socketBuffer, webPath, e, 404);
    }
}

char handleConnection(SOCKET clientSocket) {
    char r = 0, buffer[BUFSIZ], *uriPath;
    int err;
    size_t bytesRead, messageSize = errno = 0;

    LINEDBG;

    do {
        bytesRead = recv(clientSocket, buffer + messageSize, (int) (sizeof(buffer) - messageSize - 1), 0);

        switch (bytesRead) {
            case 0:
                if (messageSize == 0)
                    return 1;
                else if (messageSize >= 4 && strncmp(&buffer[messageSize - 4], HTTP_EOL HTTP_EOL, 4) == 0) {
                    r = 50;
                    break;
                }

                /* Fall through */
            case -1:
                err = platformSocketGetLastError();
                switch (err) {
                    case 0:
                    case SOCKET_WOULD_BLOCK:
#if SOCKET_WOULD_BLOCK != SOCKET_TRY_AGAIN
                    case SOCKET_TRY_AGAIN:
#endif
                        ++r;
                        continue;
                    default:
                        return 1;
                }
            default:
                messageSize += bytesRead;
                if (messageSize >= BUFSIZ - 1) {
                    SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);

                    httpHeaderWriteResponse(&socketBuffer, 431);
                    HTTP_HEADER_CONNECTION_NO_REUSE_WRITE(&socketBuffer);
                    httpHeaderWriteDate(&socketBuffer);
                    httpHeaderWriteContentLength(&socketBuffer, 0);
                    httpHeaderWriteEnd(&socketBuffer);
                    socketBufferFlush(&socketBuffer);
                    eventHttpRespondInvoke(&socketBuffer.clientSocket, "", httpUnknown, 431);

                    return 1;
                }

                if (messageSize >= 4 && strncmp(&buffer[messageSize - 4], HTTP_EOL HTTP_EOL, 4) == 0)
                    r = 50;
                else if (r)
                    r = 0;
        }

    } while (r < 50);

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

unsigned short getPort(const SOCKET *listenSocket) {
    struct sockaddr_storage sock;
    socklen_t sockLen = sizeof(sock);
    if (getsockname(*listenSocket, (struct sockaddr *) &sock, &sockLen) == -1) {
        LINEDBG;
        return 0;
    }

    if (sock.ss_family == AF_INET) {
        struct sockaddr_in *address = (struct sockaddr_in *) &sock;
        return ntohs(address->sin_port);
    } else if (sock.ss_family == AF_INET6) {
        struct SOCKIN6 *address = (struct SOCKIN6 *) &sock;
        return ntohs(address->sin6_port);
    }
    return 0;
}

void serverTick(void) {
    SOCKET i, deferredSockets;
    fd_set readyToReadSockets, readyToWriteSockets, errorSockets;
    struct timeval globalSelectSleep;

    while (serverRun) {
        RoutineTick(&globalRoutineArray, &serverWriteSockets, &deferredSockets);

        globalSelectSleep.tv_usec = 0;
        globalSelectSleep.tv_sec = (globalRoutineArray.size - deferredSockets) ? 0 : PLATFORM_SELECT_MAX_TIMEOUT;

        readyToReadSockets = serverReadSockets;
        readyToWriteSockets = serverWriteSockets;

        if (select((int) serverMaxSocket + 1, &readyToReadSockets, &readyToWriteSockets, &errorSockets, &globalSelectSleep) <
            0) {
            LINEDBG;
            exit(1);
        }

        for (i = 0; i <= serverMaxSocket; ++i) {
            /* TODO: Retest all platforms with this code */
            if (FD_ISSET(i, &errorSockets)) {
                eventSocketCloseInvoke(&i);
                RoutineArrayDelSocket(&globalRoutineArray, i);
                CLOSE_SOCKET(i);
                FD_CLR(i, &serverReadSockets);
                FD_CLR(i, &serverWriteSockets);
                continue;
            }

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

            if (FD_ISSET(i, &readyToWriteSockets)) {
                FD_CLR(i, &serverWriteSockets);
            }
        }
#ifdef PLATFORM_SHOULD_EXIT
        serverRun = PLATFORM_SHOULD_EXIT();
#endif
    }
}

void socketArrayToFdSet(const SOCKET *array, fd_set *fdSet) {
    if (array && fdSet) {
        SOCKET i, max = array[0] + 1;
        for (i = 1; i < max; ++i)
            FD_SET(array[i], fdSet);
    }
}

void serverPoke(void) {
    SOCKET sock;
    struct sockaddr_in ipv4;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        LINEDBG;
        return;
    }

    ipv4.sin_addr.s_addr = inet_addr("127.0.0.1"), ipv4.sin_family = AF_INET, ipv4.sin_port = htons(
            getPort(&serverListenSocket[1]));

    if (connect(sock, (SA *) &ipv4, sizeof(ipv4)) == 0)
        CLOSE_SOCKET(sock);
}

void serverFreeResources(void) {
    if (serverListenSocket)
        free(serverListenSocket);
}

