#ifndef NEW_DL_HTTP_H
#define NEW_DL_HTTP_H

#include "../recvbufr.h"

typedef struct HttpSiteOpenFile {
	char *fullUri;
	PlatformFileOffset start, end;
	SiteFileMeta meta;
} HttpSiteOpenFile;

typedef struct HttpSite {
	SocketAddress address;
	char *fullUri;
	HttpSiteOpenFile *file;
	RecvBuffer socket;
} HttpSite;

typedef struct HttpSiteDirectoryListing {
	time_t asOf; /* Date & Time of last reload */
	size_t idx, len; /* Position and Length of 'entry' array */
	SiteFileMeta *entry; /* Array of entries */
	char *fullUri; /* uri of current listing in the 'entry' array */
	const HttpSite *site;
} HttpSiteDirectoryListing;

void httpSiteSchemeDirectoryListingClose(void *listing);

const char *httpSiteSchemeDirectoryListingEntryStat(void *entry, void *listing, SiteFileMeta *st);

void *httpSiteSchemeDirectoryListingOpen(HttpSite *self, char *path);

void *httpSiteSchemeDirectoryListingRead(void *listing);

void httpSiteSchemeFree(HttpSite *self);

const char *httpSiteSchemeNew(HttpSite *self, const char *path);

char *httpSiteSchemeWorkingDirectoryGet(HttpSite *self);

int httpSiteSchemeWorkingDirectorySet(HttpSite *self, const char *path);

void httpSiteSchemeFileClose(HttpSite *self);

int httpSiteSchemeFileAtEnd(HttpSite *self);

SOCK_BUF_TYPE httpSiteSchemeFileRead(HttpSite *self, char *buffer, SOCK_BUF_TYPE size);

SiteFileMeta *httpSiteSchemeStatOpenMeta(HttpSite *self, const char *path);

const char *httpSiteSchemeFileOpenRead(HttpSite *self, const char *path, PlatformFileOffset start, PlatformFileOffset end);

const char *httpSiteSchemeFileOpenWrite(HttpSite *self, const char *path);

SOCK_BUF_TYPE httpSiteSchemeFileWrite(HttpSite *self, char *buffer, SOCK_BUF_TYPE size);

#endif /* NEW_DL_HTTP_H */
