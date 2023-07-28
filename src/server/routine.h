#ifndef OPEN_WEB_ROUTINE_H
#define OPEN_WEB_ROUTINE_H

#include <stdio.h>
#include "../platform/platform.h"
#include "sockbufr.h"

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
    DIR *directory;
    char *rootPath;
} DirectoryRoutine;

typedef struct Routine {
    unsigned char state;
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

Routine DirectoryRoutineNew(SocketBuffer socket, DIR *dir, const char *webPath, char *rootPath);

void DirectoryRoutineFree(DirectoryRoutine *self);

Routine FileRoutineNew(SocketBuffer socketBuffer, FILE *file, PlatformFileOffset start, PlatformFileOffset end,
                       char webPath[FILENAME_MAX]);

size_t FileRoutineContinue(Routine *self);

void FileRoutineFree(FileRoutine *self);

char RoutineArrayAdd(RoutineArray *self, Routine routine);

char RoutineArrayDel(RoutineArray *self, Routine *fileRoutine);

Routine RoutineNew(SocketBuffer socketBuffer, const char *webPath);

#endif /* OPEN_WEB_ROUTINE_H */
