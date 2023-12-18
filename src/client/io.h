#ifndef NEW_DL_IO_H
#define NEW_DL_IO_H

#include "../platform/platform.h"

/* TODO: Put in a head shared with TH and DL */
#define HTTP_RES "HTTP/1.1"
#define HTTP_EOL "\r\n"

typedef struct ServerHeaderResponse {
    PlatformTimeStruct *modifiedDate; /* Last modified date (to allow HTTP 304) */
    PlatformFileOffset length; /* Length of HTTP body */
    char *filePath, *fileName;
    short dataType; /* MIME type or is it a webpage with more links */
    short httpCode; /* HTTP Response */
    int options; /* User options to change the behaviour of this download */
} ServerHeaderResponse;

enum SocketAddressState {
    SA_STATE_QUEUED,
    SA_STATE_CONNECTED,
    SA_STATE_FAILED,
    SA_STATE_FINISHED,
    SA_STATE_TRY_LATER,
    SA_STATE_ALT_MODE,
    SA_STATE_ENCRYPTION
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

extern char *ioCreateSocketFromSocketAddress(SocketAddress *self, SOCKET *sock);

extern char *ioHttpHeadRead(const SOCKET *socket, char **header);

extern char *ioHttpHeadRequest(const SOCKET *socket, const char *path, const char *extra);

extern char *ioHttpGetRequest(const SOCKET *socket, const char *path, const char *extra);

extern char *HttpGetEssentialResponse(const char *headerFile, char **scheme, char **response);

extern char *FindHeader(const char *headerFile, const char *header, char **variable);

extern char *SendRequest(const SOCKET *socket, const char *type, const char *path, const char *extra);

#endif /* NEW_DL_IO_H */
