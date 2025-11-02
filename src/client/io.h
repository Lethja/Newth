#ifndef NEW_DL_IO_H
#define NEW_DL_IO_H

#include "../platform/platform.h"
#include "../common/defines.h"
#include "recvbufr.h"

typedef struct HttpResponseHeader {
	/**
	 * @brief Last modified date (to allow HTTP 304)
	 */
	PlatformTimeStruct *modifiedDate;
	/**
	 * @brief Content-Length field; of HTTP body
	 */
	PlatformFileOffset length;
	/**
	 * @brief Content-Disposition field; filename if provided
	 */
	char *fileName;
	/**
	 * @brief The queue state, probably not needed
	 */
	char state;
	/**
	 * @brief Boolean states of the http header
	 */
	char protocol;
} HttpResponseHeader;

enum HttpStateFlags {
	/**
	 * @brief The http connection has TLS
	 */
	HTTP_ENCRYPTION,
	/**
	 * @brief The http data encoding is chunked
	 */
	HTTP_CHUCKED_MODE,
	/**
	 * @brief The http data is an attachment and should be treated like a file download
	 */
	HTTP_CONTENT_ATTACHMENT
};

enum SocketAddressFlags {
	SA_STATE_QUEUED,
	SA_STATE_CONNECTED,
	SA_STATE_FAILED,
	SA_STATE_FINISHED,
	SA_STATE_TRY_LATER
};

/**
 * Prepare SocketAddress into a socket ready for connection
 * @param self In: The socket address to attempt to build
 * @param sock Out: The socket ready to run connect on
 * @return NULL on success, user friendly error message otherwise
 * @note On success parameter 'sock' should be closed before going out of scope
 */
extern char *ioCreateSocketFromSocketAddress(SocketAddress *self, SOCKET *sock);

/**
 * Convert hex string into a number
 * @param hex In: The hex string to convert
 * @param value Out: The number the hex string represents
 * @return NULL on success, user friendly error message otherwise
 */
const char *ioHttpBodyChunkHexToSize(const char *hex, size_t *value);

/**
 * Reformat a stream of data to count and remove HTTP chunks
 * @param data In-Out: The data buffer with http chunks that should be parsed and removed from the content stream.
 * @param max In-Out: The maximum length of the data buffer.
 * This parameter may return with a smaller number then called with to indicate the new length of the data buffer
 * @param len In-Out: The length remaining in the current chunk. Should be set to -1 at the beginning of a chunk body.
 * This value has reentrant significance and should not be written to while chunk data is being parsed.
 * It will be set back to -1 to indicate the last chunk has been read.
 * @return NULL on success, user friendly error message otherwise
 * @remark When returning 'Chunk metadata overflows buffer' valid data will be available up until the overflowing chunk.
 * In this case the application can process data up to max like normal then should seek the buffer to the 'max' position
 * so that the next chunk can be read in it's entirety.
 */
const char *ioHttpBodyChunkStrip(char *data, size_t *max, size_t *len);

/**
 * Generate a GET HTTP 1.1 request ready for sending to a remote server
 * @param path In: The path to request
 * @param extra In: The raw string to append after the path or NULL
 * @return A string of the GET request on success, NULL on failure
 */
extern char *ioHttpRequestGenerateGet(const char *path, const char *extra);

/**
 * Generate a HEAD HTTP 1.1 request ready for sending to a remote server
 * @param path In: The path to request
 * @param extra In: The raw string to append after the path or NULL
 * @return A string of the HEAD request on success, NULL on failure
 */
extern char *ioHttpRequestGenerateHead(const char *path, const char *extra);

/**
 * Generate a raw HTTP 1.1 request ready for sending to a remote server
 * @param type In: The type to request
 * @param path In: The path to request
 * @param extra In: The raw string to append after the path or NULL
 * @return A string of the HTTP request on success, NULL on failure
 */
extern char *ioHttpRequestGenerateSend(const char *type, const char *path, const char *extra);

/**
 * Get essential HTTP/1.1 head response from the server
 * @param headerFile In: The header response to read from
 * @param scheme Out: The scheme of the response (usually HTTP 1.1)
 * @param response Out: The response number to the request in string format (200 OK)
 * @return NULL on success, user friendly error message otherwise
 * @remark 'scheme' should be freed before leaving scope
 * @remark 'scheme' and 'response' are part of the same memory allocation, 'response' isn't available if 'scheme' is freed
 */
extern char *ioHttpResponseHeaderEssential(const char *headerFile, char **scheme, char **response);

/**
 * Search for a specific header in a full header string
 * @param headerFile In: A header string (like the one returned by ioHttpResponseHeaderRead()) to look for the header in
 * @param header In: The header name to look for
 * @param variable Out: The variable of the header if found
 * @return NULL on success, user friendly error message otherwise
 * @remark On success parameter 'variable' should be freed before leaving scope
 */
extern char *ioHttpResponseHeaderFind(const char *headerFile, const char *header, char **variable);

/**
 * Read http header from a socket, leave the socket stream at the start of the HTTP body if there is one
 * @param socket In: The socket to read a http header from
 * @param header Out: Heap allocation containing the header
 * @return NULL on success, user friendly error message otherwise
 * @note On success out parameter 'header' should be freed before leaving scope
 */
extern const char *ioHttpResponseHeaderRead(RecvBuffer *socket, char **header);

#endif /* NEW_DL_IO_H */
