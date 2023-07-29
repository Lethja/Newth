#include <malloc.h>
#include <string.h>
#include "routine.h"
#include "http.h"
#include "event.h"

size_t DirectoryRoutineContinue(Routine *self) {
    const size_t max = 10;
    size_t bytesWrite = 0, i, pathLen = strlen(self->webPath);
    PlatformDirEntry *entry;
    char *buffer = NULL;

    for (i = 0; i < max; ++i) {
        entry = platformDirRead(self->type.dir.directory);
        if (entry) {
            size_t entryLen;
            char pathBuf[BUFSIZ], *entryName;

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

                htmlListWritePathLink(&buffer, pathBuf);
                httpBodyWriteChunk(&self->socketBuffer, &buffer);

                bytesWrite += strlen(buffer);
                ++self->type.dir.count;
            }

            if (buffer)
                free(buffer), buffer = NULL;

        } else {
            const char *emptyDirectory = "\t\t\t<LI>Empty Directory</LI>\n";
            /* If the directory is empty, say so */
            if (!self->type.dir.count)
                buffer = malloc(strlen(emptyDirectory) + 1), strcpy(buffer, emptyDirectory);

            htmlListEnd(&buffer);
            htmlFooterWrite(&buffer);
            if (httpBodyWriteChunk(&self->socketBuffer, &buffer))
                httpBodyWriteChunkEnding(&self->socketBuffer);

            free(buffer);

            socketBufferFlush(&self->socketBuffer);

            return 0;
        }
    }

    socketBufferFlush(&self->socketBuffer);

    return bytesWrite;
}

Routine DirectoryRoutineNew(SocketBuffer socketBuffer, DIR *dir, const char *webPath, char *rootPath) {
    size_t i;

    Routine self;
    self.type.dir.directory = dir, self.type.dir.count = 0, self.type.dir.rootPath = rootPath, self.state =
            TYPE_ROUTINE_DIR | STATE_FLUSH;

    self.socketBuffer = socketBuffer;

    for (i = 0; i < FILENAME_MAX; ++i) {
        self.webPath[i] = webPath[i];
        if (webPath[i] == '\0')
            break;
    }

    return self;
}

void DirectoryRoutineFree(DirectoryRoutine *self) {
    platformDirClose(self->directory);
}

Routine FileRoutineNew(SocketBuffer socketBuffer, FILE *file, PlatformFileOffset start, PlatformFileOffset end,
                       char webPath[FILENAME_MAX]) {
    Routine self;

    self.type.file.file = file, self.type.file.start = start, self.type.file.end = end, self.state =
            TYPE_ROUTINE_FILE | STATE_FLUSH;
    self.socketBuffer = socketBuffer;

    strncpy(self.webPath, webPath, FILENAME_MAX - 1);
    platformFileSeek(self.type.file.file, self.type.file.start, SEEK_SET);
    return self;
}

size_t FileRoutineContinue(Routine *self) {
    size_t bytesRead, bytesWrite;
    char buffer[SB_DATA_SIZE];

    PlatformFileOffset currentPosition = platformFileTell(self->type.file.file);
    PlatformFileOffset remaining = self->type.file.end - currentPosition;

    bytesRead = platformFileRead(buffer, 1, (size_t) (remaining < SB_DATA_SIZE ? remaining : SB_DATA_SIZE),
                                 self->type.file.file);

    if (bytesRead > 0) {
        bytesWrite = socketBufferWriteData(&self->socketBuffer, buffer, bytesRead);

#pragma region Rewind file descriptor when socket buffer can not send all data

        if (bytesWrite < bytesRead)
            platformFileSeek(self->type.file.file, (PlatformFileOffset) (currentPosition + bytesWrite), SEEK_SET);

#pragma endregion
    } else
        bytesWrite = 0, self->state &= ~STATE_CONTINUE, self->state |= STATE_FLUSH;

    return bytesWrite;
}

void FileRoutineFree(FileRoutine *self) {
    platformFileClose(self->file);
}

char RoutineArrayAdd(RoutineArray *self, Routine fileRoutine) {
    Routine *array;

    ++self->size;
    if (self->size == 1)
        self->array = malloc(sizeof(Routine));
    else if (platformHeapResize((void **) &self->array, sizeof(Routine), self->size))
        return 1;

    if (!self->array)
        return 1;

    array = (Routine *) self->array;
    memcpy(&array[self->size - 1], &fileRoutine, sizeof(Routine));

    return 0;
}

static inline void RoutineArrayDelIdx(RoutineArray *self, size_t idx) {
    Routine *array = (Routine *) self->array, *routine = &array[idx];
    if (routine->state & TYPE_ROUTINE_FILE)
        FileRoutineFree(&array[idx].type.file);
    else if (routine->state & TYPE_ROUTINE_DIR)
        DirectoryRoutineFree(&array[idx].type.dir);

    if (idx + 1 < self->size)
        memmove(&array[idx], &array[idx + 1], sizeof(Routine) * (self->size - (idx + 1)));

    --self->size;

    if (self->size)
        platformHeapResize((void **) &self->array, sizeof(Routine), self->size);
    else
        free(self->array);
}

char RoutineArrayDelSocket(RoutineArray *self, SOCKET socket) {
    size_t i;
    Routine *array = (Routine *) self->array;

    for (i = 0; i < self->size; i++) {
        if (array[i].socketBuffer.clientSocket != socket)
            continue;

        RoutineArrayDelIdx(self, i);

        break; /* There should never be any duplicates in the array, early return */
    }
    return 0;
}

char RoutineArrayDel(RoutineArray *self, Routine *routine) {
    size_t i;
    Routine *array = (Routine *) self->array;

    for (i = 0; i < self->size; i++) {
        if (&array[i] != routine)
            continue;

        RoutineArrayDelIdx(self, i);

        break; /* There should never be any duplicates in the array, early return */
    }
    return 0;
}

Routine RoutineNew(SocketBuffer socketBuffer, const char *webPath) {
    size_t i;

    Routine self;
    self.state = TYPE_ROUTINE | STATE_FLUSH, self.socketBuffer = socketBuffer;

    for (i = 0; i < FILENAME_MAX; ++i) {
        self.webPath[i] = webPath[i];
        if (webPath[i] == '\0')
            break;
    }

    return self;
}

void RoutineTick(RoutineArray *routineArray, fd_set *serverWriteSockets, SOCKET *deferredSockets) {
    SOCKET i;
    Routine *routines = (Routine *) routineArray->array;

    for (i = *deferredSockets = 0; i < routineArray->size; ++i) {
        Routine *routine = &routines[i];

#pragma region Routine State Machine
        switch (routine->state) {
            case STATE_DEFER | TYPE_ROUTINE:
            case STATE_DEFER | TYPE_ROUTINE_FILE:
            case STATE_DEFER | TYPE_ROUTINE_DIR:
                if (FD_ISSET(routine->socketBuffer.clientSocket, serverWriteSockets)) {
                    ++*deferredSockets;
                    continue;
                } else {
                    if (routine->socketBuffer.options & SOC_BUF_ERR_FAIL) {
                        routine->state |= STATE_FAIL;
                    } else {
                        size_t sent = socketBufferFlush(&routine->socketBuffer);
                        if (sent == 0 || routine->socketBuffer.options & SOC_BUF_ERR_FAIL)
                            routine->state |= STATE_FAIL;
                        else if (routine->socketBuffer.options & SOC_BUF_ERR_FULL)
                            break;
                        else
                            routine->state |= STATE_FLUSH;
                    }

                    routine->state &= ~STATE_DEFER;
                    break;
                }

            case STATE_FLUSH | TYPE_ROUTINE:
            case STATE_FLUSH | TYPE_ROUTINE_FILE:
            case STATE_FLUSH | TYPE_ROUTINE_DIR:
                if (routine->socketBuffer.buffer) {
                    size_t sent = socketBufferFlush(&routine->socketBuffer);
                    if (sent == 0 && (routine->socketBuffer.options & SOC_BUF_ERR_FAIL) > 0)
                        routine->state |= STATE_FAIL;
                } else {
                    routine->state |= STATE_CONTINUE, routine->state &= ~STATE_FLUSH;
                }
                break;

            case STATE_CONTINUE | TYPE_ROUTINE:
                routine->state |= STATE_FINISH, routine->state &= ~STATE_CONTINUE;
                break;

            case STATE_CONTINUE | TYPE_ROUTINE_FILE:
                if (FileRoutineContinue(routine) || routine->socketBuffer.options & SOC_BUF_ERR_FULL)
                    break;

                if (routine->socketBuffer.options & SOC_BUF_ERR_FAIL)
                    routine->state = STATE_FAIL | TYPE_ROUTINE_FILE;
                else if (routine->socketBuffer.buffer) {
                    FD_SET(routine->socketBuffer.clientSocket, serverWriteSockets);
                    routine->state = STATE_DEFER | TYPE_ROUTINE_FILE;
                } else
                    routine->state = STATE_FINISH | TYPE_ROUTINE_FILE;

                if (!(routine->state & STATE_FAIL))
                    break;
                LINEDBG;
            case STATE_FINISH | TYPE_ROUTINE_FILE:
            case STATE_FAIL | TYPE_ROUTINE_FILE:
                FD_CLR(routine->socketBuffer.clientSocket, serverWriteSockets);
                eventHttpFinishInvoke(&routine->socketBuffer.clientSocket, routine->webPath, httpGet,
                                      routine->state & STATE_FAIL ? 1 : 0);
                socketBufferFailFree(&routine->socketBuffer);
                RoutineArrayDel(routineArray, routine);
                break;

            case STATE_CONTINUE | TYPE_ROUTINE_DIR:
                if (DirectoryRoutineContinue(routine))
                    break;
                LINEDBG;
                /* Fall through */
            case STATE_FINISH | TYPE_ROUTINE_DIR:
            case STATE_FAIL | TYPE_ROUTINE_DIR:
                FD_CLR(routine->socketBuffer.clientSocket, serverWriteSockets);
                eventHttpFinishInvoke(&routine->socketBuffer.clientSocket, routine->webPath, httpGet,
                                      routine->state & STATE_FAIL ? 1 : 0);
                socketBufferFailFree(&routine->socketBuffer);
                RoutineArrayDel(routineArray, routine);
                break;

            case STATE_FINISH | TYPE_ROUTINE:
            case STATE_FAIL | TYPE_ROUTINE:
                FD_CLR(routine->socketBuffer.clientSocket, serverWriteSockets);
                socketBufferFailFree(&routine->socketBuffer);
                RoutineArrayDel(routineArray, routine);
                break;
        }
#pragma endregion

    }
}
