#include "recvbufr.h"

char *recvBufferAppend(RecvBuffer *self, size_t len) {
    size_t i = 0;

    if (!self->buffer)
        self->buffer = platformMemoryStreamNew();
    else
        platformMemoryStreamSeek(self->buffer, SEEK_END, 0);

    while (i < len) {
        char buf[SB_DATA_SIZE];
        PlatformFileOffset l;
        size_t s = len - i;
        if (s > SB_DATA_SIZE)
            s = SB_DATA_SIZE;

        switch ((l = recv(self->serverSocket, buf, s, 0))) {
            case -1:
                /* TODO: Handle when something is wrong with the socket */
                return strerror(platformSocketGetLastError());
            case 0:
                if (i)
                    return NULL;

                return "No data to be retrieved";
            default:
                /* TODO: Get correct error to return */
                if ((fwrite(buf, 1, l, self->buffer)) != l)
                    return strerror(errno);

                self->remain -= l, self->escape += l;
                i += l;
                break;
        }
    }

    return NULL;
}

void recvBufferClear(RecvBuffer *self) {
    if (self->buffer)
        fclose(self->buffer), self->buffer = NULL;
}

void recvBufferDitch(RecvBuffer *self, PlatformFileOffset len) {
    FILE *tmp;
    SOCK_BUF_TYPE i;
    char buf[SB_DATA_SIZE];

    if (!self->buffer || !(tmp = tmpfile()))
        return;

    platformMemoryStreamSeek(self->buffer, len, SEEK_SET);
    while ((i = fread(buf, 1, SB_DATA_SIZE, self->buffer)))
        fwrite(buf, 1, i, tmp);

    fclose(self->buffer), self->buffer = tmp;
}

void recvBufferFailFree(RecvBuffer *self) {
    if (self->buffer)
        fclose(self->buffer);
}

char *recvBufferFetch(RecvBuffer *self, char *buf, PlatformFileOffset pos, size_t len) {
    if (self->buffer) {
        size_t l;
        if (fseek(self->buffer, pos, SEEK_SET))
            return strerror(errno);

        if ((l = fread(buf, 1, len - 1, self->buffer))) {
            buf[l] = '\0';
            return NULL;
        }
    }

    return "No buffered data";
}

PlatformFileOffset recvBufferFind(RecvBuffer *self, PlatformFileOffset pos, const char *token, size_t len) {
    PlatformFileOffset r;
    size_t i, j, l;
    char buf[SB_DATA_SIZE];

    if (!self->buffer)
        return -1;

    j = r = 0, platformMemoryStreamSeek(self->buffer, pos, SEEK_SET);
    while ((l = fread(buf, 1, SB_DATA_SIZE, self->buffer)) > 0) {
        for (i = 0; i < l; ++i) {
            if (buf[i] == token[j]) {
                ++j;
                if (j == len) {
                    r += pos + (PlatformFileOffset) (i - len + 1);
                    return r;
                }
            } else
                j = 0;
        }

        r += (PlatformFileOffset) l;
    }

    return -1;
}

const char *recvBufferFindAndDitch(RecvBuffer *self, const char *token, size_t len) {
    PlatformFileOffset o;
    const char *e;

    if (!self->buffer) {
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;
    }

    while ((o = recvBufferFind(self, 0, token, len)) == -1) {
        if (!(e = recvBufferAppend(self, SB_DATA_SIZE))) {
            PlatformFileOffset p;

            platformMemoryStreamSeek(self->buffer, 0, SEEK_END), p = platformMemoryStreamTell(self->buffer);
            if (p > len)
                p -= (PlatformFileOffset) len, recvBufferDitch(self, p);
        } else
            return e;
    }

    if (o > 1)
        recvBufferDitch(self, o);

    return NULL;
}

const char *recvBufferFindAndFetch(RecvBuffer *self, const char *token, size_t len, size_t max) {
    PlatformFileOffset i = 0, o;
    const char *e;

    if (!self->buffer) {
        if ((e = recvBufferAppend(self, SB_DATA_SIZE)))
            return e;
    }

    while ((o = recvBufferFind(self, i, token, len)) == -1) {
        if (!(e = recvBufferAppend(self, SB_DATA_SIZE))) {
            platformMemoryStreamSeek(self->buffer, 0, SEEK_END);

            if ((i = platformMemoryStreamTell(self->buffer)) > max)
                return "Exceeded maximum allowed buffer size";
            else
                i -= (PlatformFileOffset) len;
        } else
            return e;
    }

    platformMemoryStreamSeek(self->buffer, i + o, SEEK_CUR);
    return NULL;
}

void recvBufferUpdateSocket(RecvBuffer *self, SOCKET *socket) {
    self->serverSocket = *socket;
}

RecvBuffer recvBufferNew(SOCKET serverSocket, char options) {
    /* TODO: Make aware of site and/or socketAddress for reconnection */
    RecvBuffer self;

    self.options = options, self.serverSocket = serverSocket;
    self.escape = self.remain = 0, self.buffer = NULL;
    return self;
}