#ifndef NEW_DL_URI_H
#define NEW_DL_URI_H

#include "io.h"
#include "../platform/platform.h"

typedef enum UriScheme {
    SCHEME_HTTP = 80, SCHEME_HTTPS = 443, SCHEME_FTP = 21, SCHEME_FTPS = 990, SCHEME_UNKNOWN = 0
} UriScheme;

typedef struct UriDetails {
    char *scheme, *host, *path, *port;
} UriDetails;

/**
 * Turn a URI into a UriDetails for easier processing
 * @param uri The full URI address to process
 * @return UriDetails with all applicable data set
 * @remark The return UriDetails should be passed to uriDetailsFree()
 * before leaving scope to free memory
 */
extern UriDetails uriDetailsNewFrom(const char *uri);

/**
 * Free memory in a UriDetails structure
 * @param details The UriDetails to free
 */
extern void uriDetailsFree(UriDetails *details);

/**
 * Return the raw address of a hostname stored in UriDetails
 * @param details The UriDetails to convert the hostname from
 * @return A IPV4 or IPV6 address in string form or NULL if the hostname couldn't be resolved
 * @remark Returned string should be freed before leaving scope
 */
extern char *uriDetailsGetHostAddr(UriDetails *details);

/**
 * Get the port number in a URI as a unsigned 16-bit number
 * @param details The UriDetails to convert the port from
 * @return The port number or 0 on error
 */
extern unsigned short uriDetailsGetPort(UriDetails *details);

/**
 * Get the protocol scheme in a URI
 * @param details The UriDetails to get the scheme from
 * @return UriScheme enum value
 */
extern UriScheme uriDetailsGetScheme(UriDetails *details);

/**
 * Create a socket address from a Uris details
 * @param self In: The UriDetails to get information from
 * @param socketAddress Out: The socketAddress union to populate
 * @param scheme In: The scheme to assume if that information is absent from 'self' or 'SCHEME_UNKNOWN'
 * @return 0 on success other on error
 */
extern char uriDetailsCreateSocketAddress(UriDetails *self, SocketAddress *socketAddress, unsigned short scheme);

#ifdef UNIT_TEST

#ifdef GETHOSTBYNAME_CANT_IPV4STR

extern char isValidIpv4Str(const char *str);

#endif /* GETHOSTBYNAME_CANT_IPV4STR */

#endif /* UNIT_TEST */

#endif /* NEW_DL_URI_H */
