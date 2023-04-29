#include <stdio.h>
#include "wsock2.h"

typedef DWORD (WINAPI *AdapterInfoProc)(PIP_ADAPTER_INFO, PULONG);

HMODULE wsock2;
AdapterInfoProc AdapterInfoFunc;

static void wSock2Free() {
    if(wsock2)
        FreeLibrary(wsock2);
}

void *wSock2Available() {
    wsock2 = LoadLibrary("Iphlpapi.dll");
    if(wsock2) {
        AdapterInfoFunc = (AdapterInfoProc) GetProcAddress(wsock2, "GetAdaptersInfo");
        return &wSock2Free;
    }
    return NULL;
}

AdapterAddressArray *
wSock2GetAdapterInformationIpv4(void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t, char *)) {
    if(AdapterInfoFunc) {
        AdapterAddressArray *array = NULL;
        PIP_ADAPTER_INFO pAdapterInfo = NULL;
        PIP_ADAPTER_INFO pAdapter = NULL;
        ULONG dwRetVal, ulOutBufLen = sizeof(IP_ADAPTER_INFO);

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
