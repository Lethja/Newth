#ifndef NEW_DL_URI_H
#define NEW_DL_URI_H

#include "io.h"
#include "../platform/platform.h"

typedef enum UriScheme {
    SCHEME_FILE = 1, SCHEME_HTTP = 80, SCHEME_HTTPS = 443, SCHEME_FTP = 21, SCHEME_FTPS = 990, SCHEME_UNKNOWN = 0
} UriScheme;

typedef struct UriDetails {
    char *scheme, *host, *path, *port, *query;
} UriDetails;

/**
 * Turn a URI into a UriDetails for easier processing
 * @param uri The full URI address to process or NULL
 * @return UriDetails with all applicable data set
 * @remark The returned UriDetails should be passed to uriDetailsFree()
 * before leaving scope to free memory
 */
extern UriDetails uriDetailsNewFrom(const char *uri);

/**
 * Convert a UriDetails back into a string representation
 * @param details The details to convert back into a string
 * @return A string with the URI address on success, NULL on failure
 * @remark Returned string should be freed before leaving scope
 */
extern char *uriDetailsCreateString(UriDetails *details);

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
extern unsigned short uriDetailsGetPort(const UriDetails *details);

/**
 * Get the protocol scheme in a URI
 * @param details The UriDetails to get the scheme from
 * @return UriScheme enum value
 */
extern UriScheme uriDetailsGetScheme(const UriDetails *details);

/**
 * Create a socket address from a Uris details
 * @param self In: The UriDetails to get information from
 * @param socketAddress Out: The socketAddress union to populate
 * @param scheme In: The scheme to assume if that information is absent from 'self' or 'SCHEME_UNKNOWN'
 * @return NULL on success, user friendly error message otherwise
 */
extern const char *uriDetailsCreateSocketAddress(UriDetails *self, SocketAddress *socketAddress, unsigned short scheme);

/**
 * Combine two path strings together
 * @param output Out: A string buffer with at least the combine amount of space for path1 and path 2
 * @param path1 In: The string to combine on the left
 * @param path2 In: The string to combine on the right
 * @remark It is the callers responsibility to make sure that output is equal or larger
 * than strlen(path1) + strlen(path2) + 2
 */
extern void uriPathCombine(char *output, const char *path1, const char *path2);

/**
 * Get the name on the end of the path
 * @param path The path to get the end of
 * @return The end of the path on success or NULL on error
 * @remark Depending on circumstances returned pointer could be either be a sub-string or it's own heap allocation.
 * In case of the latter the caller must free the allocation itself. Check for and handle this condition like so:
 * @code
 * char *l = uriPathLast(p);
 * if (l && (l &lt p || l &gt &p[strlen(p) + 1]))
 *     free(l);
 * @endcode
 * @note If the last node in the path has a trailing '/' it will be removed from the returned value
 */
char *uriPathLast(const char *path);

/**
 * Resolve the absolute path given a current path and a path to append
 * @param currentPath The current absolute path
 * @param append The relative (or absolute) path
 * @return String of the absolute path result or NULL on error
 * @remark Return value must be freed before leaving scope
 */
char *uriPathAbsoluteAppend(const char *currentPath, const char *append);

/**
 * Set UriDetails to this scheme
 * @param self The UriDetails to set the scheme of
 * @param scheme The scheme to set UriDetails to
 */
void uriDetailsSetScheme(UriDetails *self, enum UriScheme scheme);

/**
 * Set UriDetails to the ip address and port of this socket address
 * @param self The UriDetails to set the address and port of
 * @param socketAddress The socket address to set address and port from
 */
void uriDetailsSetAddress(UriDetails *self, struct sockaddr *socketAddress);

#ifdef UNIT_TEST

#ifdef GETHOSTBYNAME_CANT_IPV4STR

extern char isValidIpv4Str(const char *str);

#endif /* GETHOSTBYNAME_CANT_IPV4STR */

#endif /* UNIT_TEST */

#endif /* NEW_DL_URI_H */
