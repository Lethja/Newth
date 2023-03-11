#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include "routine.h"

FileRoutine FileRoutineNew(int socket, FILE *file, off_t start, off_t end) {
    FileRoutine self;
    self.file = file, self.start = start, self.end = end, self.socket = socket;
    fseek(self.file, self.start, SEEK_SET);
    return self;
}

size_t FileRoutineContinue(FileRoutine *self) {
    size_t bytesRead, byteWrite;
    char buffer[BUFSIZ];
    off_t currentPosition = ftell(self->file);
    long remaining = self->end - currentPosition;

    bytesRead = fread(buffer, 1, remaining < BUFSIZ ? remaining : BUFSIZ, self->file);

    if (bytesRead > 0) {
        byteWrite = write(self->socket, buffer, bytesRead);

        if (byteWrite == -1)
            bytesRead = 0;
    }

    return bytesRead;
}

void FileRoutineFree(FileRoutine *self) {
    fclose(self->file);
}

void FileRoutineArrayAdd(FileRoutineArray *self, FileRoutine fileRoutine) {
    self->size++;
    if (self->size == 1)
        self->array = malloc(sizeof(FileRoutine));
    else
        self->array = realloc(self->array, sizeof(FileRoutine) * self->size);

    memcpy(&self->array[self->size - 1], &fileRoutine, sizeof(FileRoutine));
}

void FileRoutineArrayDel(FileRoutineArray *self, FileRoutine *fileRoutine) {
    size_t i;
    int r;

    for (i = 0; i < self->size; i++) {
        r = memcmp((void *) &self->array[i], (void *) fileRoutine, sizeof(FileRoutine));

        if (r)
            continue;

        FileRoutineFree(&self->array[i]);
        if (i + 1 < self->size)
            memmove(&self->array[i], &self->array[i + 1], sizeof(FileRoutine) * (self->size - i));

        self->size--;
        self->array = realloc(self->array, sizeof(FileRoutine) * self->size);
        return;
    }
}