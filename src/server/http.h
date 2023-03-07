#ifndef OPEN_WEB_HTTP_H
#define OPEN_WEB_HTTP_H

#include "../common/http.h"

#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>

char *httpClientReadUri(char *request);

void httpBodyWriteFile(int clientSocket, FILE *file);

size_t httpBodyWriteText(int clientSocket, const char *text);

void httpHeaderWriteContentLength(char header[BUFSIZ], size_t length);

void httpHeaderWriteContentLengthSt(char header[BUFSIZ], struct stat *st);

void httpHeaderWriteDate(char header[BUFSIZ]);

void httpHeaderWriteEnd(char header[BUFSIZ]);

void httpHeaderWriteFileName(char header[BUFSIZ], char *path);

void httpHeaderWriteLastModified(char header[BUFSIZ], struct stat *st);

void httpHeaderWriteResponse(char header[BUFSIZ], short response);

#endif /*OPEN_WEB_HTTP_H */
