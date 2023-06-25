#include <malloc.h>
#include <string.h>
#include "routine.h"
#include "http.h"
#include "event.h"

size_t DirectoryRoutineContinue(DirectoryRoutine *self) {
    const size_t max = 10;
    size_t bytesWrite = 0, i, pathLen = strlen(self->webPath);
    PlatformDirEntry *entry;

    for (i = 0; i < max; ++i) {
        entry = platformDirRead(self->directory);
        if (entry) {
            size_t entryLen;
            char buffer[BUFSIZ], pathBuf[BUFSIZ], *entryName;

            if (platformDirEntryIsHidden(entry)) {
                --i;
                continue;
            }

            entryName = platformDirEntryGetName(entry, &entryLen);

            if (pathLen + entryLen + 3 < BUFSIZ) {
                memcpy(pathBuf, self->webPath, pathLen);
                if (pathBuf[pathLen ? pathLen - 1 : 0] != '/') {
                    pathBuf[pathLen ? pathLen : 0] = '/';
                    memcpy(pathLen ? pathBuf + pathLen + 1 : pathBuf + 1, entryName, entryLen + 1);
                } else
                    memcpy(pathBuf + pathLen, entryName, entryLen + 1);

                /* Append '/' on the end of directory entries */
                if (platformDirEntryIsDirectory(self->rootPath, self->webPath, entry)) {
                    size_t len = strlen(pathBuf);
                    pathBuf[len] = '/', pathBuf[len + 1] = '\0';
                }

                htmlListWritePathLink(buffer, pathBuf);
                if (httpBodyWriteChunk(&self->socketBuffer, buffer))
                    return 0;

                bytesWrite += strlen(buffer);
                ++self->count;
            }
        } else {
            char buffer[BUFSIZ];
            buffer[0] = '\0';

            /* If the directory is empty, say so */
            if (!self->count)
                strcpy(buffer, "\t\t\t<LI>Empty Directory</LI>\n");

            htmlListEnd(buffer);
            htmlFooterWrite(buffer);
            if (!httpBodyWriteChunk(&self->socketBuffer, buffer))
                httpBodyWriteChunkEnding(&self->socketBuffer);

            socketBufferFlush(&self->socketBuffer);

            eventHttpFinishInvoke(&self->socketBuffer.clientSocket, self->webPath, httpGet, 0);

            return 0;
        }
    }

    socketBufferFlush(&self->socketBuffer);

    return bytesWrite;
}

DirectoryRoutine DirectoryRoutineNew(SOCKET socket, DIR *dir, const char *webPath, char *rootPath) {
    size_t i;
    DirectoryRoutine self;
    self.directory = dir, self.socketBuffer = socketBufferNew(socket), self.count = 0, self.rootPath = rootPath;

    for (i = 0; i < FILENAME_MAX; ++i) {
        self.webPath[i] = webPath[i];
        if (webPath[i] == '\0')
            break;
    }

    return self;
}

char DirectoryRoutineArrayAdd(RoutineArray *self, DirectoryRoutine directoryRoutine) {
    DirectoryRoutine *array;
    self->size++;
    if (self->size == 1)
        self->array = malloc(sizeof(DirectoryRoutine));
    else if (platformHeapResize((void **) &self->array, sizeof(DirectoryRoutine), self->size))
        return 1;

    array = (DirectoryRoutine *) self->array;
    memcpy(&array[self->size - 1], &directoryRoutine, sizeof(DirectoryRoutine));

    return 0;
}

void DirectoryRoutineFree(DirectoryRoutine *self) {
    platformDirClose(self->directory);
}

char DirectoryRoutineArrayDel(RoutineArray *self, DirectoryRoutine *directoryRoutine) {
    size_t i;
    DirectoryRoutine *array = (DirectoryRoutine *) self->array;

    for (i = 0; i < self->size; i++) {
        if (&array[i] != directoryRoutine)
            continue;

        DirectoryRoutineFree(&array[i]);

        if (i + 1 < self->size)
            memmove(&array[i], &array[i + 1], sizeof(DirectoryRoutine) * (self->size - i));

        --self->size;

        if (self->size)
            platformHeapResize((void **) &self->array, sizeof(DirectoryRoutine), self->size);
        else
            free(self->array);

    }
    return 0;
}

FileRoutine FileRoutineNew(SOCKET socket, FILE *file, PlatformFileOffset start, PlatformFileOffset end,
                           char webPath[FILENAME_MAX]) {
    FileRoutine self;
    self.file = file, self.start = start, self.end = end, self.socket = socket;
    strncpy(self.webPath, webPath, FILENAME_MAX - 1);
    FSEEK_64(self.file, self.start, SEEK_SET);
    return self;
}

size_t FileRoutineContinue(FileRoutine *self) {
    size_t bytesRead, byteWrite;
    char buffer[BUFSIZ];
    PlatformFileOffset currentPosition = FTELL_64(self->file);
    PlatformFileOffset remaining = self->end - currentPosition;

    bytesRead = fread(buffer, 1, remaining < BUFSIZ ? remaining : BUFSIZ, self->file);

    if (bytesRead > 0) {
        byteWrite = send(self->socket, buffer, (SOCK_BUF_TYPE) bytesRead, 0);

        if (byteWrite == -1)
            bytesRead = 0;
    } else {
        eventHttpFinishInvoke(&self->socket, self->webPath, httpGet, 0);
    }

    return bytesRead;
}

void FileRoutineFree(FileRoutine *self) {
    fclose(self->file);
}

char FileRoutineArrayAdd(RoutineArray *self, FileRoutine fileRoutine) {
    FileRoutine *array;
    self->size++;
    if (self->size == 1)
        self->array = malloc(sizeof(FileRoutine));
    else if (platformHeapResize((void **) &self->array, sizeof(FileRoutine), self->size))
        return 1;

    array = (FileRoutine *) self->array;
    memcpy(&array[self->size - 1], &fileRoutine, sizeof(FileRoutine));

    return 0;
}

char FileRoutineArrayDel(RoutineArray *self, FileRoutine *fileRoutine) {
    size_t i;
    FileRoutine *array = (FileRoutine *) self->array;

    for (i = 0; i < self->size; i++) {
        if (&array[i] != fileRoutine)
            continue;

        FileRoutineFree(&array[i]);

        if (i + 1 < self->size)
            memmove(&array[i], &array[i + 1], sizeof(FileRoutine) * (self->size - i));

        --self->size;

        if (self->size)
            platformHeapResize((void **) &self->array, sizeof(FileRoutine), self->size);
        else
            free(self->array);
    }
    return 0;
}
