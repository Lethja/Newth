#ifndef OPEN_WEB_ROUTINE_H
#define OPEN_WEB_ROUTINE_H

#include <stdio.h>
#include "../platform/platform.h"
#include "sendbufr.h"

enum state {
	TYPE_ROUTINE = 1,
	TYPE_ROUTINE_DIR = 2,
	TYPE_ROUTINE_FILE = 4,
	STATE_CONTINUE = 8,
	STATE_DEFER = 16,
	STATE_FLUSH = 32,
	STATE_FINISH = 64,
	STATE_FAIL = 128
};

typedef struct FileRoutine {
	FILE *file;
	PlatformFileOffset start, end;
} FileRoutine;

typedef struct DirectoryRoutine {
	size_t count;
	PlatformDir *directory;
} DirectoryRoutine;

typedef struct Routine {
	unsigned char state;
	SendBuffer socketBuffer;
	char webPath[FILENAME_MAX];
	union {
		struct FileRoutine file;
		struct DirectoryRoutine dir;
	} type;
} Routine;

typedef struct RoutineArray {
	size_t size;
	Routine *array;
} RoutineArray;

/**
 * Continue this directory routines execution
 * @param self In: The DirectoryRoutine to continue
 * @return The amount of bytes written to a socket. Once 0 bytes are written the routine has either completed or failed
 */
size_t DirectoryRoutineContinue(Routine *self);

/**
 * Create a new DirectoryRoutine
 * @param socketBuffer In: The SocketBuffer this routine should write to
 * @param dir In: A newly opened PlatformDir
 * @param webPath In: The full web path to concatenate links on
 * @return A new Routine to be used with DirectoryRoutineContinue()
 */
Routine DirectoryRoutineNew(SendBuffer socketBuffer, PlatformDir *dir, const char *webPath);

/**
 * Free a routines allocated memory
 * @param self The routine to free
 */
void DirectoryRoutineFree(DirectoryRoutine *self);

/**
 * Create a new FileRoutine
 * @param socketBuffer In: The SocketBuffer this routine should write to
 * @param file In: The file to write into the socket buffer
 * @param start In: Where in the file pointer to start sending (use 0 for the start of the file)
 * @param end In: Where in the file pointer to stop sending (use file length in bytes for end of the file)
 * @param webPath In: The full web path to display
 * @return A new Routine to be used with FileRoutineContinue()
 */
Routine FileRoutineNew(SendBuffer socketBuffer, FILE *file, PlatformFileOffset start, PlatformFileOffset end,
                       char webPath[FILENAME_MAX]);

/**
 * Continue this file routines execution
 * @param self In: The FileRoutine to continue
 * @return The amount of bytes written to a socket. Once 0 bytes are written the routine has either completed or failed
 */
size_t FileRoutineContinue(Routine *self);

/**
 * Free a routines allocated memory
 * @param self In: The routine to free
 */
void FileRoutineFree(FileRoutine *self);

/**
 * Add a routine to the array
 * @param self In: The routine array to add to
 * @param routine In: The routine to add to the array
 * @return zero on success, otherwise error
 */
char RoutineArrayAdd(RoutineArray *self, Routine routine);

/**
 * Remove a routine from the array
 * @param self In: The routine array to remove from
 * @param routine In: The routine to remove
 * @return zero on success, otherwise error
 */
char RoutineArrayDel(RoutineArray *self, Routine *routine);

/**
 * Remove a routine from the array by it's socket id
 * @param self In: The routine array to remove from
 * @param socket In: The routines socket to remove
 * @return zero on success, otherwise error
 */
char RoutineArrayDelSocket(RoutineArray *self, SOCKET socket);

/**
 * Create a new Routine
 * @param socketBuffer In: The socket buffer for this routine
 * @param webPath In: The web path associated with this routine
 * @return A new routine
 */
Routine RoutineNew(SendBuffer socketBuffer, const char *webPath);

/**
 * A state machine that iterates and advances all routines inside it
 * @param routineArray The routine array to tick forward
 */
void RoutineTick(RoutineArray *routineArray);

#endif /* OPEN_WEB_ROUTINE_H */
