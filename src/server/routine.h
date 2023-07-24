#ifndef OPEN_WEB_ROUTINE_H
#define OPEN_WEB_ROUTINE_H

#include <stdio.h>
#include "../platform/platform.h"
#include "sockbufr.h"

enum state {
    TYPE_FILE_ROUTINE = 1, TYPE_DIR_ROUTINE = 2, STATE_CONTINUE = 4, STATE_DEFER = 8, STATE_FINISH = 16, STATE_FAIL = 32
};

typedef struct FileRoutine {
    FILE *file;
    PlatformFileOffset start, end;
} FileRoutine;

typedef struct DirectoryRoutine {
    size_t count;
    DIR *directory;
    char *rootPath;
} DirectoryRoutine;

typedef struct Routine {
    char state;
    SocketBuffer socketBuffer;
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

size_t DirectoryRoutineContinue(Routine *self);

Routine DirectoryRoutineNew(SOCKET socket, DIR *dir, const char *webPath, char *rootPath);

char DirectoryRoutineArrayAdd(RoutineArray *self, Routine directoryRoutine);

void DirectoryRoutineFree(DirectoryRoutine *self);

char DirectoryRoutineArrayDel(RoutineArray *self, Routine *directoryRoutine);

Routine
FileRoutineNew(SOCKET socket, FILE *file, PlatformFileOffset start, PlatformFileOffset end, char webPath[FILENAME_MAX]);

size_t FileRoutineContinue(Routine *self);

void FileRoutineFree(FileRoutine *self);

char FileRoutineArrayAdd(RoutineArray *self, Routine fileRoutine);

char FileRoutineArrayDel(RoutineArray *self, Routine *fileRoutine);

#endif /* OPEN_WEB_ROUTINE_H */
