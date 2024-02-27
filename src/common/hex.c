#include "hex.h"

#pragma region Static Helper Functions

static inline char HexToAscii(const char *hex) {
    char value = 0;
    unsigned char i;
    for (i = 0; i < 2; i++) {
        unsigned char ch = hex[i];
        if (ch >= '0' && ch <= '9') ch = ch - '0';
        else if (ch >= 'a' && ch <= 'f') ch = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F') ch = ch - 'A' + 10;
        value = (char) (value << 4 | (ch & 0xF));
    }
    return value;
}

#pragma endregion

void hexConvertStringToAscii(char *url) {
    char *i = url;

    while (*i != '\0') {
        if (*i == '%') {
            if (i[1] != '\0' && i[2] != '\0') {
                *i = HexToAscii(&i[1]);
                memmove(&i[1], &i[3], strlen(i) - 2);
            }
        }
        ++i;
    }

    /* Prevent system from being tricked into going up in a path it shouldn't */
    while ((i = strstr(url, "/..")))
        memmove(&i[1], &i[3], strlen(i) - 2);
}

size_t hexGetStringLength(size_t number) {
    size_t r = 0;

    if (number == 0)
        return 1;

    while (number != 0)
        number /= 16, ++r;

    return r;
}
