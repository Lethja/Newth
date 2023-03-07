#include "platform/unix.h"
#include "server/http.h"
#include "server/routine.h"

#include <sys/stat.h>

int globalServerSocket;
char *globalRootPath = NULL;
FileRoutineArray globalFileRoutineArray;

struct timeval globalSelectSleep;

void noAction() {}

void closeBindSocket() {
    if (globalServerSocket)
        shutdown(globalServerSocket, SHUT_RDWR);
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

    globalSelectSleep.tv_sec = 1;
    globalSelectSleep.tv_usec = 0;

    return newSocket;

    ServerStartupError:
    perror("Unable to start server");
    exit(1);
}

int acceptConnection(int fromSocket) {
    socklen_t addrSize = sizeof(struct sockaddr_in);
    int clientSocket;
    struct sockaddr_in clientAddress;
    char address[16] = "";

    clientSocket = accept(fromSocket, (SA *) &clientAddress, &addrSize);
    inet_ntop(AF_INET, &clientAddress.sin_addr, address, sizeof(address));
    fprintf(stdout, "%s\n", address);
    return clientSocket;
}

void handleDir(int clientSocket, char *path, struct stat *st) {
    /* TODO: Implementation */
}

void handleFile(int clientSocket, char *path, struct stat *st) {
    char header[BUFSIZ] = "";
    FILE *fp = fopen(path, "rb");

    if (fp == NULL)
        return;

    /* Headers */
    httpHeaderWriteResponse(header, 200);
    httpHeaderWriteDate(header);
    httpHeaderWriteFileName(header, path);
    httpHeaderWriteLastModified(header, st);
    httpHeaderWriteContentLengthSt(header, st);
    httpHeaderWriteEnd(header);

#ifndef NDEBUG
    printf("%s\n", header);
#endif

    if (write(clientSocket, header, strlen(header)) == -1) {
        perror("Error in writing file header");
        fclose(fp);
        return;
    }

    /* Body */
    if (st->st_size < BUFSIZ) {
        httpBodyWriteFile(clientSocket, fp);
        fclose(fp);
    } else
        FileRoutineArrayAdd(&globalFileRoutineArray, FileRoutineNew(clientSocket, fp, 0, st->st_size));
}

void handlePath(int clientSocket, char *path) {
    struct stat st;
    const char *body = "Not Found";
    char *absolutePath = NULL, *test = NULL;
    int r;
    size_t lenA, lenB;

    if (!(test = realpath(path, absolutePath)))
        goto handlePathNotFound;

    lenA = strlen(globalRootPath), lenB = strlen(test);

    if (lenA > lenB)
        goto handlePathNotFound;

    r = memcmp(globalRootPath, test, lenA);
    free(test);
    test = NULL;

    if (r)
        goto handlePathNotFound;

    if (stat(path, &st) != 0)
        goto handlePathNotFound;

    if (S_ISDIR(st.st_mode))
        handleDir(clientSocket, path, &st);
    else if (S_ISREG(st.st_mode))
        handleFile(clientSocket, path, &st);
    return;

    handlePathNotFound:
    if (test)
        free(test);

    {
        char bufferOut[BUFSIZ] = "";
        httpHeaderWriteResponse(bufferOut, 404);
        httpHeaderWriteDate(bufferOut);
        httpHeaderWriteContentLength(bufferOut, strlen(body));
        httpHeaderWriteEnd(bufferOut);
        if (write(clientSocket, bufferOut, strlen(bufferOut)) == -1)
            perror("Error handling 404");

        httpBodyWriteText(clientSocket, body);
    }
}

void handleConnection(int clientSocket) {
#define BUFFER_SIZE 4096
    char buffer[BUFFER_SIZE];
    size_t bytesRead, messageSize = 0;
    char *uriPath;

    while ((bytesRead = read(clientSocket, buffer + messageSize, sizeof(buffer) - messageSize - 1))) {
        messageSize += bytesRead;
        if (messageSize > BUFFER_SIZE - 1) {
            char bufferOut[BUFSIZ] = "";
            httpHeaderWriteResponse(bufferOut, 431);
            httpHeaderWriteDate(bufferOut);
            httpHeaderWriteContentLength(bufferOut, 0);
            httpHeaderWriteEnd(bufferOut);

            if (write(clientSocket, bufferOut, strlen(bufferOut)) == -1)
                perror("Error handling 431");

            return;
        }

        if (buffer[messageSize - 1] == '\n')
            break;
    }

    buffer[messageSize - 1] = 0;

#ifndef NDEBUG
    printf("REQUEST: %s\n", buffer);
    fflush(stdout);
#endif

    uriPath = httpClientReadUri(buffer);
    if (uriPath) {
        handlePath(clientSocket, uriPath + 1);
        free(uriPath);
    }
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

    fd_set currentSockets, readySockets;
    FD_ZERO(&currentSockets);
    FD_SET(globalServerSocket, &currentSockets);

    while (1) {
        size_t i;
        int clientSocket = -1;

        for (i = 0; i < globalFileRoutineArray.size; i++) {
            if (!FileRoutineContinue(&globalFileRoutineArray.array[i]))
                FileRoutineArrayDel(&globalFileRoutineArray, &globalFileRoutineArray.array[i]);
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
                    clientSocket = acceptConnection(globalServerSocket);
                    FD_SET(clientSocket, &currentSockets);
                } else {
                    handleConnection(clientSocket);
                    FD_CLR(i, &currentSockets);
                }
            }
        }
    }
}