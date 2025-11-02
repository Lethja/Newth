#include "../site.h"
#include "file.h"
#include "../uri.h"

#include <sys/stat.h>
#include <time.h>

#pragma region Static Helper Functions

/**
 * Update the full working directory URI
 * @param self The FileSite to update
 * @param path The current path to update the FileSite to
 */
static inline void UpdateFileUri(FileSite *self, char *path) {
	if (self->fullUri)
		free(self->fullUri);

	self->fullUri = platformPathSystemToFileScheme(path);
}

/**
 * Convert PlatformFileStat to FileSite
 * @param path In: The absolute system path used to open the stat
 * @param siteFileMeta Out: The FileSite to fill to
 */
static inline void FillFileMeta(char *path, SiteFileMeta *siteFileMeta) {
	PlatformFileStat stat;

	if ((siteFileMeta->path = platformPathSystemToFileScheme(path)))
		siteFileMeta->name = uriPathLast(siteFileMeta->path);

	if (platformFileStat(path, &stat)) {
		siteFileMeta->type = errno == ENOENT ? SITE_FILE_TYPE_NOTHING : SITE_FILE_TYPE_UNKNOWN;
		siteFileMeta->length = 0;
		return;
	}

	siteFileMeta->length = stat.st_size;

	if (platformFileStatIsFile(&stat))
		siteFileMeta->type = SITE_FILE_TYPE_FILE;
	else if (platformFileStatIsDirectory(&stat))
		siteFileMeta->type = SITE_FILE_TYPE_DIRECTORY;
	else
		siteFileMeta->type = SITE_FILE_TYPE_UNKNOWN;

	if ((siteFileMeta->modifiedDate = malloc(sizeof(PlatformTimeStruct))))
		platformGetTimeStruct(&stat.st_mtime, siteFileMeta->modifiedDate);
}

/**
 * Resolve a URI to a system path
 * @param uri In: The current absolute uri (including file://)
 * @param path In: The relative path from this uri
 * @return A system path
 */
static inline char *ResolveToSystemPath(char *uri, char *path) {
	char *fullPath, *sysPath;

	if ((fullPath = uriPathAbsoluteAppend(&uri[7], path))) {
		if ((sysPath = platformPathFileSchemePathToSystem(fullPath))) {
			free(fullPath);

			return sysPath;
		}

		free(fullPath);
	}

	return NULL;
}

/**
 * Open a file for reading or writing
 * @param self The site to open a file on
 * @param path The absolute or relative path to open in regards to the site
 * @param mode The mode to 'fopen()' mode to open file in
 * @return NULL on success, user friendly error message otherwise
 */
static inline const char *FileOpen(FileSite *self, const char *path, const char *mode) {
	char *fullPath;
	fileSiteSchemeFileClose(self);

	if (!(fullPath = ResolveToSystemPath(self->fullUri, (char *) path)))
		return strerror(errno);

	if (!(self->file = platformFileOpen(fullPath, mode))) {
		free(fullPath);
		return strerror(errno);
	}

	FillFileMeta(fullPath, &self->meta);
	free(fullPath);

	return NULL;
}

#pragma endregion

int fileSiteSchemeWorkingDirectorySet(FileSite *self, const char *path) {
	PlatformFileStat stat;
	char *nativePath, *fullPath = self->fullUri;

	if (!(toupper(fullPath[0]) == 'F' && toupper(fullPath[1]) == 'I' && toupper(fullPath[2]) == 'L' && toupper(fullPath[3]) == 'E'
	      && fullPath[4] == ':' && fullPath[5] == '/' && fullPath[6] == '/' && fullPath[7] == '/'))
		return -1;

	fullPath = &fullPath[7];
	if (!(fullPath = uriPathAbsoluteAppend(fullPath, path)))
		return -1;

	nativePath = platformPathFileSchemePathToSystem(fullPath), free(fullPath);
	if (!nativePath)
		return -1;

	if (platformFileStat(nativePath, &stat)) {
		free(nativePath);
		errno = ENOENT;
		return 1;
	}

	if (platformFileStatIsDirectory(&stat)) {
		fullPath = platformPathSystemToFileScheme(nativePath), free(nativePath);
		if (fullPath) {
			free(self->fullUri), self->fullUri = fullPath;
			return 0;
		}

		return -1;
	}

	free(nativePath);
	errno = ENOTDIR;
	return 1;
}

void fileSiteSchemeFree(FileSite *self) {
	fileSiteSchemeFileClose(self);

	if (self->fullUri)
		free(self->fullUri);
}

char *fileSiteSchemeWorkingDirectoryGet(FileSite *self) {
	return self->fullUri;
}

char *fileSiteSchemeNew(FileSite *self, const char *path) {
	char *sysDir;

	self->fullUri = NULL;
	if (path) {
		if ((sysDir = platformPathFileSchemeToSystem((char * ) path))) {
			PlatformFileStat st;

			/* Check the path exists and is a directory */
			if (platformFileStat(sysDir, &st))
				return strerror(errno);

			if (!platformFileStatIsDirectory(&st))
				return strerror(ENOTDIR);
		} else
			return strerror(errno);
	} else {
		if (!(sysDir = malloc(FILENAME_MAX)))
			return strerror(errno);

		if (!platformGetWorkingDirectory(sysDir, FILENAME_MAX)) {
			free(sysDir);
			return "Unable to get systems working directory";
		}
	}

	UpdateFileUri(self, sysDir), free(sysDir);
	return NULL;
}

void fileSiteSchemeDirectoryListingClose(void *listing) {
	platformDirClose(listing);
}

char *fileSiteSchemeDirectoryListingEntryStat(void *listing, void *entry, SiteFileMeta *meta) {
	PlatformFileStat st;
	SiteFileMeta *e = entry;
	const char *p = platformDirPath(listing);
	char *entryPath;

	if (!p || !(entryPath = malloc(strlen(e->name) + strlen(p) + 2)))
		return 0;

	platformPathCombine(entryPath, p, e->name);
	if (platformFileStat(entryPath, &st)) {
		free(entryPath);
		return strerror(errno);
	}

	meta->path = entryPath, meta->name = platformPathLast(entryPath), meta->length = st.st_size;
	if ((meta->modifiedDate = malloc(sizeof(PlatformTimeStruct))))
		if (platformGetTimeStruct(&st.st_mtime, meta->modifiedDate))
			free(meta->modifiedDate), meta->modifiedDate = NULL;

	if (platformFileStatIsFile(&st))
		meta->type = SITE_FILE_TYPE_FILE;
	else if (platformFileStatIsDirectory(&st))
		meta->type = SITE_FILE_TYPE_DIRECTORY;
	else
		meta->type = SITE_FILE_TYPE_UNKNOWN;

	return NULL;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"
void *fileSiteSchemeDirectoryListingOpen(FileSite *self, char *path) {
	char *fullPath, *nativePath;

	if (!(fullPath = uriPathAbsoluteAppend(self->fullUri, path)))
		return NULL;

	nativePath = platformPathFileSchemeToSystem(fullPath), free(fullPath);
	if (nativePath) {
		PlatformFileStat stat;

		if (!platformFileStat(nativePath, &stat)) {
			if (platformFileStatIsDirectory(&stat)) {
				PlatformDir *d = platformDirOpen(nativePath);
				free(nativePath);
				return d;
			}
		}

		free(nativePath);
	}

	return NULL;
}
#pragma clang diagnostic pop

void *fileSiteSchemeDirectoryListingRead(void *listing) {
	const char *name;
	size_t nameLen;
	PlatformDirEntry *entry;
	SiteFileMeta *siteEntry;

fileSiteReadDirectoryListing_skip:
	if (!(entry = platformDirRead(listing)) || !(name = platformDirEntryGetName(entry, &nameLen)) || !nameLen)
		return NULL;

	if (name[0] == '.')
		goto fileSiteReadDirectoryListing_skip;

	if (!(siteEntry = malloc(sizeof(SiteFileMeta))))
		return NULL;

	if (!(siteEntry->name = malloc(nameLen + 1))) {
		free(siteEntry);
		return NULL;
	}

	strcpy(siteEntry->name, name), siteEntry->modifiedDate = NULL, siteEntry->path = NULL;
	siteEntry->type = platformDirEntryIsDirectory(entry, listing, NULL) ? SITE_FILE_TYPE_DIRECTORY : SITE_FILE_TYPE_FILE;
	return siteEntry;
}

void fileSiteSchemeFileClose(FileSite *self) {
	if (self->file)
		platformFileClose(self->file), self->file = NULL;

	if (self->meta.path)
		free(self->meta.path);

	if (self->meta.modifiedDate)
		free(self->meta.modifiedDate);

	memset(&self->meta, 0, sizeof(SiteFileMeta));
}

int fileSiteSchemeFileAtEnd(FileSite *self) {
	return platformFileAtEnd(self->file);
}

SOCK_BUF_TYPE fileSiteSchemeFileRead(FileSite *self, char *buffer, SOCK_BUF_TYPE size) {
	return platformFileRead(buffer, 1, size, self->file);
}

SiteFileMeta *fileSiteSchemeStatOpenMeta(FileSite *self, const char *path) {
	char *fullPath;
	SiteFileMeta *meta;

	if (!(fullPath = ResolveToSystemPath(self->fullUri, (char *) path)))
		return NULL;

	if (!(meta = calloc(1, sizeof(SiteFileMeta)))) {
		free(fullPath);

		return NULL;
	}

	FillFileMeta(fullPath, meta);
	free(fullPath);

	return meta;
}

const char *fileSiteSchemeFileOpenRead(FileSite *self, const char *path, PlatformFileOffset start, PlatformFileOffset end) {
	const char *e = FileOpen(self, path, "rb");

	if (!e && start != -1)
		platformFileSeek(self->file, start, SEEK_SET);

	return e;
}

const char *fileSiteSchemeFileOpenAppend(FileSite *self, const char *path, PlatformFileOffset start, PlatformFileOffset end) {
	const char *e;

	if (!(e = FileOpen(self, path, "ab")))
		if (platformFileSeek(self->file, start, SEEK_SET))
			e = strerror(errno);

	return e;
}

const char *fileSiteSchemeFileOpenWrite(FileSite *self, const char *path) {
	return FileOpen(self, path, "wb");
}

SOCK_BUF_TYPE fileSiteSchemeFileWrite(FileSite *self, char *buffer, SOCK_BUF_TYPE size) {
	return platformFileWrite(buffer, 1, size, self->file);
}
