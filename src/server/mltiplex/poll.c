#include "../../platform/platform.h"
#include "../server.h"

#include <sys/poll.h>

#pragma region Internal variables and functions

static struct pollfd *listenPoll, *servePoll = NULL, *deferredPoll = NULL;
static nfds_t listenPollCount = 0, servePollCount = 0, deferredPollCount = 0;

static inline char PollArrayAdd(struct pollfd **poll, nfds_t *count, SOCKET socket) {
    if (*poll) {
        void *new = realloc(servePoll, sizeof(struct pollfd) * servePollCount + 1);
        if (new)
            *poll = new;
        else
            return 1;
    } else
        *poll = malloc(sizeof(struct pollfd));

    if (!*poll)
        return 1;

    *&poll[servePollCount]->fd = socket, *&poll[servePollCount]->events = POLL_IN | POLL_OUT;
    ++*count;
    return 0;
}

static inline void PollArrayDel(struct pollfd **poll, nfds_t *count, SOCKET socket) {
    nfds_t i;

    if (!*poll)
        return;

    for (i = 0; i < *count; ++i) {
        void *new;
        if (*&poll[i]->fd != socket)
            continue;

        memmove(*&poll[i], *&poll[i + 1], sizeof(struct pollfd) * (*count - (i + 1))), --*count;
        if (*count == 0)
            free(*poll), *poll = NULL;
        else {
            new = realloc(servePoll, sizeof(struct pollfd) * *count);
            if (new)
                servePoll = new;
        }
    }
}

static inline char PollArrayContains(struct pollfd **poll, const nfds_t *count, SOCKET socket) {
    nfds_t i;

    if (!poll || !*poll)
        return 0;

    for (i = 0; i < *count; ++i)
        if (*&poll[i]->fd == socket)
            return 1;

    return 0;
}

static inline void ProcessListeningPoll(struct pollfd *poll) {
    switch (poll->revents) {
        case POLL_IN: {
            SOCKET clientSocket = platformAcceptConnection(poll->fd);
            PollArrayAdd(&servePoll, &servePollCount, clientSocket);
        }
    }
}

static inline void ProcessServePoll(struct pollfd *poll) {
    switch (poll->revents) {
        case POLL_IN: {
            if (serverDeferredSocketExists(poll->fd))
                return;

            if (handleConnection(poll->fd)) {
                eventSocketCloseInvoke(&poll->fd);
                RoutineArrayDelSocket(&globalRoutineArray, poll->fd);
                CLOSE_SOCKET(poll->fd);
                PollArrayDel(&servePoll, &servePollCount, poll->fd);
            }
        }
    }
}

static inline SOCKET *pollFdToSocketArray(struct pollfd *poll, const nfds_t *count) {
    nfds_t i, j;
    SOCKET *r = malloc(sizeof(struct pollfd) * (*count + 1));

    if (!r)
        return NULL;

    r[0] = (SOCKET) *count;
    for (i = 0, j = 1; i < *count; ++i, ++j)
        r[j] = poll[i].fd;

    return r;
}

#pragma endregion

SOCKET *serverGetListenSocketArray(void) {
    return pollFdToSocketArray(listenPoll, &listenPollCount);
}

SOCKET *serverGetReadSocketArray(void) {
    return pollFdToSocketArray(servePoll, &servePollCount);
}

SOCKET *serverGetWriteSocketArray(void) {
    return pollFdToSocketArray(deferredPoll, &deferredPollCount);
}

char serverDeferredSocketAdd(SOCKET socket) {
    return PollArrayAdd(&deferredPoll, &deferredPollCount, socket);
}

void serverDeferredSocketRemove(SOCKET socket) {
    PollArrayDel(&deferredPoll, &deferredPollCount, socket);
}

char serverDeferredSocketExists(SOCKET socket) {
    return PollArrayContains(&deferredPoll, &deferredPollCount, socket);
}

void serverSetup(SOCKET *sockets) {
    SOCKET i, j;

    listenPollCount = sockets[0];
    listenPoll = calloc(sizeof(struct pollfd), sockets[0]);

    for (j = 0, i = 1; i < sockets[0]; ++i, ++j)
        listenPoll[j].fd = sockets[i], listenPoll[j].events = POLL_IN;
}

void serverTick(void) {
    SOCKET deferred = 0;
    while (serverRun) {
        nfds_t i;
        RoutineTick(&globalRoutineArray, &deferred);

        if (servePoll) {
            poll(servePoll, servePollCount, 0);
            for (i = 0; i < servePollCount; ++i)
                ProcessServePoll(&servePoll[i]);
        }

        poll(listenPoll, listenPollCount, servePoll ? 0 : 1000);
        for (i = 0; i < listenPollCount; ++i)
            ProcessListeningPoll(&listenPoll[i]);

#ifdef PLATFORM_SHOULD_EXIT
        serverRun = PLATFORM_SHOULD_EXIT();
#endif
    }
}
