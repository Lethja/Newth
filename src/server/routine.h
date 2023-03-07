#ifndef OPEN_WEB_ROUTINE_H
#define OPEN_WEB_ROUTINE_H

#include <stdio.h>

typedef struct FileRoutine {
    FILE *file;
    off_t start, end;
    int socket;
} FileRoutine;

typedef struct FileRoutineArray {
    size_t size;
    FileRoutine *array;
} FileRoutineArray;

FileRoutine FileRoutineNew(int socket, FILE *file, off_t start, off_t end);

size_t FileRoutineContinue(FileRoutine *self);

void FileRoutineFree(FileRoutine *self);

void FileRoutineArrayAdd(FileRoutineArray *self, FileRoutine fileRoutine);

void FileRoutineArrayDel(FileRoutineArray *self, FileRoutine *fileRoutine);

#endif /* OPEN_WEB_ROUTINE_H */
