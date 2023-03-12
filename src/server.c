#include "platform/unix.h"
#include "server/http.h"
#include "server/routine.h"

#include <sys/stat.h>
#include <dirent.h>

int globalServerSocket;
char *globalRootPath = NULL;
FileRoutineArray globalFileRoutineArray;
fd_set currentSockets;
struct timeval globalSelectSleep;

void noAction() {}

void closeBindSocket() {
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
        if (FD_ISSET(i, &currentSockets)) {
            shutdown(i, SHUT_RDWR);
        }
    }
}

void shutdownCrash() {
    closeBindSocket();
    puts("Segfault");
    exit(1);
}

void shutdownProgram() {
    closeBindSocket();
    exit(0);
}

int serverStartup(short port) {
    int newSocket;
    struct sockaddr_in serverAddress;

    if ((newSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        exit(1);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if ((bind(newSocket, (SA *) &serverAddress, sizeof(serverAddress))) < 0)
        goto ServerStartupError;

    if ((listen(newSocket, 10)) < 0)
        goto ServerStartupError;

    globalFileRoutineArray.size = 0;
    globalFileRoutineArray.array = NULL;

    return newSocket;

    ServerStartupError:
    perror("Unable to start server");
    exit(1);
}

int acceptConnection(int fromSocket) {
    socklen_t addrSize = sizeof(struct sockaddr_in);
    int clientSocket;
    struct sockaddr_in clientAddress;

    clientSocket = accept(fromSocket, (SA *) &clientAddress, &addrSize);

#ifndef NDEBUG
    {
        char address[16] = "";
        inet_ntop(AF_INET, &clientAddress.sin_addr, address, sizeof(address));
        fprintf(stdout, "Connection opened for: %s\n", address);
    }
#endif

    return clientSocket;
}

char handleDir(int clientSocket, char *realPath, struct stat *st) {
    char *webPath = realPath + strlen(globalRootPath);
    char buf[BUFSIZ] = "";
    char pathBuf[BUFSIZ] = "";
    struct dirent *entry;
    DIR *dir = opendir(realPath);

    if (dir == NULL) {
        httpHeaderWriteResponse(buf, 404);
        httpHeaderWriteEnd(buf);
        if (write(clientSocket, buf, strlen(buf)) == -1)
            goto handleDirAbort;

        return 0;
    }

    /* Headers */
    httpHeaderWriteResponse(buf, 200);
    httpHeaderWriteDate(buf);
    httpHeaderWriteLastModified(buf, st);
    httpHeaderWriteChunkedEncoding(buf);
    httpHeaderWriteEnd(buf);

    if (write(clientSocket, buf, strlen(buf)) == -1)
        goto handleDirAbort;

    memset(buf, 0, sizeof(buf));

    htmlHeaderWrite(buf, realPath);
    htmlListStart(buf);

    if (httpBodyWriteChunk(clientSocket, buf))
        goto handleDirAbort;

    while ((entry = readdir(dir))) {
        size_t pathLen, entryLen;
        if (entry->d_name[0] == '.')
            continue;

        pathLen = strlen(webPath), entryLen = strlen(entry->d_name);

        if (pathLen + entryLen + 2 < BUFSIZ) {
            memcpy(pathBuf, webPath, pathLen);
            pathBuf[pathLen ? pathLen : 0] = '/';
            memcpy(pathLen ? pathBuf + pathLen + 1 : pathBuf + 1, entry->d_name, entryLen + 1);

            htmlListWritePathLink(buf, pathBuf);
            if (httpBodyWriteChunk(clientSocket, buf))
                goto handleDirAbort;
        }
    }

    closedir(dir);
    dir = NULL;

    htmlListEnd(buf);
    htmlFooterWrite(buf);
    if (httpBodyWriteChunk(clientSocket, buf) || httpBodyWriteChunkEnding(clientSocket))
        goto handleDirAbort;

    return 0;

    handleDirAbort:
    if (dir)
        closedir(dir);

    return 1;
}

char handleFile(int clientSocket, char *path, struct stat *st) {
    char header[BUFSIZ] = "";
    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        perror("Error in opening file");
        return 1;
    }

    /* Headers */
    httpHeaderWriteResponse(header, 200);
    httpHeaderWriteDate(header);
    httpHeaderWriteFileName(header, path);
    httpHeaderWriteLastModified(header, st);
    httpHeaderWriteContentLengthSt(header, st);
    httpHeaderWriteEnd(header);

#ifndef NDEBUG
    printf("HTTP HEAD REPLY:\n%s\n", header);
#endif

    if (write(clientSocket, header, strlen(header)) == -1)
        goto handleFileAbort;

    /* Body */
    if (st->st_size < BUFSIZ) {
        if (httpBodyWriteFile(clientSocket, fp))
            goto handleFileAbort;
        fclose(fp);
        return 0;
    } else
        FileRoutineArrayAdd(&globalFileRoutineArray, FileRoutineNew(clientSocket, fp, 0, st->st_size));
    return 0;

    handleFileAbort:
    if (fp)
        fclose(fp);
    return 1;
}

char handlePath(int clientSocket, char *path) {
    struct stat st;
    const char *body = "Not Found";
    char *absolutePath = NULL, e = 0;
    int r;
    size_t lenA, lenB;

    if (path[0] == '\0')
        absolutePath = globalRootPath;
    else if (!(absolutePath = realpath(path, NULL)))
        goto handlePathNotFound;

    lenA = strlen(globalRootPath), lenB = strlen(absolutePath);

    if (lenA > lenB)
        goto handlePathNotFound;

    r = memcmp(globalRootPath, absolutePath, lenA);

    if (r || stat(absolutePath, &st) != 0)
        goto handlePathNotFound;

    if (S_ISDIR(st.st_mode))
        e = handleDir(clientSocket, absolutePath, &st);
    else if (S_ISREG(st.st_mode))
        e = handleFile(clientSocket, absolutePath, &st);

    if (absolutePath != globalRootPath)
        free(absolutePath);

    return e;

    handlePathNotFound:
    if (absolutePath && absolutePath != globalRootPath)
        free(absolutePath);

    {
        char bufferOut[BUFSIZ] = "";
        httpHeaderWriteResponse(bufferOut, 404);
        httpHeaderWriteDate(bufferOut);
        httpHeaderWriteContentLength(bufferOut, strlen(body));
        httpHeaderWriteEnd(bufferOut);
        if (write(clientSocket, bufferOut, strlen(bufferOut)) == -1) {
            perror("Error handling 404");
            return 1;
        }

        if (httpBodyWriteText(clientSocket, body))
            return 1;

        return 0;
    }
}

char handleConnection(int clientSocket) {
#define BUFFER_SIZE 4096
    char r = 0;
    char buffer[BUFFER_SIZE];
    size_t bytesRead, messageSize = 0;
    char *uriPath;

    while ((bytesRead = read(clientSocket, buffer + messageSize, sizeof(buffer) - messageSize - 1))) {
        if (bytesRead == -1)
            return 1;

        messageSize += bytesRead;
        if (messageSize > BUFFER_SIZE - 1) {
            char bufferOut[BUFSIZ] = "";
            httpHeaderWriteResponse(bufferOut, 431);
            httpHeaderWriteDate(bufferOut);
            httpHeaderWriteContentLength(bufferOut, 0);
            httpHeaderWriteEnd(bufferOut);


            if (write(clientSocket, bufferOut, strlen(bufferOut)) == -1) {
                return 1;
            }

            return 0;
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
            r = handlePath(clientSocket, uriPath + 1);

        free(uriPath);
    }

    return r;
}

static inline void connectProgramSignals(void) {
    signal(SIGHUP, shutdownProgram);
    signal(SIGINT, shutdownProgram);
    signal(SIGPIPE, noAction);
    signal(SIGSEGV, shutdownCrash);
    signal(SIGTERM, shutdownProgram);
}

static inline void setRootPath(char *path) {
    char *test = realpath(path, globalRootPath);
    if (!test) {
        printf("No such directory \"%s\"\n", path);
        exit(1);
    }
    globalRootPath = test;
}

int main(int argc, char **argv) {
    connectProgramSignals();

    if (argc > 1) {
        setRootPath(argv[1]);
    } else {
        char *buf = malloc(BUFSIZ + 1), *test = getcwd(buf, BUFSIZ);

        buf[BUFSIZ] = '\0';
        if (test)
            setRootPath(test);

        free(buf);
    }

    globalServerSocket = serverStartup(SERVER_PORT);

    fd_set readySockets;
    FD_ZERO(&currentSockets);
    FD_SET(globalServerSocket, &currentSockets);

    while (1) {
        int i;

        for (i = 0; i < globalFileRoutineArray.size; i++) {
            if (!FileRoutineContinue(&globalFileRoutineArray.array[i])) {
                FileRoutineArrayDel(&globalFileRoutineArray, &globalFileRoutineArray.array[i]);
            }
        }

        globalSelectSleep.tv_usec = 0;
        globalSelectSleep.tv_sec = globalFileRoutineArray.size ? 0 : 60;

        readySockets = currentSockets;

        if (select(FD_SETSIZE, &readySockets, NULL, NULL, &globalSelectSleep) < 0) {
            perror("Select");
            exit(1);
        }

        for (i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &readySockets)) {
                if (i == globalServerSocket) {
                    int clientSocket = acceptConnection(globalServerSocket);
                    FD_SET(clientSocket, &currentSockets);
                } else {
                    if (handleConnection(i)) {
                        close(i);
                        FD_CLR(i, &currentSockets);
                    }
                }
            }
        }
    }
}