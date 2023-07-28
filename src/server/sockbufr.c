#include "sockbufr.h"

#include <string.h>

SocketBuffer socketBufferNew(SOCKET clientSocket, char options) {
    SocketBuffer self;
    self.clientSocket = clientSocket, self.idx = 0, self.options = options, self.buffer = NULL;
    return self;
}

size_t socketBufferFlush(SocketBuffer *self) {
    char data[SB_DATA_SIZE];
    SOCK_BUF_TYPE flush;
    size_t bytesFlushed = 0, read;

    if (!self->buffer)
        return bytesFlushed;

    platformFileSeek(self->buffer, self->idx, SEEK_SET);

    do {
        read = fread(data, 1, SB_DATA_SIZE, self->buffer);

        if (read == 0) {
#pragma region Handle end of memory stream
            fclose(self->buffer), self->buffer = NULL, self->idx = 0;
#pragma endregion
            return bytesFlushed;
        }

        flush = send(self->clientSocket, data, read, 0);
        if (flush == -1) {
            switch (platformSocketGetLastError()) {
#pragma region Handle socket error
                case SOCKET_TRY_AGAIN:
#if SOCKET_TRY_AGAIN != SOCKET_WOULD_BLOCK
                case SOCKET_WOULD_BLOCK:
#endif
                    return bytesFlushed;
#pragma endregion
                default:
                    return 0;
            }

        } else if (flush < read) {
            platformFileSeek(self->buffer, (PlatformFileOffset) (flush - read), SEEK_CUR);
            self->idx += (PlatformFileOffset) flush;
            bytesFlushed += flush;
            return bytesFlushed;
        }

        self->idx += (PlatformFileOffset) flush;
        bytesFlushed += flush;
    } while (1);
}

size_t socketBufferWriteData(SocketBuffer *self, const char *data, size_t len) {
    SOCK_BUF_TYPE sent;
    size_t bytesSent = 0;

    if (!self->buffer) {
#pragma region Attempt a direct send to the socket
        sent = send(self->clientSocket, data, len, 0);

        if (sent == -1) {

            switch (platformSocketGetLastError()) {
#pragma region Handle socket error
                case SOCKET_TRY_AGAIN:
#if SOCKET_TRY_AGAIN != SOCKET_WOULD_BLOCK
                case SOCKET_WOULD_BLOCK:
#endif
                    break;
#pragma endregion
                default:
                    return 0;
            }
        } else
            bytesSent = sent;
#pragma endregion

        if (bytesSent < len) {
#pragma region Write what does not fit into a memory stream
            self->buffer = tmpfile();
            bytesSent += fwrite(&data[len - (len - bytesSent)], 1, len - bytesSent, self->buffer);
#pragma endregion
        }
        return bytesSent;
    } else {
#pragma region Append onto the end of the memory stream
        platformFileSeek(self->buffer, 0, SEEK_END);
        bytesSent = fwrite(data, 1, len, self->buffer);
#pragma endregion
        return bytesSent;
    }
}

size_t socketBufferWriteText(SocketBuffer *self, const char *data) {
    return socketBufferWriteData(self, data, strlen(data));
}
