#ifndef NEW_DL_SITE_H
#define NEW_DL_SITE_H

#include "../platform/platform.h"

enum SiteFileType {
    /**
     * @brief A regular file that can be read and written to
     */
    SITE_FILE_TYPE_FILE = 4,
    /**
     * @brief A directory (a.k.a folder) that may contain files and/or other directories
     */
    SITE_FILE_TYPE_DIRECTORY = 2,
    /**
     * @brief The path does not exist
     */
    SITE_FILE_TYPE_NOTHING = 1,
    /**
     * @brief The path either has not been checked or is not a file or directory
     */
    SITE_FILE_TYPE_UNKNOWN = 0
};

/**
 * @struct SiteFileMeta
 * @brief Contains metadata about a site path
 */
typedef struct SiteFileMeta {
    /**
     * @brief The date that a file was modified or 0 when unknown
     */
    PlatformTimeStruct *modifiedDate;

    /**
     * @brief The length of a file in bytes or 0 when unknown
     */
    PlatformFileOffset length;

    /**
     * @brief The preferred name to use if the file is to be referenced or copied. NULL when unknown
     * @remark Circumstances may make fileName either point it's own heap allocation or within fullPath.
     * Do not manually modify
     */
    char *name;

    /**
     * @brief The full path relative to the site uri
     */
    char *path;

    /**
     * @brief The type (a.k.a mode) of this file entry
     * @ref SiteFileType
     */
    char type;
} SiteFileMeta;

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
 * Free internal memory of a SiteFileMeta
 * @param meta The SiteFileMeta to free internal memory of
 */
void siteFileMetaFree(SiteFileMeta *meta);

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
 * @param meta Out: The meta structure to populate
 * @return NULL on success, user friendly error message otherwise
 */
const char *siteDirectoryListingEntryStat(Site *self, void *listing, void *entry, SiteFileMeta *meta);

/**
 * Site-like implementation of POSIX 'readdir()'
 * @param self In: The site relative to the listing
 * @param listing In: The listing returned from siteDirectoryListingOpen()
 * @return A SiteDirectoryEntry with the next files information
 * @remark Returned pointer must be freed with siteDirectoryEntryFree()
 */
SiteFileMeta *siteDirectoryListingRead(Site *self, void *listing);

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

/**
 * Close the currently open file if any
 * @param self The site to close the file on
 */
void siteFileClose(Site *self);

/**
 * Check if the open file stream on the site has reached the end
 * @param self The site to check if the open file stream has reached the end of
 * @return 0 if the sites file hasn't reached the end, positive number if it has, negative it can't be determined
 */
int siteFileAtEnd(Site *self);

/**
 * Read from the open file
 * @param self The site to read the open file from
 * @param buffer The buffer to write the file streams data to
 * @param size The size of data in bytes to transfer into the buffer
 * @return Amount of bytes transferred or -1 on error
 */
SOCK_BUF_TYPE siteFileRead(Site *self, char *buffer, SOCK_BUF_TYPE size);

/**
 * Open file at path, get metadata from it then close the file
 * @param self The site to the the meta from
 * @param path The path to the the meta from
 * @return NULL of failure, allocation of files metadata on success
 * @remark Before checking any other data make sure type is not equal to NOTHING. If it is then file doesn't exist
 * @remark Use siteFileMetaFree() then free() on returned pointer before leaving scope
 */
SiteFileMeta *siteStatOpenMeta(Site *self, const char *path);

/**
 * Get the file meta of the currently open file on a site
 * @param self The site to the the file meta from
 * @return A pointer to a SiteFileMeta struct populated with the currently open file or NULL on error
 * @remark Do not free return pointer
 */
SiteFileMeta *siteFileOpenMeta(Site *self);

/**
 * Read to a open file on the site
 * @param self The site to read a file on
 * @param path The path (relative or absolute to the path of the URI)
 * @param start Number of bytes to start writing at or -1 for default
 * @param end Number of bytes to end writing at or -1 for default
 * @return NULL on success, user friendly error message otherwise
 * @remark By design sites can only have one file open at a time and it can only be opened to read or write, not both.
 * Opening another file will close the first.
 * @remark Some types of sites are more strict than others with the start and/or finish byte requested
 */
const char *siteFileOpenRead(Site *self, const char *path, PlatformFileOffset start, PlatformFileOffset end);

/**
 * Open a file on the site for appending to an existing file
 * @param self The site to write a file on
 * @param path The path (relative or absolute to the path of the URI)
 * @param start Number of bytes to start writing at or -1 for default
 * @param end Number of bytes to end writing at or -1 for default
 * @return NULL on success, user friendly error message otherwise
 * @remark By design sites can only have one file open at a time and it can only be opened to read or write, not both.
 * Opening another file will close the first.
 * @remark Some sites types are more strict than others on a valid file and byte range requested. Be sure the
 * requested sense. Use siteFileStatMeta() to get if a file exists and what it's length is if it does
 */
const char *siteFileOpenAppend(Site *self, const char *path, PlatformFileOffset start, PlatformFileOffset end);

/**
 * Open a file on the site for writing a new file
 * @param self The site to write a file on
 * @param path The path (relative or absolute to the path of the URI)
 * @return NULL on success, user friendly error message otherwise
 * @remark By design sites can only have one file open at a time and it can only be opened to read or write, not both.
 * Opening another file will close the first.
 * @remark If a file exists this function will overwrite it, to resume a download use siteFileOpenAppend() instead
 */
const char *siteFileOpenWrite(Site *self, const char *path);

/**
 * Write to the open file of a site
 * @param self The site to write the open file to
 * @param buffer The buffer to write into the sites file stream
 * @param size The size of data in bytes to transfer into the buffer
 * @return Amount of bytes transferred or -1 on error
 */
SOCK_BUF_TYPE siteFileWrite(Site *self, char *buffer, SOCK_BUF_TYPE size);

#pragma endregion

#endif /* NEW_DL_SITE_H */
