#include "platform/unix.h"
#include "server/http.h"
#include "server/routine.h"

#include <sys/stat.h>

int globalServerSocket;
char *globalRootPath = NULL;
FileRoutineArray globalFileRoutineArray;
size_t connections = 0;

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

void handleDir(int clientSocket, char *path, struct stat *st) {
    /* TODO: Implementation */
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

    if (write(clientSocket, header, strlen(header)) == -1) {
        perror("Error in writing file header");
        return 1;
    }

    /* Body */
    if (st->st_size < BUFSIZ) {
        httpBodyWriteFile(clientSocket, fp);
        fclose(fp);
        return 0;
    } else
        FileRoutineArrayAdd(&globalFileRoutineArray, FileRoutineNew(clientSocket, fp, 0, st->st_size));
    return 0;
}

char handlePath(int clientSocket, char *path) {
    struct stat st;
    const char *body = "Not Found";
    char *absolutePath = NULL, *test = NULL, e = 0;
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
        e = handleFile(clientSocket, path, &st);

    return e;

    handlePathNotFound:
    if (test)
        free(test);

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

        httpBodyWriteText(clientSocket, body);
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
        messageSize += bytesRead;
        if (messageSize > BUFFER_SIZE - 1) {
            char bufferOut[BUFSIZ] = "";
            httpHeaderWriteResponse(bufferOut, 431);
            httpHeaderWriteDate(bufferOut);
            httpHeaderWriteContentLength(bufferOut, 0);
            httpHeaderWriteEnd(bufferOut);

            if (write(clientSocket, bufferOut, strlen(bufferOut)) == -1){
                perror("Error handling 431");
                return 1;
            }

            return 0;
        }

        if (buffer[messageSize - 1] == '\n')
            break;
    }

    buffer[messageSize - 1] = 0;

#ifndef NDEBUG
    printf("HTTP HEAD REQUEST:\n%s\n", buffer);
    fflush(stdout);
#endif

    uriPath = httpClientReadUri(buffer);
    if (uriPath) {
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

    fd_set currentSockets, readySockets;
    FD_ZERO(&currentSockets);
    FD_SET(globalServerSocket, &currentSockets);

    while (1) {
        size_t i;
        int clientSocket = 0;

        for (i = 0; i < globalFileRoutineArray.size; i++) {
            if (!FileRoutineContinue(&globalFileRoutineArray.array[i]))
                FileRoutineArrayDel(&globalFileRoutineArray, &globalFileRoutineArray.array[i]);
        }

        globalSelectSleep.tv_usec = connections ? 500 : 0;
        globalSelectSleep.tv_sec = globalFileRoutineArray.size || connections ? 0 : 60;

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
                    connections++;
                } else if (clientSocket) {
                    handleConnection(clientSocket);
                    FD_CLR(i, &currentSockets);
                }
            }
        }
    }
}