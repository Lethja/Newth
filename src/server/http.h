#ifndef OPEN_WEB_HTTP_H
#define OPEN_WEB_HTTP_H

#include "../common/http.h"

#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>

#define WRITESTR(c, str) write(c, str, strlen(str))

char *httpClientReadUri(char *request);

void httpHeaderWriteDate(int clientSocket);

void httpHeaderWriteContentLength(int clientSocket, size_t length);

void httpHeaderWriteContentLengthSt(int clientSocket, struct stat *st);

void httpHeaderWriteResponse(int clientSocket, short response);

void httpHeaderWriteEnd(int clientSocket);

void httpBodyWriteFile(int clientSocket, FILE *file);

void httpBodyWriteText(int clientSocket, const char *text);

#endif /*OPEN_WEB_HTTP_H */
