#ifndef NEW_DL_IO_H
#define NEW_DL_IO_H

#include "../platform/platform.h"
#include "../common/defines.h"

typedef struct HttpResponseHeader {
    PlatformTimeStruct *modifiedDate; /* Last modified date (to allow HTTP 304) */
    PlatformFileOffset length; /* Length of HTTP body */
    char *fileName; /* Content-Disposition given file name if applicable */
    int options; /* User options to change the behaviour of this download */
} HttpResponseHeader;

enum SocketAddressFlags {
    SA_STATE_QUEUED,
    SA_STATE_CONNECTED,
    SA_STATE_FAILED,
    SA_STATE_FINISHED,
    SA_STATE_TRY_LATER,
    SA_PROTOCOL_ENCRYPTION,
    SA_PROTOCOL_ALT_MODE,
    SA_PROTOCOL_DIRECTORY
};

typedef struct SocketAddress {
    union address {
        struct sockaddr sock;
        struct sockaddr_in ipv4;
#ifdef ENABLE_IPV6
        struct sockaddr_in6 ipv6;
#endif
        struct sockaddr_storage storage;
    } address;
    unsigned short scheme, state;
} SocketAddress;

/**
 * Prepare SocketAddress into a socket ready for connection
 * @param self In: The socket address to attempt to build
 * @param sock Out: The socket ready to run connect on
 * @return NULL on success, user friendly error message otherwise
 * @note On success parameter 'sock' should be closed before going out of scope
 */
extern char *ioCreateSocketFromSocketAddress(SocketAddress *self, SOCKET *sock);

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
extern char *ioHttpResponseHeaderRead(const SOCKET *socket, char **header);

#endif /* NEW_DL_IO_H */
