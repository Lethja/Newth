#ifndef NEW_DL_URI_H
#define NEW_DL_URI_H

#include "../platform/platform.h"

typedef enum Protocols {
    PROTOCOL_HTTP = 80, PROTOCOL_HTTPS = 443, PROTOCOL_FTP = 21, PROTOCOL_FTPS = 990, PROTOCOL_UNKNOWN = 0
} Protocols;

typedef struct UrlDetails {
    char *scheme, *host, *path, *port;
} UriDetails;

extern UriDetails UriDetailsNewFrom(const char *uri);

extern void UriDetailsFree(UriDetails *details);

#endif /* NEW_DL_URI_H */
