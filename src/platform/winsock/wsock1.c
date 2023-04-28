#include "wsock1.h"
#include "../../common/debug.h"

#include <stdio.h>

#pragma region WsControl helper structs and defines

typedef int (__stdcall *WsControlProc)(DWORD, DWORD, LPVOID, LPDWORD, LPVOID, LPDWORD);

#define WSCTL_TCP_QUERY_INFORMATION 0
#define IF_MIB_STATS_ID     1
#define MAX_PHYSADDR_SIZE   8
#define IP_MIB_ADDRTABLE_ENTRY_ID   0x102

typedef struct IFEntry {
    ULONG if_index;
    ULONG if_type;
    ULONG if_mtu;
    ULONG if_speed;
    ULONG if_physaddrlen;
    UCHAR if_physaddr[MAX_PHYSADDR_SIZE];
    ULONG if_adminstatus;
    ULONG if_operstatus;
    ULONG if_lastchange;
    ULONG if_inoctets;
    ULONG if_inucastpkts;
    ULONG if_innucastpkts;
    ULONG if_indiscards;
    ULONG if_inerrors;
    ULONG if_inunknownprotos;
    ULONG if_outoctets;
    ULONG if_outucastpkts;
    ULONG if_outnucastpkts;
    ULONG if_outdiscards;
    ULONG if_outerrors;
    ULONG if_outqlen;
    ULONG if_descrlen;
    UCHAR if_descr[1];
} IFEntry;

typedef struct IPSNMPInfo {
    ULONG ipsi_forwarding;
    ULONG ipsi_defaultttl;
    ULONG ipsi_inreceives;
    ULONG ipsi_inhdrerrors;
    ULONG ipsi_inaddrerrors;
    ULONG ipsi_forwdatagrams;
    ULONG ipsi_inunknownprotos;
    ULONG ipsi_indiscards;
    ULONG ipsi_indelivers;
    ULONG ipsi_outrequests;
    ULONG ipsi_routingdiscards;
    ULONG ipsi_outdiscards;
    ULONG ipsi_outnoroutes;
    ULONG ipsi_reasmtimeout;
    ULONG ipsi_reasmreqds;
    ULONG ipsi_reasmoks;
    ULONG ipsi_reasmfails;
    ULONG ipsi_fragoks;
    ULONG ipsi_fragfails;
    ULONG ipsi_fragcreates;
    ULONG ipsi_numif;
    ULONG ipsi_numaddr;
    ULONG ipsi_numroutes;
} IPSNMPInfo;

typedef struct IPAddrEntry {
    ULONG iae_addr;
    ULONG iae_index;
    ULONG iae_mask;
    ULONG iae_bcastaddr;
    ULONG iae_reasmsize;
    USHORT iae_context;
    USHORT iae_pad;
} IPAddrEntry;

#define	MAX_TDI_ENTITIES 4096
#define	CONTEXT_SIZE 16
#define	GENERIC_ENTITY 0
#define	ENTITY_LIST_ID 0
#define	ENTITY_TYPE_ID 1
#define	INFO_CLASS_GENERIC 0x100
#define	INFO_TYPE_PROVIDER 0x100
#define	IF_ENTITY 0x200
#define	INFO_CLASS_PROTOCOL 0x200
#define	IF_MIB 0x202
#define	CL_NL_ENTITY 0x301
#define	CL_NL_IP 0x303

typedef struct TDIEntityID {
  ULONG  tei_entity;
  ULONG  tei_instance;
} TDIEntityID;

typedef struct _TDIObjectID {
	TDIEntityID  toi_entity;
	ULONG  toi_class;
	ULONG  toi_type;
	ULONG  toi_id;
} TDIObjectID;

typedef struct _TCP_REQUEST_QUERY_INFORMATION_EX {
  TDIObjectID  ID;
  ULONG_PTR  Context[CONTEXT_SIZE / sizeof(ULONG_PTR)];
} TCP_REQUEST_QUERY_INFORMATION_EX, *PTCP_REQUEST_QUERY_INFORMATION_EX;

#pragma endregion

#pragma region Network interface collection

typedef struct NIC {
    ULONG index;
    char *desc;
} NIC;

static NIC *networkInterfaces = NULL;
static size_t networkInterfacesSize = 0;

static void nicAdd(ULONG index, char *desc) {
    size_t i = networkInterfacesSize;
    ++networkInterfacesSize;
    if (networkInterfacesSize) {
        void *tmp = realloc(networkInterfaces, sizeof(NIC) * networkInterfacesSize);
        if (tmp)
            networkInterfaces = tmp;
    } else {
        networkInterfaces = malloc(sizeof(NIC) * networkInterfacesSize);
    }

    networkInterfaces[i].desc = desc;
    networkInterfaces[i].index = index;
}

static char *nicFind(ULONG index) {
    size_t i;
    for (i = 0; i < networkInterfacesSize; ++i) {
        if (networkInterfaces[i].index == index)
            return networkInterfaces[i].desc;
    }
    return NULL;
}

static void nicFree(void) {
    size_t i;
    for (i = 0; i < networkInterfacesSize; ++i) {
        free(networkInterfaces[i].desc);
    }
    if (networkInterfaces)
        free(networkInterfaces), networkInterfaces = NULL;
    networkInterfacesSize = 0;
}

#pragma endregion

WsControlProc WsControlFunc;

AdapterAddressArray *
platformGetAdapterInformationIpv4(void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t, char *)) {
    HMODULE wsock32 = LoadLibrary("wsock32.dll");
    DEBUGPRT("wsock32 = %p", wsock32);
    if (wsock32) {
        WSADATA WSAData;
        int result;
        TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
        TDIEntityID *entityIds;
        DWORD tcpRequestBufSize, entityIdsBufSize, entityCount, ifCount, i;

        AdapterAddressArray *array = NULL;

        WsControlFunc = (WsControlProc) GetProcAddress(wsock32, "WsControl");
        DEBUGPRT("WsControlFunc = %p", WsControlFunc);
        if (!WsControlFunc) {
            FreeLibrary(wsock32);
            return NULL;
        }

        result = WSAStartup(MAKEWORD(1, 1), &WSAData);
        DEBUGPRT("WSAStartup = %d", result);
        if (result) {
            fprintf(stderr, "WSAStartup failed (%d)\n", result);
            FreeLibrary(wsock32);
            return NULL;
        }

        memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));
        tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = GENERIC_ENTITY;
        tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = 0;
        tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_GENERIC;
        tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
        tcpRequestQueryInfoEx.ID.toi_id = ENTITY_LIST_ID;

        tcpRequestBufSize = sizeof(tcpRequestQueryInfoEx);

        entityIdsBufSize = MAX_TDI_ENTITIES * sizeof(TDIEntityID);

        entityIds = (TDIEntityID *) calloc(entityIdsBufSize, 1);

        result = WsControlFunc(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx, &tcpRequestBufSize,
                               entityIds, &entityIdsBufSize);
        DEBUGPRT("WsControlFunc(IPPROTO_TCP) = %d", result);
        if (result) {
            fprintf(stderr, "%s(%d) WsControlFunc failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
            WSACleanup();
            FreeLibrary(wsock32);
            return NULL;
        }

        entityCount = entityIdsBufSize / sizeof(TDIEntityID);
        ifCount = 0;

        /* print out the interface info for the generic interfaces */
        for (i = 0; i < entityCount; i++) {
            if (entityIds[i].tei_entity == IF_ENTITY) {
                ULONG entityType;
                DWORD entityTypeSize;
                ++ifCount;

                /* see if the interface supports snmp mib-2 info */
                memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));
                tcpRequestQueryInfoEx.ID.toi_entity = entityIds[i];
                tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_GENERIC;
                tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
                tcpRequestQueryInfoEx.ID.toi_id = ENTITY_TYPE_ID;


                entityTypeSize = sizeof(entityType);

                result = WsControlFunc(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx,
                                       &tcpRequestBufSize, &entityType, &entityTypeSize);
                DEBUGPRT("WsControlFunc(IPPROTO_TCP) = %d", result);
                if (result) {
                    fprintf(stderr, "%s(%d) WsControlFunc failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                    WSACleanup();
                    FreeLibrary(wsock32);
                    return NULL;
                }

                if (entityType == IF_MIB) { /* Supports MIB-2 interface. */
                    DWORD ifEntrySize;
                    IFEntry *ifEntry;
                    char *desc = NULL;

                    /* get snmp mib-2 info */
                    tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
                    tcpRequestQueryInfoEx.ID.toi_id = IF_MIB_STATS_ID;

                    /* note: win95 winipcfg use 130 for MAX_IFDESCR_LEN while
                    ddk\src\network\wshsmple\SMPLETCP.H defines it as 256
                    we are trying to dup the winipcfg parameters for now */
                    ifEntrySize = sizeof(IFEntry) + 128 + 1;
                    ifEntry = (IFEntry *) calloc(ifEntrySize, 1);

                    result = WsControlFunc(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx,
                                           &tcpRequestBufSize, ifEntry, &ifEntrySize);

                    if (result) {
                        fprintf(stderr, "%s(%d) WsControlFunc failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                        WSACleanup();
                        FreeLibrary(wsock32);
                        return NULL;
                    }

                    /* print interface index and description */
                    *(ifEntry->if_descr + ifEntry->if_descrlen) = 0;
                    desc = malloc(ifEntry->if_descrlen);
                    strncpy(desc, (char *) ifEntry->if_descr, ifEntry->if_descrlen);
                    nicAdd(ifEntry->if_index, desc);
                }
            }
        }

        /* find the ip interface */
        for (i = 0; i < entityCount; i++) {

            if (entityIds[i].tei_entity == CL_NL_ENTITY) {
                ULONG entityType;
                DWORD entityTypeSize;

                /* get ip interface info */
                memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));
                tcpRequestQueryInfoEx.ID.toi_entity = entityIds[i];
                tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_GENERIC;
                tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
                tcpRequestQueryInfoEx.ID.toi_id = ENTITY_TYPE_ID;

                entityTypeSize = sizeof(entityType);

                result = WsControlFunc(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx,
                                       &tcpRequestBufSize, &entityType, &entityTypeSize);

                if (result) {
                    fprintf(stderr, "%s(%d) WsControlFunc failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                    WSACleanup();
                    FreeLibrary(wsock32);
                    return NULL;
                }

                if (entityType == CL_NL_IP) {   /* Entity implements IP. */
                    DWORD ipAddrEntryBufSize, j;
                    IPAddrEntry *ipAddrEntry;

                    /* get ip address list */
                    tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
                    tcpRequestQueryInfoEx.ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

                    ipAddrEntryBufSize = sizeof(IPAddrEntry) * ifCount;
                    ipAddrEntry = (IPAddrEntry *) calloc(ipAddrEntryBufSize, 1);

                    result = WsControlFunc(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx,
                                           &tcpRequestBufSize, ipAddrEntry, &ipAddrEntryBufSize);

                    if (result) {
                        fprintf(stderr, "%s(%d) WsControlFunc failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                        WSACleanup();
                        FreeLibrary(wsock32);
                        return NULL;
                    }

                    if (ifCount)
                        array = malloc(sizeof(AdapterAddressArray)), array->size = 0;

                    /* Add ip address if interface is found */
                    for (j = 0; j < ifCount; j++) {
                        char *nicStr = nicFind(ipAddrEntry[j].iae_index);
                        /* If the interface index was found then this adapter and IP address are valid matches */
                        if (nicStr) {
                            char addrStr[INET_ADDRSTRLEN];
                            unsigned char *addr = (unsigned char *) &ipAddrEntry[j].iae_addr;
                            if (*addr != 127) { /* Not loop-back interface */
                                snprintf(addrStr, INET_ADDRSTRLEN, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
                                arrayAdd(array, nicStr, 0, addrStr);
                            }
                        }
                    }
                }
            }
        }
        nicFree();
        FreeLibrary(wsock32);
        DEBUGPRT("array = %p", array);
        if (array) {
            if (array->size)
                return array;
            else
                free(array);
        }
    } else { /* If all else fails at least do something */
        fprintf(stderr, "Couldn't load any network adapter information. Are you missing 'thwsock2.dll'?");
    }
    return NULL;
}
