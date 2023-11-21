#ifndef NEW_DL_URI_H
#define NEW_DL_URI_H

#include "../platform/platform.h"

typedef enum Protocols {
    PROTOCOL_HTTP = 80, PROTOCOL_HTTPS = 443, PROTOCOL_FTP = 21, PROTOCOL_FTPS = 990, PROTOCOL_UNKNOWN = 0
} UriScheme;

typedef struct UrlDetails {
    char *scheme, *host, *path, *port;
} UriDetails;

extern UriDetails uriDetailsNewFrom(const char *uri);

extern void uriDetailsFree(UriDetails *details);

extern UriScheme uriDetailsGetScheme(UriDetails *details);

#endif /* NEW_DL_URI_H */
