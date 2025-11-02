#include "../platform/platform.h"
#include "event.h"
#include "http.h"
#include "../common/hex.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma region Static Helper Functions

/**
 * Convert a ascii character to its uri escape equivalent
 * @param ascii In: The character to convert
 * @param out Out: The hex representation of that ascii character suitable for uris
 */
static inline void AsciiToHex(char ascii, char out[8]) {
	if (isalnum(ascii))
		out[0] = ascii, out[1] = '\0';
	else
		sprintf(out, "%%%x", (unsigned char) ascii);
}

/**
 * Get the filename of a path and convert it for use in a html <a href>
 * @param path In: The path to convert into a link
 * @param maxPaths In: The maximum depth a path name can go
 * @param linkPath Out: The raw data that should be inserted into 'href' attribute of an 'a' element
 * @param displayPath Out: The raw data that should be inserted inside the 'a' element
 */
static inline void
GetPathName(const char *path, size_t maxPaths, char linkPath[FILENAME_MAX], char displayPath[FILENAME_MAX]) {
	size_t i, max = strlen(path) + 1, currentPath = 0;
	memcpy(linkPath, path, max);

	for (i = 0; i < max; ++i) {
		if (linkPath[i] != '/')
			continue;
		else {
			if (currentPath != maxPaths)
				++currentPath;
			else {
				if (i)
					linkPath[i + 1] = '\0';
				else {
					linkPath[1] = '\0';
					memcpy(displayPath, linkPath, 2);
					return;
				}

				break;
			}
		}
	}

	if (linkPath[i - 1] == '\0') {
		--i;
		linkPath[i] = '/', linkPath[i + 1] = '\0';
	}

	if (currentPath) {
		do
			--i;
		while (linkPath[i] != '/');

		memcpy(displayPath, &linkPath[i + 1], max - i);
	} else
		memcpy(displayPath, linkPath, max);
}

/**
 * Count how many subdirectories exist on this path
 * @param path The path to calculate
 * @return The number of path dividers including zero if there are none
 */
static inline size_t GetPathCount(const char *path) {
	size_t r = 0;
	while (*path != '\0') {
		if (path[0] == '/' && path[1] != '\0')
			++r;
		++path;
	}
	return r;
}

#pragma endregion

char convertPathToUrl(char *path, size_t max) {
	size_t i = 0;
	char out[8];
	size_t outLen;

	while (path[i] != '\0') {
		if (!isalnum(path[i]) && path[i] != '/') {
			AsciiToHex(path[i], out);
			outLen = strlen(out);

			if (strlen(path) + outLen > max - 1)
				return -1;

			memmove(&path[i + outLen], &path[i + 1], strlen(&path[i]) + 1);
			memcpy(&path[i], out, outLen);

			i += outLen - 1;
		}

		i++;
	}

	return 0;
}

char *httpClientReadUri(const char *request) {
	char *start, *end, *path;
	size_t len;

	if (!(start = strchr(request, ' ')) || !(end = strchr(start + 1, ' ')))
		return NULL;

	len = end - start;

	path = malloc(len + 1);
	if (!path)
		return NULL;

	memcpy(path, start + 1, len - 1);
	path[len - 1] = '\0';

	hexConvertStringToAscii(path);

	return path;
}

static inline void httpHeaderDataClamp(char **str, size_t *length) {
	char *end;
	do {
		switch (**str) {
			case ' ':
			case ':':
			case '\t':
				++*str;
				continue;
			case '\r':
			case '\n':
				*length = 0;
				return;
			default:
				break;
		}
		break;
	} while (**str != '\0');

	end = strchr(*str, '\r');
	if (!end) {
		end = strchr(*str, '\n');
		if (!end) {
			*length = 0;
			return;
		}
	}

	*length = end - *str;
}


static inline int ValidHttpDateStr(const char *date) {
	return date[3] == ',' && date[4] == ' ' && date[7] == ' ' && date[11] == ' ' && date[16] == ' ' &&
	       date[19] == ':' && date[22] == ':' && date[25] == ' ';
}

char httpHeaderReadIfModifiedSince(const char *request, PlatformTimeStruct *tm) {
#define READ_DATE_MAX 39
	const char *headerValue = "If-Modified-Since";
	size_t length;
	char *data = strstr(request, headerValue);

	if (!data)
		return 1;

	data += strlen(headerValue);
	httpHeaderDataClamp(&data, &length);

	if (length && length < READ_DATE_MAX && ValidHttpDateStr(data)) {
		char date[READ_DATE_MAX];
		strncpy(date, data, READ_DATE_MAX);
		date[length] = '\0';
		return platformTimeGetFromHttpStr(date, tm);
	}

	return 1;
}

char httpHeaderReadRange(const char *request, PlatformFileOffset *start, PlatformFileOffset *end,
                         const PlatformFileOffset *max) {
#define MAX_RANGE_STR_BUF 128
	const char *headerValue = "Range", *parameter = "bytes=";
	size_t strLength;
	char *data = strstr(request, headerValue);

	if (data) {
		char range[MAX_RANGE_STR_BUF], *dash, *equ;
		data += strlen(headerValue);
		httpHeaderDataClamp(&data, &strLength);

		if (strLength >= MAX_RANGE_STR_BUF - 1)
			return 1;

		strncpy(range, data, strLength);
		range[strLength] = '\0';
		data = strstr(range, parameter);
		if (!data)
			return 1;

		data += strlen(parameter);
		equ = strchr(range, '=');
		dash = strchr(range, '-');

		if (!equ || !dash)
			return 1;

		*equ = *dash = '\0';

		if (equ + 1 != dash)
			*start = atol(equ + 1); /* NOLINT(cert-err34-c) */
		if (dash[1] != '\0')
			*end = atol(dash + 1); /* NOLINT(cert-err34-c) */
		if (*end == 0)
			*end = *max;

		return 0;
	}
	return 1;
}

void httpHeaderWriteAcceptRanges(SendBuffer *socketBuffer) {
	const char *str = "Accept-Ranges: bytes" HTTP_EOL;
	sendBufferWriteText(socketBuffer, str);
}

#pragma GCC diagnostic ignored "-Wformat" /* PF_OFFSET is defined in posix01.h */

void httpHeaderWriteRange(SendBuffer *socketBuffer, PlatformFileOffset start, PlatformFileOffset finish,
                          PlatformFileOffset fileLength) {
	sendBufferPrintf(socketBuffer, SB_DATA_SIZE, "Content-Range: bytes %"PF_OFFSET"-%"PF_OFFSET"/%"PF_OFFSET HTTP_EOL,
	                 start, finish == fileLength ? finish - 1 : finish, fileLength);
}

void httpHeaderWriteConnectionClose(SendBuffer *socketBuffer) {
	const char *connection = "Connection: close" HTTP_EOL;
	sendBufferWriteText(socketBuffer, connection);
}

void httpHeaderWriteDate(SendBuffer *socketBuffer) {
	char time[30];
	platformGetCurrentTime(time);
	sendBufferPrintf(socketBuffer, 40, "Date: %s" HTTP_EOL, time);
}

void httpHeaderWriteChunkedEncoding(SendBuffer *socketBuffer) {
	const char *chunked = "Transfer-Encoding: chunked" HTTP_EOL;
	sendBufferWriteText(socketBuffer, chunked);
}

void httpHeaderWriteContentLength(SendBuffer *socketBuffer, PlatformFileOffset length) {
	sendBufferPrintf(socketBuffer, 80, "Content-Length: %"PF_OFFSET HTTP_EOL, length);
}

void httpHeaderWriteContentLengthSt(SendBuffer *socketBuffer, PlatformFileStat *st) {
	httpHeaderWriteContentLength(socketBuffer, st->st_size);
}

void httpHeaderWriteLastModified(SendBuffer *socketBuffer, PlatformFileStat *st) {
	char time[30];
	platformGetTime(&st->st_mtime, time);
	sendBufferPrintf(socketBuffer, 50, "Last-Modified: %s" HTTP_EOL, time);
}

void httpHeaderWriteContentType(SendBuffer *socketBuffer, char *type, char *charSet) {
	size_t len = strlen(type) + strlen(charSet) + strlen("Content-Type: ;" HTTP_EOL) + 1;
	if (len < SB_DATA_SIZE)
		sendBufferPrintf(socketBuffer, len, "Content-Type: %s; %s" HTTP_EOL, type, charSet);
}

char *httpHeaderGetResponse(short response) {
	char *r;
	switch (response) {
		case 200:
			r = "200 OK";
			break;
		case 204:
			r = "204 No Content";
			break;
		case 206:
			r = "206 Partial Content";
			break;
		case 304:
			r = "304 Not Modified";
			break;
		case 404:
			r = "404 Not Found";
			break;
		case 431:
			r = "431 Request Header Fields Too Large";
			break;
		case 500:
		default:
			r = "500 Internal Server Error";
			break;
	}
	return r;
}

void httpHeaderWriteResponseStr(SendBuffer *socketBuffer, const char *response) {
	size_t len = strlen(HTTP_RES) + strlen(response) + strlen(HTTP_EOL) + 2;
	sendBufferPrintf(socketBuffer, len, HTTP_RES " %s" HTTP_EOL, response);
}

void httpHeaderWriteResponse(SendBuffer *socketBuffer, short response) {
	char *r = httpHeaderGetResponse(response);
	httpHeaderWriteResponseStr(socketBuffer, r);
}

void httpHeaderWriteEnd(SendBuffer *socketBuffer) {
	sendBufferWriteText(socketBuffer, HTTP_EOL);
}

void htmlHeaderWrite(char **buffer, char *title) {
	const char *h1 = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"
	                 "\t\"http://www.w3.org/TR/html4/strict.dtd\">\n"
	                 "<HTML>\n"
	                 "\t<HEAD>\n"
	                 "\t\t<TITLE>", *h2 = "</TITLE>\n"
	                                      "\t\t<STYLE TYPE=\"text/css\">\n"
	                                      "\t\t*{\n"
	                                      "\t\t\tfont-family: monospace;\n"
	                                      "\t\t}\n"
	                                      "\t\t\n"
	                                      "\t\ta:hover,tr:hover{\n"
	                                      "\t\t\tfont-weight: bold;\n"
	                                      "\t\t}\n"
	                                      "\t\t</STYLE>\n"
	                                      "\t</HEAD>\n\n"
	                                      "\t<BODY>\n";

	size_t max = strlen(h1) + strlen(h2) + strlen(title);
	char *tmp;

	if (!(tmp = malloc(max + 1)))
		return;

	sprintf(tmp, "%s%s%s", h1, title, h2);
	platformHeapStringAppendAndFree(buffer, tmp);
}

void htmlListStart(char **buffer) {
	const char *listStart = "\t\t<UL>\n";
	platformHeapStringAppend(buffer, listStart);
}

void htmlListEnd(char **buffer) {
	const char *listEnd = "\t\t</UL>\n";
	platformHeapStringAppend(buffer, listEnd);
}

void htmlFooterWrite(char **buffer) {
	const char *htmlEnd = "\t</BODY>\n" "</HTML>\n";
	platformHeapStringAppend(buffer, htmlEnd);
}

void htmlListWritePathLink(char **buffer, char *webPath) {
	const char *h1 = "\t\t\t<LI><A HREF=\"", *h2 = "\">", *h3 = "</A></LI>\n";
	char linkPath[FILENAME_MAX], *filePath = NULL, *tmp;
	size_t pathLen = strlen(webPath) + 1, i;
	size_t total;

	memcpy(linkPath, webPath, pathLen);

	/* Find the second last '/' in the webPath, use it as the name of the HTML link */
	for (i = pathLen - 3; i > 0; --i) {
		if (webPath[i] == '/') {
			filePath = &webPath[i];
			break;
		}
	}

	/* If a filePath couldn't be set, use the full path */
	if (filePath == NULL)
		filePath = webPath;

	convertPathToUrl(linkPath, FILENAME_MAX);
	total = strlen(linkPath) + strlen(filePath + 1) + strlen(h1) + strlen(h2) + strlen(h3);

	if ((tmp = malloc(total + 1))) {
		sprintf(tmp, "\t\t\t<LI><A HREF=\"%s\">%s</A></LI>\n", linkPath, filePath + 1);
		platformHeapStringAppendAndFree(buffer, tmp);
	}
}

void htmlBreadCrumbWrite(char **buffer, const char *webPath) {
	size_t i, max = GetPathCount(webPath) + 1;

	platformHeapStringAppend(buffer, "\t\t<DIV>\n");

	for (i = 0; i < max; ++i) {
		const char *h1 = "\t\t\t<A HREF=\"", *h2 = "\">", *h3 = "</A>\n";
		char internalBuffer[20 + FILENAME_MAX * 2], linkPath[FILENAME_MAX], displayPath[FILENAME_MAX];
		GetPathName(webPath, i, linkPath, displayPath);
		convertPathToUrl(linkPath, FILENAME_MAX);
		sprintf(internalBuffer, "%s%s%s%s%s", h1, linkPath, h2, displayPath, h3);
		platformHeapStringAppend(buffer, internalBuffer);
	}

	platformHeapStringAppend(buffer, "\t\t</DIV>\n\t\t<HR>\n");
}

size_t httpBodyWriteChunk(SendBuffer *socketBuffer, char **buffer) {
	size_t bufLen = strlen(*buffer);
	char hexStr[40] = {0};
	if (sprintf(hexStr, "%"HEX_OFFSET HTTP_EOL, bufLen)) {
		size_t r = sendBufferWriteText(socketBuffer, hexStr);
		r += sendBufferWriteData(socketBuffer, *buffer, bufLen);
		r += sendBufferWriteText(socketBuffer, HTTP_EOL);
		return r;
	}

	return 0;
}

size_t httpBodyWriteChunkEnding(SendBuffer *socketBuffer) {
	const char *chunkEnding = "0" HTTP_EOL HTTP_EOL;
	if (sendBufferWriteText(socketBuffer, chunkEnding) == 0)
		return 0;
	return sendBufferFlush(socketBuffer);
}

size_t httpBodyWriteText(SendBuffer *socketBuffer, const char *text) {
	return sendBufferWriteText(socketBuffer, text);
}

void httpHeaderWriteFileName(SendBuffer *socketBuffer, char *path) {
	char *name = strrchr(path, '/');

	if (!name)
		name = path;
	else
		++name;

	sendBufferPrintf(socketBuffer, strlen(name) + 40, "Content-Disposition: filename=\"%s\"" HTTP_EOL, name);
}

char httpHeaderHandleError(SendBuffer *socketBuffer, const char *path, char httpType, short error) {
#ifndef NDEBUG
	switch (error) {
		case 200:
		case 204:
		case 206:
		case 304:
			printf("Warning: valid HTTP request %d is in error path\n", error);
			break;
		default:
			printf("Warning: error HTTP request %d\n", error);
	}
#endif

	{
		char *errMsg = httpHeaderGetResponse(error);
		httpHeaderWriteResponseStr(socketBuffer, errMsg);
		HTTP_HEADER_CONNECTION_NO_REUSE_WRITE(socketBuffer);
		httpHeaderWriteDate(socketBuffer);
		httpHeaderWriteContentLength(socketBuffer, (PlatformFileOffset) strlen(errMsg));
		httpHeaderWriteEnd(socketBuffer);
		eventHttpRespondInvoke(&socketBuffer->clientSocket, path, httpType, error);

		if (httpBodyWriteText(socketBuffer, errMsg) == 0 || sendBufferFlush(socketBuffer) == 0)
			return 1;
	}
	return 0;
}

httpType httpClientReadType(const char *request) {
	switch (toupper(request[0])) {
		case 'G':
			return httpGet;
		case 'H':
			return httpHead;
		case 'P':
			return httpPost;
		default:
			return httpUnknown;
	}
}
