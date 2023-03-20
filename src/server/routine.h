#ifndef OPEN_WEB_ROUTINE_H
#define OPEN_WEB_ROUTINE_H

#include <stdio.h>
#include <dirent.h>
#include "sockbufr.h"

typedef struct FileRoutine {
    FILE *file;
    off_t start, end;
    int socket;
} FileRoutine;

typedef struct DirectoryRoutine {
    size_t count;
    DIR *directory;
    SocketBuffer socketBuffer;
    char webPath[PATH_MAX];
} DirectoryRoutine;

typedef struct RoutineArray {
    size_t size;
    char *array;
} RoutineArray;

size_t DirectoryRoutineContinue(DirectoryRoutine *self);

DirectoryRoutine DirectoryRoutineNew(int socket, DIR *dir, const char *webPath);

void DirectoryRoutineArrayAdd(RoutineArray *self, DirectoryRoutine directoryRoutine);

void DirectoryRoutineFree(DirectoryRoutine *self);

void DirectoryRoutineArrayDel(RoutineArray *self, DirectoryRoutine *directoryRoutine);

FileRoutine FileRoutineNew(int socket, FILE *file, off_t start, off_t end);

size_t FileRoutineContinue(FileRoutine *self);

void FileRoutineFree(FileRoutine *self);

void FileRoutineArrayAdd(RoutineArray *self, FileRoutine fileRoutine);

void FileRoutineArrayDel(RoutineArray *self, FileRoutine *fileRoutine);

#endif /* OPEN_WEB_ROUTINE_H */
