#ifndef NEW_DL_SITE_H
#define NEW_DL_SITE_H

#include "../platform/platform.h"

typedef struct SiteDirectoryEntry {
    char *name;
    time_t modifiedDate;
    char isDirectory;
} SiteDirectoryEntry;

#include "site/file.h"
#include "site/http.h"

enum SiteType {
    SITE_FILE, SITE_HTTP
};

typedef struct Site {
    int type;
    union site {
        FileSite file;
        HttpSite http;
    } site;
} Site;

#pragma region Site Array

/**
 * Return the active site structure
 * @return Pointer to the currently active site
 */
Site *siteArrayActiveGet(void);

/**
 * Return the active site number
 * @return Entry number of the active site
 */
long siteArrayActiveGetNth(void);

/**
 * Set the active site by pointer
 * @param site The site to set to active
 */
void siteArrayActiveSet(Site *site);

/**
 * Set the active site by number
 * @param siteNumber The site number to set to active
 * @return NULL on success, user friendly error message as to why the site couldn't be changed otherwise
 */
const char *siteArrayActiveSetNth(long siteNumber);

/**
 * Is the site id valid
 * @param siteNumber The site number to check
 * @return Non-zero when site number is valid
 */
char siteArrayNthMounted(long siteNumber);

/**
 * Add a site to the list of mounted devices
 * @param site The site to be added
 * @return NULL on success, user friendly error message on failure
 */
char *siteArrayAdd(Site *site);

/**
 * Free all memory inside a siteArray made with siteArrayInit()
 */
void siteArrayFree(void);

/**
 * Initialize a siteArray system
 * @return Structure with a file:// site pre-mounted
 * @remark Free with siteArrayFree() after use
 */
char *siteArrayInit(void);

/**
 * Array pointer to iterate over, use with caution
 * @param length Out/Null: The size of the array
 * @return A pointer to the site array
 */
Site *siteArrayPtr(long *length);

/**
 * Remove this site from mounted devices if found
 * @param site The site to be removed
 */
void siteArrayRemove(Site *site);

/**
 * Remove index n from mounted devices
 * @param n the mounted device to be removed
 */
void siteArrayRemoveNth(long n);

#pragma endregion

#pragma region Memory Functions

/**
 * Free internal memory of a SiteDirectoryEntry
 * @param entry The SiteDirectoryEntry to be freed
 */
void siteDirectoryEntryFree(void *entry);

#pragma endregion

#pragma region Site Base Functions

/**
 * Site-like implementation of POSIX 'opendir()'
 * @param self In: The site to open the path from
 * @param path In: The path to open the directory of. Can be relative to current directory, uri, or absolute
 * @return Pointer to a listing to be used on siteReadDirectoryListing or NULL on error
 * @remark Returned pointer must be freed with siteCloseDirectoryListing()
 */
void *siteDirectoryListingOpen(Site *self, char *path);

/**
 * Get the file stats of a entry
 * @param self In: the site relative to the listing
 * @param listing In: The listing returned from siteDirectoryListingOpen()
 * @param entry In: The entry returned from siteDirectoryListingRead()
 * @param st Out: The file stat structure to populate
 * @return NULL on success, user friendly error message otherwise
 */
const char *siteDirectoryListingEntryStat(Site *self, void *listing, void *entry, PlatformFileStat *st);

/**
 * Site-like implementation of POSIX 'readdir()'
 * @param self In: The site relative to the listing
 * @param listing In: The listing returned from siteDirectoryListingOpen()
 * @return A SiteDirectoryEntry with the next files information
 * @remark Returned pointer must be freed with siteDirectoryEntryFree()
 */
SiteDirectoryEntry *siteDirectoryListingRead(Site *self, void *listing);

/**
 * Site-like implementation of POSIX 'closedir()'
 * @param self In: The site relative to the listing
 * @param listing In: The listing returned from siteDirectoryListingOpen() to free internal memory from
 */
void siteDirectoryListingClose(Site *self, void *listing);

/**
 * Get the current working directory of site
 * @param self In: The site to get the directory of
 * @return The full uri of the sites working directory
 * @remark Do not free returned value
 */
char *siteWorkingDirectoryGet(Site *self);

/**
 * Set the working directory of a site
 * @param self In/Mod: The site to set the directory of
 * @param path In: The path to set the directory to. Can be relative to current directory, uri, or absolute
 * @return 0 on success, positive number on path error, negative value on unknown errors
 */
int siteWorkingDirectorySet(Site *self, char *path);

/**
 * Free internal members of a site
 * @param self The site pointer to free internal members of
 */
void siteFree(Site *self);

/**
 * Create a new site that's ready to mount
 * @param site Out: Pointer to the site to populate
 * @param type In: The type of site to create
 * @param path In: The current site to mount or NULL for default
 * @return NULL on success, user friendly error message otherwise
 */
const char *siteNew(Site *site, enum SiteType type, const char *path);

void siteFileClose(Site *self);

SOCK_BUF_TYPE siteFileRead(Site *self, char *buffer, SOCK_BUF_TYPE size);

const char *siteFileOpenRead(Site *self, const char *path);

const char *siteFileOpenWrite(Site *self, const char *path);

SOCK_BUF_TYPE siteFileWrite(Site *self, char *buffer, SOCK_BUF_TYPE size);

#pragma endregion

#endif /* NEW_DL_SITE_H */
