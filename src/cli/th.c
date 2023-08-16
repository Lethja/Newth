#include "../common/server.h"

static void shutdownCrash(int signal) {
    switch (signal) {
        default: /* Close like normal unless something has gone catastrophically wrong */
            platformCloseBindSockets(&serverReadSockets, serverMaxSocket);
            serverFreeResources();
            platformIpStackExit();
            printf("Emergency shutdown: %d\n", signal);
            /* Fallthrough */
        case SIGABRT:
        case SIGSEGV:
            exit(1);
    }
}

static void shutdownProgram(int signal) {
    if (signal == SIGINT)
        printf("\n"); /* Put next message on a different line from ^C */
    platformCloseBindSockets(&serverReadSockets, serverMaxSocket);
    serverFreeResources();
    platformIpStackExit();
    exit(0);
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

    if (getpeername(*sock, (struct sockaddr *) &ss, &sockLen))
        return;

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

    if (getpeername(*event->clientSocket, (struct sockaddr *) &sock, &sockLen))
        return;

    platformGetIpString((struct sockaddr *) &sock, ip, &family);
    port = platformGetPort((struct sockaddr *) &sock);
    if (family == AF_INET)
        printf("%c%03d:%s:%u/%s\n", type, *event->response, ip, port, event->path);
    else if (family == AF_INET6)
        printf("%c%03d:[%s]:%u/%s\n", type, *event->response, ip, port, event->path);
}

static void printAdapterInformation(char *protocol, sa_family_t family, unsigned short port) {
    AdapterAddressArray *adapters = platformGetAdapterInformation(family);
    size_t i, j;
    for (i = 0; i < adapters->size; ++i) {
        printf("%s:\n", adapters->adapter[i].name);
        for (j = 0; j < adapters->adapter[i].addresses.size; ++j) {
            if (!adapters->adapter[i].addresses.array[j].type)
                printf("\t%s://%s:%u\n", protocol, adapters->adapter[i].addresses.array[j].address,
                       port);
            else
                printf("\t%s://[%s]:%u\n", protocol, adapters->adapter[i].addresses.array[j].address,
                       port);
        }
    }

    platformFreeAdapterInformation(adapters);
}

static int setup(int argc, char **argv) {
    sa_family_t family;
    SOCKET *sockets;
    char *ports, *err;

    /* Get the list of ports then try to bind one */
    ports = getenv("TH_HTTP_PORT");
    platformArgvGetFlag(argc, argv, 'p', "port", &ports);
    if (!ports)
        ports = "0";

    /* Choose IP standards to run on the socket */
    if (platformArgvGetFlag(argc, argv, '6', "ipv6", NULL) && !platformArgvGetFlag(argc, argv, '4', "ipv4", NULL))
        family = AF_INET6;
    else if (platformOfficiallySupportsIpv6() || platformArgvGetFlag(argc, argv, '\0', "dual-stack", NULL))
        family = AF_UNSPEC;
    else
        family = AF_INET;

    err = platformIpStackInit();
    if (err) {
        puts(err);
        return 1;
    }

    serverMaxSocket = 0;
    sockets = platformServerStartup(family, ports, &serverMaxSocket, &err);
    if (!sockets) {
        LINEDBG;
        puts(err);
        platformIpStackExit();
        return 1;
    }

    printAdapterInformation("http", family, getPort(&sockets[1]));

    FD_ZERO(&serverReadSockets);
    FD_ZERO(&serverWriteSockets);

    socketArrayToFdSet(sockets, &serverListenSockets);
    socketArrayToFdSet(sockets, &serverReadSockets);
    serverListenSocket = sockets;

    globalRoutineArray.size = 0, globalRoutineArray.array = NULL;

    return 0;
}

int main(int argc, char **argv) {
    LINEDBG;

    platformConnectSignals(noAction, shutdownCrash, shutdownProgram);

    if (argc > 1) {
        globalRootPath = platformGetRootPath(argv[1]);
    } else {
        char *buf = malloc(BUFSIZ + 1), *test = platformGetWorkingDirectory(buf, BUFSIZ);

        if (buf) {
            buf[BUFSIZ] = '\0';
            if (test)
                globalRootPath = platformGetRootPath(test);

            free(buf);
        } else {
            puts("Unable to get root path");
            return 1;
        }
    }

    printf("Root Path: %s\n\n", globalRootPath);

    if (setup(argc, argv))
        return 1;

    eventHttpRespondSetCallback(printHttpEvent);
    eventHttpFinishSetCallback(printHttpEvent);
    eventSocketAcceptSetCallback(printSocketAccept);
    eventSocketCloseSetCallback(printSocketClose);

    serverTick();

    return 0;
}
