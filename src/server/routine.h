#ifndef OPEN_WEB_ROUTINE_H
#define OPEN_WEB_ROUTINE_H

#include <stdio.h>
#include "../platform/platform.h"
#include "sockbufr.h"

typedef struct FileRoutine {
    FILE *file;
    PlatformFileOffset start, end;
    SOCKET socket;
    char webPath[FILENAME_MAX];
} FileRoutine;

typedef struct DirectoryRoutine {
    size_t count;
    DIR *directory;
    char *rootPath;
    SocketBuffer socketBuffer;
    char webPath[FILENAME_MAX];
} DirectoryRoutine;

typedef struct RoutineArray {
    size_t size;
    char *array;
} RoutineArray;

size_t DirectoryRoutineContinue(DirectoryRoutine *self);

DirectoryRoutine DirectoryRoutineNew(SOCKET socket, DIR *dir, const char *webPath, char *rootPath);

char DirectoryRoutineArrayAdd(RoutineArray *self, DirectoryRoutine directoryRoutine);

void DirectoryRoutineFree(DirectoryRoutine *self);

char DirectoryRoutineArrayDel(RoutineArray *self, DirectoryRoutine *directoryRoutine);

FileRoutine
FileRoutineNew(SOCKET socket, FILE *file, PlatformFileOffset start, PlatformFileOffset end, char webPath[FILENAME_MAX]);

size_t FileRoutineContinue(FileRoutine *self);

void FileRoutineFree(FileRoutine *self);

char FileRoutineArrayAdd(RoutineArray *self, FileRoutine fileRoutine);

char FileRoutineArrayDel(RoutineArray *self, FileRoutine *fileRoutine);

#endif /* OPEN_WEB_ROUTINE_H */
