#include <stdio.h>
#include "wsock2.h"

typedef DWORD (WINAPI *AdapterInfoCall)(PIP_ADAPTER_INFO, PULONG);

HMODULE wSock2;
static AdapterInfoCall AdapterInfoFunc;

static void wSock2Free(void) {
    if (wSock2)
        FreeLibrary(wSock2);
}

void *wSock2Available(void) {
    wSock2 = LoadLibrary("Iphlpapi.dll");
    if (wSock2) {
        AdapterInfoFunc = (AdapterInfoCall) GetProcAddress(wSock2, "GetAdaptersInfo");
        return &wSock2Free;
    }
    return NULL;
}

AdapterAddressArray *wSock2GetAdapterInformation(void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t, char *)) {
    LINEDBG;
    if (AdapterInfoFunc) {
        AdapterAddressArray *array = NULL;
        PIP_ADAPTER_INFO pAdapterInfo = NULL;
        PIP_ADAPTER_INFO pAdapter = NULL;
        ULONG dwRetVal, ulOutBufLen = sizeof(IP_ADAPTER_INFO);

        LINEDBG;

        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
        if (pAdapterInfo == NULL) {
            printf("Error fetching 'GetAdaptersInfo'\n");
        }

        if (AdapterInfoFunc(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
            free(pAdapterInfo);
            pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
            if (pAdapterInfo == NULL) {
                printf("Error fetching 'GetAdaptersInfo'\n");
            }
        }

        if ((dwRetVal = AdapterInfoFunc(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
            pAdapter = pAdapterInfo;
            array = malloc(sizeof(AdapterAddressArray));

            if (!array) {
                free(pAdapterInfo);
                return NULL;
            }

            array->size = 0;

            while (pAdapter) {
                char ip[INET_ADDRSTRLEN];
                strncpy(ip, pAdapter->IpAddressList.IpAddress.String, INET_ADDRSTRLEN);

                if ((strcmp(ip, "0.0.0.0")) != 0) {
                    char desc[BUFSIZ];
                    strcpy(desc, pAdapter->Description);
                    arrayAdd(array, desc, 0, ip);
                }

                pAdapter = pAdapter->Next;
            }
        } else
            printf("GetAdaptersInfo failed with error: %ld\n", dwRetVal);

        if (pAdapterInfo)
            free(pAdapterInfo);

        return array;
    }
    return NULL;
}
