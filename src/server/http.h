#ifndef OPEN_WEB_HTTP_H
#define OPEN_WEB_HTTP_H

#include "../common/http.h"
#include "sockbufr.h"

#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>
#include <time.h>

typedef enum httpType {
    httpGet, httpHead, httpPost, httpUnknown
} httpType;

#pragma region HTML

/**
 * Write a breadcrumb of the webPath
 * @param buffer Concat: The buffer to write the bread crumb to
 * @param webPath In: The path to convert into a bread crumb
 */
void htmlBreadCrumbWrite(char buffer[BUFSIZ], const char *webPath);

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
char *httpClientReadUri(const char *request);

/**
 * Get the type of request from the buffer of a request
 * @param request In: The socket buffer to read the request from
 * @return httpUnknown on error otherwise a vaild httpType enum
 */
httpType httpClientReadType(const char *request);

/**
 * Convert 'If-Modified-Since' header to a tm struct for comparing
 * @param request In: the entire header buffer
 * @param tm Out: the struct to put valid data into
 * @return 0 on success, other on error
 */
char httpHeaderReadIfModifiedSince(const char *request, struct tm *tm);

/**
 * Convert 'Range' header to a set of off_t types
 * @param request In: the entire header buffer
 * @param start Out: The start of the range
 * @param end Out: The end of the range
 * @return 0 on success, other on error
 */
char httpHeaderReadRange(const char *request, off_t *start, off_t *end, off_t *max);

#pragma endregion

#pragma region HTTP Body

/**
 * Write a HTTP 1.1 chunk
 * @param socketBuffer In: the TCP socket buffer to append the chunk to
 * @param buffer In: The contents chunk body
 * @return 0 on success, other on error
 */
size_t httpBodyWriteChunk(SocketBuffer *socketBuffer, char buffer[BUFSIZ]);

/**
 * Write the end of a HTTP chunk request and flush
 * @param socketBuffer In: the TCP socket buffer to append the last chunk onto and flush
 * @return 0 on success, other on error
 */
size_t httpBodyWriteChunkEnding(SocketBuffer *socketBuffer);

/**
 * Send the entirety of a file over TCP
 * @param socketBuffer In: The TCP socket buffer to send the file over
 * @param file In: The file to be sent over TCP
 * @param start In: The offset to start the transfer at
 * @param finish In: The offset to stop the transfer at
 * @return 0 on success, other on error
 */
size_t httpBodyWriteFile(SocketBuffer *socketBuffer, FILE *file, off_t start, off_t finish);

/**
 * Send the entirety of a string over TCP
 * @param socketBuffer In: The TCP socket buffer to append text to
 * @param text In: The string to sent over TCP
 * @return 0 on success, other on error
 */
size_t httpBodyWriteText(SocketBuffer *socketBuffer, const char *text);

#pragma endregion

#pragma region HTTP headers

/**
 * Write HTTP header Accept-Ranges: bytes to indicate 206 support for this page to the client
 * @param socketBuffer In: The socketBuffer to write to
 */
void httpHeaderWriteAcceptRanges(SocketBuffer *socketBuffer);

/**
 * Write the HTTP 1.1 header Transfer-Encoding: chunked
 * @param socketBuffer In: The socketBuffer to write to
 */
void httpHeaderWriteChunkedEncoding(SocketBuffer *socketBuffer);

/**
 * Write the HTTP header Content-Length
 * @param socketBuffer In: The socketBuffer to write to
 * @param length In: Content length in bytes
 */
void httpHeaderWriteContentLength(SocketBuffer *socketBuffer, size_t length);

/**
 * Write the HTTP header Content-Length using the files size
 * @param socketBuffer In: The socketBuffer to write to
 * @param st In: The status structure of a file you intend to send
 */
void httpHeaderWriteContentLengthSt(SocketBuffer *socketBuffer, struct stat *st);

/**
 * Write the HTTP header Content-Type
 * @param socketBuffer In: The socketBuffer to write to
 * @param type In: The string with the type in it without the semicolon
 * @param charSet In: The string with the character set in it
 */
void httpHeaderWriteContentType(SocketBuffer *socketBuffer, char *type, char *charSet);

/**
 * Write HTTP date header
 * @param socketBuffer In: The socketBuffer to write to
 */
void httpHeaderWriteDate(SocketBuffer *socketBuffer);

/**
 * Write HTTP header end
 * @param socketBuffer In: The socketBuffer to write to
 */
void httpHeaderWriteEnd(SocketBuffer *socketBuffer);

/**
 * Write HTTP Content-Disposition header with contents filename
 * @param socketBuffer In: The socketBuffer to write to
 * @param path In: The filename to write
 */
void httpHeaderWriteFileName(SocketBuffer *socketBuffer, char *path);

/**
 * Write HTTP Last-Modified header using the files modification date
 * @param socketBuffer In: The socketBuffer to write to
 * @param st In: The status structure of a file you intend to send
 */
void httpHeaderWriteLastModified(SocketBuffer *socketBuffer, struct stat *st);

/**
 * Write a HTTP
 * @param socketBuffer
 * @param start
 * @param finish
 * @param fileLength
 */
void httpHeaderWriteRange(SocketBuffer *socketBuffer, off_t start, off_t finish, off_t fileLength);

/**
 * Write HTTP header response code
 * @param socketBuffer In: The socketBuffer to write to
 * @param response The HTTP response code to write such as 200 or 404
 */
void httpHeaderWriteResponse(SocketBuffer *socketBuffer, short response);

/**
 * Helper function for writing entire HTTP error replies under non-special circumstances
 * @param socketBuffer Socket buffer to write the error response to
 * @param path the web path that this error is a response to
 * @param httpType the http type that this error is a response to
 * @param error Error code to respond with
 * @return 0 on success, error on other
 */
char httpHeaderHandleError(SocketBuffer *socketBuffer, const char *path, char httpType, short error);

#endif /* OPEN_WEB_HTTP_H */
