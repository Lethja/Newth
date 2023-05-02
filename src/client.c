#include "common/http.h"
#include "platform/platform.h"

#include <ctype.h>
#include <stdio.h>

#ifdef WIN32

static WSADATA wsaData;

#endif

long errln = __LINE__;

typedef struct ServerHeaderResponse {
    short code;
    short type;
    PlatformTimeStruct *modifiedDate;
    PlatformFileOffset length;
    char path[FILENAME_MAX];
    char file[FILENAME_MAX];
} ServerHeaderResponse;

char headerStructGetEssential(ServerHeaderResponse *self, FILE *headerFile) {
    char *p, buf[BUFSIZ] = "";

    fseek(headerFile, 0L, SEEK_SET);
    if (fgets(buf, BUFSIZ, headerFile) == NULL)
        goto headerStructGetEssentialFailed;

    /* Check this is actually HTTP */
    if (toupper(buf[0]) != 'H')
        goto headerStructGetEssentialFailed;

    /* Get response code */
    p = strchr(buf, ' ');

    if (p) {
        char *start = &p[1], code[4];
        size_t len;

        p = strchr(&p[2], ' ');
        if (p)
            len = p - start;
        else
            len = strlen(start);

        if (len != 3)
            goto headerStructGetEssentialFailed;

        code[0] = start[0], code[1] = start[1], code[2] = start[2], code[3] = '\0';
        self->code = (short) (atoi(code));
    }

    return 0;

    headerStructGetEssentialFailed:
    self->code = 0;
    return 1;
}

char headerStructGetFileName(ServerHeaderResponse *self, FILE *headerFile) {
    char buf[BUFSIZ] = "";

    fseek(headerFile, 0L, SEEK_SET);
    while (fgets(buf, BUFSIZ, headerFile)) {
        char *p;

        if ((p = strstr(buf, "Content-Disposition:"))) {
            if ((p = strstr(p, "filename="))) {
                char *s;
                s = strchr(p, '"');
                if (s) {
                    char *e;
                    ++s;
                    e = strchr(s, '"');
                    if (e && s - e < BUFSIZ) {
                        *e = '\0';
                        strcpy(self->file, s);
                        return 0;
                    }
                }
            }
        }
    }

    strcpy(self->file, "download.txt");
    return 1;
}

ServerHeaderResponse headerStructNew(FILE *headerFile) {
    ServerHeaderResponse self;
    if (headerStructGetEssential(&self, headerFile))
        return self;

    headerStructGetFileName(&self, headerFile);

    /* TODO: Implement content-length lookup */
    self.length = 0;

    return self;
}

#ifdef WIN32

static inline void *CreateTempFile() {
    char path[BUFSIZ];
    GetTempPath(BUFSIZ, path);
    strncat(path, "dlbuf", BUFSIZ);
    return fopen(path, "w+bTD");
}

#else

static inline void *CreateTempFile() {
    return tmpfile();
}

#endif /* WIN32 */

static char *getAddressPath(char *uri) {
    char *colon, *slash;

    colon = strchr(uri, ':');

    if (colon && colon[1] == '/' && colon[2] == '/')
        slash = strchr(&colon[3], '/');
    else
        slash = strchr(uri, '/');

    if (slash)
        return slash;
    return "/";
}

static char getAddressAndPort(char *uri, char *address, unsigned short *port) {
    char *it = uri;
    size_t hold, len = hold = 0, i, j;
    char portStr[6] = "";

    while (*it != '\0') {
        switch (*it) {
            case '[':
                ++hold;
                break;
            case ']':
                if (!hold)
                    return 1;
                --hold;
                break;
            case ':':
                goto getAddressAndPortBreakOutLoop;
        }
        ++len;
        ++it;
    }

    getAddressAndPortBreakOutLoop:
    if (len == 0 || len >= FILENAME_MAX) {
        errln = __LINE__; return 1;
	}

    hold = it - uri;
    memcpy(address, uri, hold);
    address[hold] = '\0';

    if (*it == '\0')
        portStr[0] = '8', portStr[1] = '0', portStr[2] = '\0';
    else {
        for (i = 1, j = 0; j < 5; ++i, ++j) {
            if (it[i] == '\0' || it[i] == '/' || !isdigit(it[i])) {
                if (j)
                    portStr[j] = '\0';
                else
                    portStr[0] = '8', portStr[1] = '0', portStr[2] = '\0';
                break;
            } else {
                portStr[j] = it[i];
            }
        }
    }

    *port = (unsigned short) atoi(portStr);

    return 0;
}

char clientConnectSocketTo(const SOCKET *socket, char *uri, int type) {
    struct sockaddr_storage addrStruct;
    char address[FILENAME_MAX];
    unsigned short port = 80;
    /*
	int i;
    struct hostent *host;
	*/

    if (getAddressAndPort(uri, (char *) &address, &port)) {
        errln = __LINE__; return -1;
	}

	/*
		TODO: Check if the string is a valid IPV4 address internally before using gethostbyname()
		as Windows 95As implementation dosen't appear to notice raw addresses
	*/

    /*host = gethostbyname(address);
    if (!host || host->h_addrtype != type || host->h_length == 0) {
		printf("Unable to get hostname of '%s'\n", address);
		if(host) {
			printf("%d vs %d, length = %d", host->h_addrtype, type, host->h_length);
		}

		errln = __LINE__; return -1;
	}*/

    if (type == AF_INET) {
        struct sockaddr_in *serverAddress = (struct sockaddr_in *) &addrStruct;
		long addr = inet_addr(address);
        memset(serverAddress, 0, sizeof(addrStruct));

        serverAddress->sin_family = type;
        serverAddress->sin_port = htons(port);
		serverAddress->sin_addr.s_addr = addr;

        /*for (i = 0; host->h_addr_list[i] != NULL; ++i) {
            memcpy(&serverAddress->sin_addr.s_addr, host->h_addr_list[0], host->h_length);

            if (connect(*socket, (SA *) serverAddress, sizeof(addrStruct)) == 0)
                return 0;
        }*/

		if (connect(*socket, (SA *) serverAddress, sizeof(addrStruct)) == 0)
                return 0;
    }

    errln = __LINE__; return -1;
}

void writeToDisk(ServerHeaderResponse *self, const SOCKET *socket, FILE *disk) {
    size_t totalBytes = 0;
    ssize_t bytesReceived;
    char rxBytes[BUFSIZ] = "";

    while ((bytesReceived = recv(*socket, rxBytes, BUFSIZ - 1, 0)) > 0) {
        fwrite(rxBytes, bytesReceived, 1, disk);
        totalBytes += bytesReceived;
        putc('.', stdout);
    }

    fflush(disk);

    if (self->length != 0) {
        if (self->length != totalBytes)
            printf("\nUnusual amount of bytes transferred. File might be corrupt.\n");
    }
}

FILE *getHeader(const SOCKET *socket) {
    char rxBytes[BUFSIZ] = "";
    size_t totalBytes = 0;
    ssize_t bytesReceived;
    FILE *header;

    header = CreateTempFile();

    if (header) {
        char *headEnd = NULL;

        while ((bytesReceived = recv(*socket, rxBytes, BUFSIZ - 1, MSG_PEEK)) > 0 && !headEnd) {

            /* Check if the end of header was part of this buffer and truncate anything after if so */
            rxBytes[bytesReceived] = '\0';
            if ((headEnd = strstr(rxBytes, HTTP_EOL HTTP_EOL)) != NULL)
                bytesReceived = (headEnd - rxBytes);

            /* Write header buffer into temporary file */
            fwrite(rxBytes, bytesReceived, 1, header);
            totalBytes += bytesReceived;

            /* Non-peek to move buffer along */
            recv(*socket, rxBytes, bytesReceived, 0);
        }

        /* If the buffer has a body after the head then jump over it so the next function is ready to read the body */
        if (headEnd)
            recv(*socket, rxBytes, 4, 0);

        return header;
    }

    return NULL;
}

char sendRequest(const SOCKET *socket, const char *type, const char *path) {
    char txLine[BUFSIZ];
    int sendBytes;

    sprintf(txLine, "%s %s HTTP/1.1" HTTP_EOL HTTP_EOL, type, path);
    sendBytes = (int) strlen(txLine);

    if (send(*socket, txLine, sendBytes, 0) != sendBytes)
        return 1;

    return 0;
}

int main(int argc, char **argv) {
    FILE *file;
    SOCKET socketFd;
    struct sockaddr_storage serverAddress;
    ServerHeaderResponse header;

    if (argc != 2) {
		errln = __LINE__; goto errorOut;
	}

#ifdef WIN32
    if (WSAStartup(MAKEWORD(1, 1), &wsaData)) {
		errln = __LINE__; goto errorOut;
	}
#endif

    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        errln = __LINE__; goto errorOut;
	}

    memset(&serverAddress, 0, sizeof(serverAddress));

    if (clientConnectSocketTo(&socketFd, argv[1], AF_INET)) {
        /*errln = __LINE__;*/ goto errorOut;
	}

    if (sendRequest(&socketFd, "GET", getAddressPath(argv[1]))) {
        errln = __LINE__; goto errorOut;
	}

    file = getHeader(&socketFd);
    if (!file) {
        errln = __LINE__; goto errorOut;
	}

    header = headerStructNew(file);
    fclose(file);

    if (!header.code) {
        errln = __LINE__; goto errorOut;
	}

    file = fopen(header.file, "w+b");
    if (file) {
        printf("Downloading '%s'\n", header.file);
        writeToDisk(&header, &socketFd, file);
        fclose(file);
    } else {
        printf("Unable to write to '%s'", header.file);
        perror("");
    }

    fflush(stdout);
#ifdef WIN32
    WSACleanup();
#endif
    return 0;

    errorOut:
#ifdef WIN32
    WSACleanup();
#endif
    fprintf(stderr, "ERR %ld\n", errln);
	getchar();
    return 1;
}
