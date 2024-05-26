#include "server.h"

char *globalRootPath = NULL;
volatile int serverRun = 1;

RoutineArray globalRoutineArray;

SOCKET *serverListenSocket;

char handleDir(SOCKET clientSocket, char *webPath, char *absolutePath, char type, PlatformFileStat *st) {
    char *buf = NULL;
    SendBuffer socketBuffer = sendBufferNew(clientSocket, 0);
    PlatformDir *dir = platformDirOpen(absolutePath);

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
        if (sendBufferFlush(&socketBuffer) == 0)
            goto handleDirAbort;
    }

    htmlHeaderWrite(&buf, webPath[0] == '\0' ? "/" : webPath);

    if (httpBodyWriteChunk(&socketBuffer, &buf) == 0)
        goto handleDirAbort;

    sendBufferFlush(&socketBuffer);
    free(buf), buf = NULL;

    htmlBreadCrumbWrite(&buf, webPath[0] == '\0' ? "/" : webPath);

    htmlListStart(&buf);

    if (httpBodyWriteChunk(&socketBuffer, &buf) == 0)
        goto handleDirAbort;

    sendBufferFlush(&socketBuffer);
    free(buf);

    RoutineArrayAdd(&globalRoutineArray, DirectoryRoutineNew(socketBuffer, dir, webPath));

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
    SendBuffer socketBuffer = sendBufferNew(clientSocket, 0);
    PlatformFile fp = platformFileOpen(absolutePath, "rb");
    PlatformFileOffset start, finish;
    char e;

    if (fp == NULL)
        return 1;

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

    if (platformPathWebToSystem(globalRootPath, webPath, absolutePath) || platformFileStat(absolutePath, &st))
        goto handlePathNotFound;

    e = httpClientReadType(header);

    if (!httpHeaderReadIfModifiedSince(header, &tm)) {
        PlatformTimeStruct mt;
        if (!platformGetTimeStruct(&st.st_mtime, &mt)) {
            if (platformTimeStructEquals(&tm, &mt)) {
                SendBuffer socketBuffer = sendBufferNew(clientSocket, 0);

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

handlePathNotFound: {
        SendBuffer socketBuffer = sendBufferNew(clientSocket, 0);
        return httpHeaderHandleError(&socketBuffer, webPath, e, 404);
    }
}

char handleConnection(SOCKET clientSocket) {
    char r = 0, buffer[BUFSIZ], *uriPath;
    int err;
    size_t bytesRead, messageSize = errno = 0;

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
                    SendBuffer socketBuffer = sendBufferNew(clientSocket, 0);

                    httpHeaderWriteResponse(&socketBuffer, 431);
                    HTTP_HEADER_CONNECTION_NO_REUSE_WRITE(&socketBuffer);
                    httpHeaderWriteDate(&socketBuffer);
                    httpHeaderWriteContentLength(&socketBuffer, 0);
                    httpHeaderWriteEnd(&socketBuffer);
                    sendBufferFlush(&socketBuffer);
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
    if (getsockname(*listenSocket, (struct sockaddr *) &sock, &sockLen) == -1)
        return 0;

    if (sock.ss_family == AF_INET) {
        struct sockaddr_in *address = (struct sockaddr_in *) &sock;
        return ntohs(address->sin_port);
    } else if (sock.ss_family == AF_INET6) {
        struct SOCKIN6 *address = (struct SOCKIN6 *) &sock;
        return ntohs(address->sin6_port);
    }
    return 0;
}

void serverPoke(void) {
    SOCKET sock;
    struct sockaddr_in ipv4;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return;

    ipv4.sin_addr.s_addr = inet_addr("127.0.0.1"), ipv4.sin_family = AF_INET;
    ipv4.sin_port = htons(getPort(&serverListenSocket[1]));

    connect(sock, (struct sockaddr *) &ipv4, sizeof(ipv4)); /* NOLINT(*-unused-return-value) */
    CLOSE_SOCKET(sock);
}

void serverFreeResources(void) {
    if (serverListenSocket)
        free(serverListenSocket);
}

