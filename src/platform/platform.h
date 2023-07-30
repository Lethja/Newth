#ifndef OPEN_WEB_PLATFORM_H
#define OPEN_WEB_PLATFORM_H

#ifdef NDEBUG
#define LINEDBG
#else
#define LINEDBG printf("%s:%d\n", __FILE__, __LINE__)
#endif

#define SA struct sockaddr

#ifndef __STDC_VERSION__
#define inline
#endif

#ifdef _WIN32

/* Disable unknown pragma warning */
#pragma warning(disable:4068)

#include "mscrtdl.h"

#elif MOCK

#include "mockput.h"

#else

#include "posix01.h"

#endif

#ifndef SB_DATA_SIZE
#define SB_DATA_SIZE BUFSIZ
#endif

#define ADAPTER_NAME_LENGTH 512

typedef struct Address {
    sa_family_t type;
    char address[INET6_ADDRSTRLEN];
} Address;

typedef struct AddressArray {
    size_t size;
    Address *array;
} AddressArray;

typedef struct Adapter {
    char name[ADAPTER_NAME_LENGTH];
    AddressArray addresses;
} NetworkAdapter;

typedef struct AdapterAddressArray {
    size_t size;
    NetworkAdapter *adapter;
} AdapterAddressArray;

/**
 * Get the argv position of the flag parameter (and optionally the argument)
 * @param argc In: argc from main()
 * @param argv In: argv from main()
 * @param shortFlag Optional In: The short flag to look for, pass '\0' to omit
 * @param longFlag Optional In: The long flag to look for, pass NULL to omit
 * @param optArg Optional Out: The argument of the flag or NULL if there wasn't one, pass NULL to omit
 * @return The position of the flag in argv or 0 if the flag couldn't be found
 */
int platformArgvGetFlag(int argc, char **argv, char shortFlag, char *longFlag, char **optArg);

/**
 * Bind one of any ports mentioned in a string
 * @param listenSocket In: The socket to attempt to bind a listen port to
 * @param sockAddr In: The socket address structure get use as binding information
 * @param socketSize In: The size of sockAddr
 * @param port In: A string with all the potential ports in it
 * @return 0 on successful bind, otherwise error
 */
char platformBindPort(const SOCKET *listenSocket, SA *sockAddr, char *port);

/**
 * Wrapper around reallocation that gracefully hands control back to the caller in the event of a failure
 * @param heap In-out: pointer to the allocation pointer
 * @param elementSize In: size of the elements being allocated
 * @param elementNumber In: number of elements to allocate
 * @return 0 on success other on allocation error
 * @note in the event of an allocation error the existing allocation will remain unchanged
 */
char platformHeapResize(void **heap, size_t elementSize, size_t elementNumber);

/**
 * Combine two path strings together
 * @param output Out: A string buffer with at least the combine amount of space for path1 and path 2
 * @param path1 In: The string to combine on the left
 * @param path2 In: The string to combine on the right
 */
void platformPathCombine(char *output, const char *path1, const char *path2);

/**
 * Get the absolute path from a relative path
 * @param path In: the relative path to convert
 * @return The absolute path
 */
char *platformRealPath(char *path);

/**
 * Close any open sockets in the file descriptor
 * @param sockets In: the fd_set to close all open sockets on
 * @param max In: the maximum socket number select() has seen in the session
 */
void platformCloseBindSockets(fd_set *sockets, SOCKET max);

/**
 * Free any memory resources that platformServerProgramInit() allocated
 */
void platformIpStackExit(void);

/**
 * Allocate any memory resources that this platform may need to run network services
 * @return NULL on success, human readable string to print error
 */
char *platformIpStackInit(void);

/**
 * Start up the server
 * @param listenSocket Out: the socket the server has been bound to
 * @param family In: protocol to start the server under
 * @param ports In: A port of comma separated ports to try to bind to from left to right
 * @return NULL on success, human readable string to print error
 */
char *platformServerStartup(SOCKET *listenSocket, sa_family_t family, char *ports);

/**
 * Attach signals to comment interrupts
 * @param noAction In: The function to callback on pipe errors
 * @param shutdownCrash In: The function to callback on crashes
 * @param shutdownProgram In: The function to callback on graceful exiting signals
 */
void platformConnectSignals(void(*noAction)(int), void(*shutdownCrash)(int), void(*shutdownProgram)(int));

/**
 * Get the ipv4 or ipv6 address of a TCP socket as a human readable string
 * without having to worry about the systems underlying capabilities
 * @param addr In: the socket to get the address of
 * @param ipStr Out: the char array to use
 * @param family Out: Family type of returned string (ipv4 or ipv6)
 */
void platformGetIpString(struct sockaddr *addr, char ipStr[INET6_ADDRSTRLEN], sa_family_t *family);

/**
 * Get the port number of a TCP socket as a unsigned 16 bit integer
 * without having to worry about the systems underlying capabilities
 * @param addr In: the socket to get the port of
 * @return The port number of this socket
 */
unsigned short platformGetPort(struct sockaddr *addr);

/**
 * Build an adapter address array structure containing all the network adapters and addresses of the system
 * @param family In: IP address versions to include choose between AF_INET, AF_INET6 or AF_UNSPEC for both
 * @return The adapter address array for your application to parse through
 * @remark Return result must be freed with platformFreeAdapterInformation()
 */
AdapterAddressArray *platformGetAdapterInformation(sa_family_t family);

/**
 * Free an adapter address array created with platformGetAdapterInformation()
 * @param array In: The adapter address array to be freed
 */
void platformFreeAdapterInformation(AdapterAddressArray *array);

/**
 * Add a entry to the adapter address array
 * @param array In: The array to add the new entry to
 * @param adapter In: The string name that represents this adapter (differs depending on underlying system)
 * @param type In: Family type of the ip string, choose between AF_INET or AF_INET6
 * @param ip In: A human readable representation of the ip address (and only the ip address)
 */
void platformFindOrCreateAdapterIp(AdapterAddressArray *array, char adapter[ADAPTER_NAME_LENGTH], sa_family_t type,
                                   char ip[INET6_ADDRSTRLEN]);

/**
 * Check if the platform is know to have a stable IPv6 stack
 * @return return non-zero if IPv6 is supported or zero if you should only run IPv4 era functions in your application
 */
int platformOfficiallySupportsIpv6(void);

/**
 * Get a HTTP formatted string of the time
 * @param clock In: The time to be converted into a string
 * @param timeStr Out: The HTTP formatted representation of the time
 */
void platformGetTime(void *clock, char *timeStr);

/**
 * Get a HTTP formatted string of the current system time
 * @param timeStr Out: The HTTP formatted representation of the time
 */
void platformGetCurrentTime(char *timeStr);

/**
 * Get a platform neutral implementation of time
 * @param clock In: The time to be converted
 * @param timeStructure Out: The time structure
 * @return zero on success, other on error
 */
char platformGetTimeStruct(void *clock, PlatformTimeStruct *timeStructure);

/**
 * Convert HTTP date-time string to a platform neutral implementation time
 * @param str In: The HTTP date-time string to be converted
 * @param time Out: The time structure
 * @return zero on success, other on error
 */
char platformTimeGetFromHttpStr(const char *str, PlatformTimeStruct *time);

/**
 * Compare two time structures
 * @param t1 In: The first time structure to compare
 * @param t2 In: The second time structure
 * @return zero when time structures have different times, other when identical
 */
int platformTimeStructEquals(PlatformTimeStruct *t1, PlatformTimeStruct *t2);

/**
 * Open a directory path in a platform portable way
 * @param path In: The directory to be opened
 * @return NULL on error, else a directory struct pointer that can be used on other platformDir functions
 * @remark Returned result must be freed with platformDirClose()
 */
void *platformDirOpen(char *path);

/**
 * Free the pointer created by platformDirOpen()
 * @param dirp In: A pointer created from platformDirOpen() to free
 */
void platformDirClose(void *dirp);

/**
 * Get the First/Next entry
 * @param dirp In: A pointer created from platformDirOpen()
 * @return A PlatformDirEntry pointer that can be used with platformDirEntry functions
 * @remark Do not free or write to the returning pointer struct
 */
void *platformDirRead(void *dirp);

/**
 * Get the name of the file/folder in the entry
 * @param entry In: The entry to get the filename from
 * @param length Out: The length of the string being returned
 * @return A string containing the name of the file or folder
 * @remark Do not free returned value, copy value if required beyond entry scope
 */
char *platformDirEntryGetName(PlatformDirEntry *entry, size_t *length);

/**
 * Check if entry is hidden file/folder from the platforms point of view
 * @param entry In: The entry to check if the file/folder is hidden from
 * @return zero if visible, other on hidden
 */
char platformDirEntryIsHidden(PlatformDirEntry *entry);

/**
 * Check if entry is a Directory (aka Folder)
 * @param rootPath In: The root directory
 * @param webPath In: The web relative directory
 * @param entry In: The entry to check if the file/folder is a folder
 * @return Zero if not a directory/folder, other if it is a directory/folder
 */
char platformDirEntryIsDirectory(char *rootPath, char *webPath, PlatformDirEntry *entry);

/**
 * Get the status structure of a file
 * @param path In: string with the path to the file
 * @param fileStat Out: the stat file structure to populate
 * @return zero on success, other on error
 */
int platformFileStat(const char *path, PlatformFileStat *fileStat);

/**
 * Accept a connection from a listen socket
 * @param fromSocket In: the listen socket to accept the new socket from
 * @return A new established socket from the client
 */
SOCKET platformAcceptConnection(SOCKET fromSocket);

/**
 * Get if the file/folder in this stat structure is a directory
 * @param stat In: the stat file structure
 * @return zero if not a directory, other if it is a directory
 */
char platformFileStatIsDirectory(PlatformFileStat *stat);

/**
 * Get if the file/folder in this stat structure is a regular file
 * @param stat In: the stat file structure
 * @return zero if not a regular file, other if it is a regular file
 */
char platformFileStatIsFile(PlatformFileStat *stat);

/**
 * Get the absolute path of a directory
 * @param path In: The path to expand into a full path
 * @return The full path or NULL or error
 */
char *platformGetRootPath(char *path);

/**
 * Get the working directory of the process
 * @param buffer In-out: The buffer to store the working directory into
 * @param length In: The maximum length of buffer
 * @return a pointer to buffer on success, NULL on failure
 */
char *platformGetWorkingDirectory(char *buffer, size_t length);

/**
 * Convert a path from a HTTP request into a system path for opening the file.
 * @param rootPath In: The root path of the server
 * @param webPath In: The HTTP request path
 * @param absolutePath Out: The absolute system file path representation
 * @return 0 on success, appropriate HTTP response code on failure
 * @remark If the path would be longer then FILENAME_MAX this function will return response code 500
 */
short platformPathWebToSystem(const char *rootPath, char *webPath, char *absolutePath);

/**
 * Convert a path from a system path to a HTTP path.
 * @param rootPath In: The root path of the server
 * @param absolutePath In: The absolute system file path
 * @param webPath Out: The HTTP request path representation
 * @return 0 on success, appropriate HTTP response code on failure
 */
/*
short platformPathSystemToWeb(const char *rootPath, char *absolutePath, char *webPath);
*/

/**
 * Open a file with the largest bit offset the platform supports
 * @param fileName In: Path to file
 * @param fileMode In: Modes to use on the file
 * @return The platforms native open file type
 * @remark Return value must be run through platformFileClose() before freeing memory or leaving scope
 */
PlatformFile platformFileOpen(const char *fileName, const char *fileMode);

/**
 * Close a file opened with platformFileOpen
 * @param stream In: The file stream to be closed
 * @return 0 on success, other on error
 */
int platformFileClose(PlatformFile stream);

/**
 * Seek through a file stream created with platformFileOpen to a certain position
 * @param stream In: The file stream to seek on
 * @param offset In: The amount of bytes to seek, use a negative number to rewind
 * @param whence In: Where to seek from, start, end or current position
 * @return 0 on success, other on error
 */
int platformFileSeek(PlatformFile stream, PlatformFileOffset offset, int whence);

/**
 * Get the current seek position of a stream created with platformFileOpen
 * @param stream In: The stream to get the position of
 * @return The position of the stream on success, -1 on error
 */
PlatformFileOffset platformFileTell(PlatformFile stream);

/**
 * Read the stream created with platformFileOpen into a memory buffer
 * @param buffer Out: The memory buffer to write to
 * @param size In: the number of elements to read
 * @param n In: the size of each element to be read
 * @param stream In: the stream to read from
 * @return The number of elements read or 0 when no elements have been read
 */
size_t platformFileRead(void *buffer, size_t size, size_t n, PlatformFile stream);

/**
 * Open a temporary file stream
 * @return The platforms native memory stream
 * @remark Return value must be run through platformMemoryStreamFree() before freeing memory or leaving scope
 */
FILE *platformMemoryStreamNew(void);

/**
 * Free a memory stream
 * @param stream The memory stream to free
 */
void platformMemoryStreamFree(FILE *stream);

/**
 * Seek through a memory stream created with platformMemoryStreamNew to a certain position
 * @param stream In: The memory stream to seek on
 * @param offset In: The amount of bytes to seek, use a negative number to rewind
 * @param origin In: Where to seek from, start, end or current position
 * @return
 */
int platformMemoryStreamSeek(FILE *stream, long offset, int origin);

/**
 * Get the current seek position of a stream created with platformMemoryStreamNew
 * @param stream In: The stream to get the position of
 * @return The position of the stream on success, -1 on error
 */
long platformMemoryStreamTell(FILE *stream);

/**
 * Set the socket to be blocking or non-blocking in a platform agnostic way
 * @param socket In: The socket to set blocking to
 * @param blocking In: Should the socket block? 0 for non-blocking other for blocking
 * @return -1 on error, other on success. Call platformSocketGetLastError for more information
 */
int platformSocketSetBlock(SOCKET socket, char blocking);

/**
 * Get the last socket error in a platform agnostic way
 * @return The error the socket returned
 */
int platformSocketGetLastError(void);

#endif /* OPEN_WEB_PLATFORM_H */
