#include <malloc.h>
#include <string.h>
#include "routine.h"
#include "http.h"
#include "event.h"

size_t DirectoryRoutineContinue(Routine *self) {
    const size_t max = 10;
    size_t bytesWrite = 0, i, pathLen = strlen(self->webPath);
    PlatformDirEntry *entry;

    for (i = 0; i < max; ++i) {
        entry = platformDirRead(self->type.dir.directory);
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
                if (platformDirEntryIsDirectory(self->type.dir.rootPath, self->webPath, entry)) {
                    size_t len = strlen(pathBuf);
                    pathBuf[len] = '/', pathBuf[len + 1] = '\0';
                }

                htmlListWritePathLink(buffer, pathBuf);
                if (httpBodyWriteChunk(&self->socketBuffer, buffer) == 0)
                    return 0;

                bytesWrite += strlen(buffer);
                ++self->type.dir.count;
            }
        } else {
            char buffer[BUFSIZ];
            buffer[0] = '\0';

            /* If the directory is empty, say so */
            if (!self->type.dir.count)
                strcpy(buffer, "\t\t\t<LI>Empty Directory</LI>\n");

            htmlListEnd(buffer);
            htmlFooterWrite(buffer);
            if (httpBodyWriteChunk(&self->socketBuffer, buffer))
                httpBodyWriteChunkEnding(&self->socketBuffer);

            socketBufferFlush(&self->socketBuffer);

            eventHttpFinishInvoke(&self->socketBuffer.clientSocket, self->webPath, httpGet, 0);

            return 0;
        }
    }

    socketBufferFlush(&self->socketBuffer);

    return bytesWrite;
}

Routine DirectoryRoutineNew(SOCKET socket, DIR *dir, const char *webPath, char *rootPath) {
    size_t i;

    Routine self;
    self.type.dir.directory = dir, self.type.dir.count = 0, self.type.dir.rootPath = rootPath, self.state =
            TYPE_DIR_ROUTINE | STATE_CONTINUE;
    self.socketBuffer = socketBufferNew(socket, SOC_BUF_OPT_EXTEND | SOC_BUF_OPT_HTTP_CHUNK);

    for (i = 0; i < FILENAME_MAX; ++i) {
        self.webPath[i] = webPath[i];
        if (webPath[i] == '\0')
            break;
    }

    return self;
}

char DirectoryRoutineArrayAdd(RoutineArray *self, Routine directoryRoutine) {
    Routine *array;

    ++self->size;
    if (self->size == 1)
        self->array = malloc(sizeof(Routine));
    else if (platformHeapResize((void **) &self->array, sizeof(Routine), self->size))
        return 1;

    /* TODO: if(!self->array) */

    array = (Routine *) self->array;
    memcpy(&array[self->size - 1], &directoryRoutine, sizeof(Routine));

    return 0;
}

void DirectoryRoutineFree(DirectoryRoutine *self) {
    platformDirClose(self->directory);
}

char DirectoryRoutineArrayDel(RoutineArray *self, Routine *directoryRoutine) {
    size_t i;
    Routine *array = (Routine *) self->array;

    for (i = 0; i < self->size; ++i) {
        if (&array[i] != directoryRoutine)
            continue;

        DirectoryRoutineFree(&array[i].type.dir);

        if (i + 1 < self->size)
            memmove(&array[i], &array[i + 1], sizeof(Routine) * (self->size - (i + 1)));

        --self->size;

        if (self->size)
            platformHeapResize((void **) &self->array, sizeof(Routine), self->size);
        else
            free(self->array);

    }
    return 0;
}

Routine FileRoutineNew(SOCKET socket, FILE *file, PlatformFileOffset start, PlatformFileOffset end,
                       char webPath[FILENAME_MAX]) {
    Routine self;

    self.type.file.file = file, self.type.file.start = start, self.type.file.end = end, self.state =
            TYPE_FILE_ROUTINE | STATE_CONTINUE;
    self.socketBuffer = socketBufferNew(socket, 0);

    strncpy(self.webPath, webPath, FILENAME_MAX - 1);
    platformFileSeek(self.type.file.file, self.type.file.start, SEEK_SET);
    return self;
}

size_t FileRoutineContinue(Routine *self) {
    size_t bytesRead, bytesWrite;
    char buffer[BUFSIZ];

    PlatformFileOffset currentPosition = platformFileTell(self->type.file.file);
    PlatformFileOffset remaining = self->type.file.end - currentPosition;

    bytesRead = platformFileRead(buffer, 1, (size_t) (remaining < BUFSIZ ? remaining : BUFSIZ), self->type.file.file);

    if (bytesRead > 0) {
        bytesWrite = socketBufferWriteData(&self->socketBuffer, buffer, bytesRead);

#pragma region Rewind file descriptor when socket buffer can not send all data

        if (bytesWrite < bytesRead)
            platformFileSeek(self->type.file.file,
                             (PlatformFileOffset) -((PlatformFileOffset) (bytesRead - bytesWrite)), SEEK_CUR);

#pragma endregion
    } else {
        bytesWrite = socketBufferFlush(&self->socketBuffer);

        if (bytesWrite == 0 && (self->socketBuffer.options & SOC_BUF_ERR_FULL) == 0 &&
            (self->socketBuffer.options & SOC_BUF_ERR_FAIL)) {
            self->state &= ~STATE_CONTINUE, self->state |= STATE_FINISH;
            eventHttpFinishInvoke(&self->socketBuffer.clientSocket, self->webPath, httpGet, 0);
        }
    }

    return bytesWrite;
}

void FileRoutineFree(FileRoutine *self) {
    platformFileClose(self->file);
}

char FileRoutineArrayAdd(RoutineArray *self, Routine fileRoutine) {
    Routine *array;

    ++self->size;
    if (self->size == 1)
        self->array = malloc(sizeof(Routine));
    else if (platformHeapResize((void **) &self->array, sizeof(Routine), self->size))
        return 1;

    /* TODO: if(!self->array) */

    array = (Routine *) self->array;
    memcpy(&array[self->size - 1], &fileRoutine, sizeof(Routine));

    return 0;
}

char FileRoutineArrayDel(RoutineArray *self, Routine *fileRoutine) {
    size_t i;
    Routine *array = (Routine *) self->array;

    for (i = 0; i < self->size; i++) {
        if (&array[i] != fileRoutine)
            continue;

        FileRoutineFree(&array[i].type.file);

        if (i + 1 < self->size)
            memmove(&array[i], &array[i + 1], sizeof(Routine) * (self->size - (i + 1)));

        --self->size;

        if (self->size)
            platformHeapResize((void **) &self->array, sizeof(Routine), self->size);
        else
            free(self->array);

        break; /* There should never be any duplicates in the array, early return */
    }
    return 0;
}
