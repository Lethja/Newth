#ifndef OPEN_WEB_HTTP_H
#define OPEN_WEB_HTTP_H

#include "../common/http.h"

#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>

#pragma region HTML

/**
 * Write a basic HTML4 header with a title
 * @param buffer Out: The array to write into
 * @param title In: The title to set in the HTML4 <TITLE> tag
 */
void htmlHeaderWrite(char buffer[BUFSIZ], char *title);

/**
 * Start of a HTML4 <UL>
 * @param buffer Out: The array to write into
 */
void htmlListStart(char buffer[BUFSIZ]);

/**
 * End of the HTML4 <UL> tag started by htmlListStart()
 * @param buffer Out: The array to write into
 */
void htmlListEnd(char buffer[BUFSIZ]);

/**
 * HTML footer to finish document started by htmlHeaderWrite()
 * @param buffer Out: The array to write into
 */
void htmlFooterWrite(char buffer[BUFSIZ]);

/**
 * HTML List element with formatted <A HREF=> to the link
 * @param buffer Out: The array to write into
 * @param webPath In: String representation of the path to make a link to
 */
void htmlListWritePathLink(char buffer[BUFSIZ], char *webPath);

#pragma endregion

#pragma region HTTP URI

/**
 * Convert webURL into a normal system path
 * @param request In: The URL to convert
 * @return Free: The decoded representation of URL
 */
char *httpClientReadUri(char *request);

#pragma endregion

#pragma region HTTP Body

/**
 * Write a HTTP 1.1 chunk
 * @param clientSocket In: the TCP socket to send the chunk on
 * @param buffer In: The contents chunk body
 * @return 0 on success, other on error
 */
size_t httpBodyWriteChunk(int clientSocket, char buffer[BUFSIZ]);

/**
 * Write the end of a HTTP chunk request
 * @param clientSocket In: the TCP socket to send the last chunk on
 * @return 0 on success, other on error
 */
size_t httpBodyWriteChunkEnding(int clientSocket);

/**
 * Send the entirety of a file over TCP
 * @param clientSocket In: The TCP socket to send the file over
 * @param file In: The file to be sent over TCP
 * @return 0 on success, other on error
 */
size_t httpBodyWriteFile(int clientSocket, FILE *file);

/**
 * Send the entirety of a string over TCP
 * @param clientSocket In: The TCP socket to send the string over
 * @param text In: The string to sent over TCP
 * @return 0 on success, other on error
 */
size_t httpBodyWriteText(int clientSocket, const char *text);

#pragma endregion

#pragma region HTTP headers

/**
 * Write the HTTP 1.1 header Transfer-Encoding: chunked
 * @param header Concat: The buffer to write the HTTP header to
 */
void httpHeaderWriteChunkedEncoding(char header[BUFSIZ]);

/**
 * Write the HTTP header Content-Length
 * @param header Concat: The buffer to write the HTTP header to
 * @param length In: Content length in bytes
 */
void httpHeaderWriteContentLength(char header[BUFSIZ], size_t length);

/**
 * Write the HTTP header Content-Length using the files size
 * @param header Concat: The buffer to write the HTTP header to
 * @param st In: The status structure of a file you intend to send
 */
void httpHeaderWriteContentLengthSt(char header[BUFSIZ], struct stat *st);

/**
 * Write HTTP date header
 * @param header Concat: The buffer to write the HTTP header to
 */
void httpHeaderWriteDate(char header[BUFSIZ]);

/**
 * Write HTTP header end
 * @param header Concat: The buffer to write the HTTP header to
 */
void httpHeaderWriteEnd(char header[BUFSIZ]);

/**
 * Write HTTP Content-Disposition header with contents filename
 * @param header Concat: The buffer to write the HTTP header to
 * @param path In: The filename to write
 */
void httpHeaderWriteFileName(char header[BUFSIZ], char *path);

/**
 * Write HTTP Last-Modified header using the files modification date
 * @param header Concat: The buffer to write the HTTP header to
 * @param st In: The status structure of a file you intend to send
 */
void httpHeaderWriteLastModified(char header[BUFSIZ], struct stat *st);

/**
 * Write HTTP header response code
 * @param header Concat: The buffer to write the HTTP header to
 * @param response The HTTP response code to write such as 200 or 404
 */
void httpHeaderWriteResponse(char header[BUFSIZ], short response);

#pragma endregion

#endif /*OPEN_WEB_HTTP_H */
