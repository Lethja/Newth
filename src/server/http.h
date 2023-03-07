#ifndef OPEN_WEB_HTTP_H
#define OPEN_WEB_HTTP_H

#include "../common/http.h"

#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>

#define WRITESTR(c, str) write(c, str, strlen(str))

char *httpClientReadUri(char *request);

void httpHeaderWriteDate(char header[BUFSIZ]);

void httpHeaderWriteContentLength(char header[BUFSIZ], size_t length);

void httpHeaderWriteContentLengthSt(char header[BUFSIZ], struct stat *st);

void httpHeaderWriteResponse(char header[BUFSIZ], short response);

void httpHeaderWriteEnd(char header[BUFSIZ]);

void httpHeaderWriteFileName(char header[BUFSIZ], char *path);

void httpBodyWriteFile(int clientSocket, FILE *file);

void httpBodyWriteText(int clientSocket, const char *text);

#endif /*OPEN_WEB_HTTP_H */
