#include "sendbufr.h"

#include <stdarg.h>
#include <string.h>

void sendBufferFailFree(SendBuffer *self) {
	if (self->buffer)
		platformMemoryStreamFree(self->buffer);
}

SendBuffer sendBufferNew(SOCKET clientSocket, char options) {
	SendBuffer self;
	self.clientSocket = clientSocket, self.idx = 0, self.options = options, self.buffer = NULL;
	return self;
}

size_t sendBufferFlush(SendBuffer *self) {
	char data[SB_DATA_SIZE];
	SOCK_BUF_TYPE flush;
	size_t bytesFlushed = 0, read;

	if (!self->buffer)
		return bytesFlushed;

	platformMemoryStreamSeek(self->buffer, self->idx, SEEK_SET);

	do {
		read = fread(data, 1, SB_DATA_SIZE, self->buffer);

		if (read == 0) {
			#pragma region Handle end of memory stream
			platformMemoryStreamFree(self->buffer), self->buffer = NULL, self->idx = 0;
			#pragma endregion
			return bytesFlushed;
		}

		flush = send(self->clientSocket, data, (SOCK_BUF_TYPE) read, 0);
		if (flush == -1) {
			int error = platformSocketGetLastError();
			switch (error) { /* NOLINT(*-multiway-paths-covered) */
					#pragma region Handle socket error
				case SOCKET_TRY_AGAIN:
#if SOCKET_TRY_AGAIN != SOCKET_WOULD_BLOCK
				case SOCKET_WOULD_BLOCK:
#endif
					return bytesFlushed;
					#pragma endregion
				default:
					self->options = SOC_BUF_ERR_FAIL;
					return 0;
			}

		} else if ((size_t) flush < read) {
			platformMemoryStreamSeek(self->buffer, (long) (flush - read), SEEK_CUR);
			self->idx += (long) flush;
			bytesFlushed += flush;
			return bytesFlushed;
		}

		self->idx += (long) flush;
		bytesFlushed += flush;
	} while (1);
}

size_t sendBufferWriteData(SendBuffer *self, const char *data, size_t len) {
	SOCK_BUF_TYPE sent;
	size_t bytesSent = 0;

	if (!self->buffer) {
		#pragma region Attempt a direct send to the socket
		sent = send(self->clientSocket, data, len, 0);

		if (sent == -1) {
			int error = platformSocketGetLastError();
			switch (error) { /* NOLINT(*-multiway-paths-covered) */
					#pragma region Handle socket error
				case SOCKET_TRY_AGAIN:
#if SOCKET_TRY_AGAIN != SOCKET_WOULD_BLOCK
				case SOCKET_WOULD_BLOCK:
#endif
					break;
					#pragma endregion
				default:
					self->options = SOC_BUF_ERR_FAIL;
					return 0;
			}
		} else
			bytesSent = sent;
		#pragma endregion

		if (bytesSent < len) {
			#pragma region Write what does not fit into a memory stream
			self->buffer = platformMemoryStreamNew();
			bytesSent += fwrite(&data[len - (len - bytesSent)], 1, len - bytesSent, self->buffer);
			#pragma endregion
		}
		return bytesSent;
	} else {
		#pragma region Append onto the end of the memory stream
		platformMemoryStreamSeek(self->buffer, 0, SEEK_END);
		bytesSent = fwrite(data, 1, len, self->buffer);
		#pragma endregion
		return bytesSent;
	}
}

size_t sendBufferWriteText(SendBuffer *self, const char *data) {
	return sendBufferWriteData(self, data, strlen(data));
}

FILE *sendBufferGetBuffer(SendBuffer *self) {
	if (!self->buffer)
		self->buffer = platformMemoryStreamNew();
	else
		platformMemoryStreamSeek(self->buffer, 0, SEEK_END);

	return self->buffer;
}

int sendBufferPrintf(SendBuffer *self, size_t max, const char *format, ...) {
	int e;
	va_list args;
	va_start(args, format);
	if (!self->buffer) {
		char *msg = malloc(max);

		if (!msg) {
			va_end(args);
			return -1;
		}

		e = vsprintf(msg, format, args);
		sendBufferWriteText(self, msg);
		free(msg);
	} else {
		platformMemoryStreamSeek(self->buffer, 0, SEEK_END);
		e = vfprintf(self->buffer, format, args);
	}
	va_end(args);

	return e;
}
