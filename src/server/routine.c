#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include "routine.h"
#include "http.h"

size_t DirectoryRoutineContinue(DirectoryRoutine *self) {
    const size_t max = 10;
    size_t bytesWrite = 0, i, pathLen = strlen(self->webPath);
    struct dirent *entry;

    for (i = 0; i < max; ++i) {
        size_t entryLen;
        entry = readdir(self->directory);
        if (entry) {
            char buffer[BUFSIZ];
            char pathBuf[BUFSIZ];

            if (entry->d_name[0] == '.')
                continue;

            entryLen = strlen(entry->d_name);

            if (pathLen + entryLen + 2 < BUFSIZ) {
                memcpy(pathBuf, self->webPath, pathLen);
                pathBuf[pathLen ? pathLen : 0] = '/';
                memcpy(pathLen ? pathBuf + pathLen + 1 : pathBuf + 1, entry->d_name, entryLen + 1);

                htmlListWritePathLink(buffer, pathBuf);
                if (httpBodyWriteChunk(self->socket, buffer))
                    return 0;

                bytesWrite += strlen(buffer);
            }
        } else {
            char buffer[BUFSIZ];
            htmlListEnd(buffer);
            htmlFooterWrite(buffer);
            if (!httpBodyWriteChunk(self->socket, buffer))
                httpBodyWriteChunkEnding(self->socket);

            return 0;
        }
    }

    return bytesWrite;
}

DirectoryRoutine DirectoryRoutineNew(int socket, DIR *dir, const char *webPath) {
    size_t i;
    DirectoryRoutine self;
    self.directory = dir, self.socket = socket;

    for (i = 0; i < PATH_MAX; ++i) {
        self.webPath[i] = webPath[i];
        if (webPath[i] == '\0')
            break;
    }

    return self;
}

void DirectoryRoutineArrayAdd(RoutineArray *self, DirectoryRoutine directoryRoutine) {
    DirectoryRoutine *array;
    self->size++;
    if (self->size == 1)
        self->array = malloc(sizeof(DirectoryRoutine));
    else
        self->array = realloc(self->array, sizeof(DirectoryRoutine) * self->size);

    array = (DirectoryRoutine *) self->array;

    memcpy(&array[self->size - 1], &directoryRoutine, sizeof(DirectoryRoutine));
}

void DirectoryRoutineFree(DirectoryRoutine *self) {
    closedir(self->directory);
}

void DirectoryRoutineArrayDel(RoutineArray *self, DirectoryRoutine *directoryRoutine) {
    size_t i;
    DirectoryRoutine *array = (DirectoryRoutine *) self->array;

    for (i = 0; i < self->size; i++) {
        if (&array[i] != directoryRoutine)
            continue;

        DirectoryRoutineFree(&array[i]);

        if (i + 1 < self->size)
            memmove(&array[i], &array[i + 1], sizeof(DirectoryRoutine) * (self->size - i));

        self->size--;

        if (self->size)
            self->array = realloc(self->array, sizeof(DirectoryRoutine) * self->size);
        else
            free(self->array);

        return;
    }
}

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

void FileRoutineArrayAdd(RoutineArray *self, FileRoutine fileRoutine) {
    FileRoutine *array;
    self->size++;
    if (self->size == 1)
        self->array = malloc(sizeof(FileRoutine));
    else
        self->array = realloc(self->array, sizeof(FileRoutine) * self->size);

    array = (FileRoutine *) self->array;

    memcpy(&array[self->size - 1], &fileRoutine, sizeof(FileRoutine));
}

void FileRoutineArrayDel(RoutineArray *self, FileRoutine *fileRoutine) {
    size_t i;
    FileRoutine *array = (FileRoutine *) self->array;

    for (i = 0; i < self->size; i++) {
        if (&array[i] != fileRoutine)
            continue;

        FileRoutineFree(&array[i]);

        if (i + 1 < self->size)
            memmove(&array[i], &array[i + 1], sizeof(FileRoutine) * (self->size - i));

        self->size--;

        if (self->size)
            self->array = realloc(self->array, sizeof(FileRoutine) * self->size);
        else
            free(self->array);

        return;
    }
}