#include "wsock1.h"

#include <stdio.h>
#include <tdiinfo.h>

#define WSCTL_TCP_QUERY_INFORMATION 0
#define IP_MIB_ROUTETABLE_ENTRY_ID  0x101

#define IF_MIB_STATS_ID     1
#define MAX_PHYSADDR_SIZE   8

#define IP_MIB_STATS_ID             1
#define IP_MIB_ADDRTABLE_ENTRY_ID   0x102

typedef int (__stdcall *WsControlProc)(DWORD, DWORD, LPVOID, LPDWORD, LPVOID, LPDWORD);

typedef struct IPRouteEntry {
    ULONG ire_addr;
    ULONG ire_index;    /* matches if_index in IFEntry and iae_index in IPAddrEntry */
    ULONG ire_metric;
    ULONG ire_unk1;
    ULONG ire_unk2;
    ULONG ire_unk3;
    ULONG ire_gw;
    ULONG ire_unk4;
    ULONG ire_unk5;
    ULONG ire_unk6;
    ULONG ire_mask;
    ULONG ire_unk7;
} IPRouteEntry;

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


HMODULE wsock32 = NULL;

WsControlProc WsControl = NULL;

int setUp(void) {
    wsock32 = LoadLibrary("wsock32.dll");
    if (!wsock32) {
        fprintf(stderr, "LoadLibrary failed for wsock32.dll (%ld)\n", GetLastError());
        return 1;
    }

    WsControl = (WsControlProc) GetProcAddress(wsock32, "WsControl");
    if (!WsControl) {
        fprintf(stderr, "GetProcAddress failed for WsControl (%ld)\n", GetLastError());
        FreeLibrary(wsock32);
        return 1;
    }

    return 0;
}

void tearDown(void) {
    if (wsock32)
        FreeLibrary(wsock32);
}

/* TODO: So much to do. Use https://tangentsoft.com/wskfaq/articles/wscontrol.html as a reference implementation */
AdapterAddressArray *
platformGetAdapterInformationIpv4(void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t, char *)) {
    int result;
    TCP_REQUEST_QUERY_INFORMATION_EX tcpRequestQueryInfoEx;
    TDIEntityID *entityIds;
    DWORD tcpRequestBufSize, entityIdsBufSize, entityCount, ifCount = 0, i;

    memset(&tcpRequestQueryInfoEx, 0, sizeof(tcpRequestQueryInfoEx));
    tcpRequestQueryInfoEx.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    tcpRequestQueryInfoEx.ID.toi_entity.tei_instance = 0;
    tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_GENERIC;
    tcpRequestQueryInfoEx.ID.toi_type = INFO_TYPE_PROVIDER;
    tcpRequestQueryInfoEx.ID.toi_id = ENTITY_LIST_ID;

    tcpRequestBufSize = sizeof(tcpRequestQueryInfoEx);

    entityIdsBufSize = MAX_TDI_ENTITIES * sizeof(TDIEntityID);

    entityIds = (TDIEntityID *) calloc(entityIdsBufSize, 1);

    result = WsControl(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx, &tcpRequestBufSize, entityIds,
                       &entityIdsBufSize);

    if (result) {
        fprintf(stderr, "%s(%d) WsControl failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
        WSACleanup();
        FreeLibrary(wsock32);
        return NULL;
    }

    entityCount = entityIdsBufSize / sizeof(TDIEntityID);

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

            result = WsControl(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx, &tcpRequestBufSize,
                               &entityType, &entityTypeSize);

            if (result) {
                fprintf(stderr, "%s(%d) WsControl failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                WSACleanup();
                FreeLibrary(wsock32);
                return NULL;
            }

            if (entityType == IF_MIB) { /* Supports MIB-2 interface. */
                DWORD ifEntrySize;
                IFEntry *ifEntry;

                /* get snmp mib-2 info */
                tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
                tcpRequestQueryInfoEx.ID.toi_id = IF_MIB_STATS_ID;

                /* note: win95 winipcfg use 130 for MAX_IFDESCR_LEN while
                ddk\src\network\wshsmple\SMPLETCP.H defines it as 256
                we are trying to dup the winipcfg parameters for now */
                ifEntrySize = sizeof(IFEntry) + 128 + 1;
                ifEntry = (IFEntry *) calloc(ifEntrySize, 1);

                result = WsControl(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx, &tcpRequestBufSize,
                                   ifEntry, &ifEntrySize);

                if (result) {
                    fprintf(stderr, "%s(%d) WsControl failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                    WSACleanup();
                    FreeLibrary(wsock32);
                    return NULL;
                }

                /* print interface index and description */
                *(ifEntry->if_descr + ifEntry->if_descrlen) = 0;
                fprintf(stdout, "IF Index %lu  %s\n", ifEntry->if_index, ifEntry->if_descr);

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

            result = WsControl(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx, &tcpRequestBufSize,
                               &entityType, &entityTypeSize);

            if (result) {
                fprintf(stderr, "%s(%d) WsControl failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                WSACleanup();
                FreeLibrary(wsock32);
                return NULL;
            }

            if (entityType == CL_NL_IP) {   /* Entity implements IP. */
                DWORD ipSnmpInfoSize, ipAddrEntryBufSize, ipRouteEntryBufSize, j;
                IPRouteEntry *ipRouteEntry;
                IPSNMPInfo ipSnmpInfo;
                IPAddrEntry *ipAddrEntry;

                /* get ip snmp info */
                tcpRequestQueryInfoEx.ID.toi_class = INFO_CLASS_PROTOCOL;
                tcpRequestQueryInfoEx.ID.toi_id = IP_MIB_STATS_ID;


                ipSnmpInfoSize = sizeof(ipSnmpInfo);

                result = WsControl(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx, &tcpRequestBufSize,
                                   &ipSnmpInfo, &ipSnmpInfoSize);

                if (result) {
                    fprintf(stderr, "%s(%d) WsControl failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                    WSACleanup();
                    FreeLibrary(wsock32);
                    return NULL;
                }

                /* print ip snmp info */
                fprintf(stdout, "IP NumIfs: %lu\n", ipSnmpInfo.ipsi_numif);
                fprintf(stdout, "IP NumAddrs: %lu\n", ipSnmpInfo.ipsi_numaddr);
                fprintf(stdout, "IP NumRoutes: %lu\n", ipSnmpInfo.ipsi_numroutes);

                /* get ip address list */
                tcpRequestQueryInfoEx.ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

                ipAddrEntryBufSize = sizeof(IPAddrEntry) * ifCount;
                ipAddrEntry = (IPAddrEntry *) calloc(ipAddrEntryBufSize, 1);

                result = WsControl(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx, &tcpRequestBufSize,
                                   ipAddrEntry, &ipAddrEntryBufSize);

                if (result) {
                    fprintf(stderr, "%s(%d) WsControl failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                    WSACleanup();
                    FreeLibrary(wsock32);
                    return NULL;
                }

                /* print ip address list */
                for (j = 0; j < ifCount; j++) {

                    unsigned char *addr = (unsigned char *) &ipAddrEntry[j].iae_addr;
                    unsigned char *mask = (unsigned char *) &ipAddrEntry[j].iae_mask;

                    fprintf(stdout, "IF Index %ld  "
                                    "Address %d.%d.%d.%d  "
                                    "Mask %d.%d.%d.%d\n", ipAddrEntry[j].iae_index, addr[0], addr[1], addr[2], addr[3],
                            mask[0], mask[1], mask[2], mask[3]);
                }

                /* get route table */
                tcpRequestQueryInfoEx.ID.toi_id = IP_MIB_ROUTETABLE_ENTRY_ID;
                ipRouteEntryBufSize = sizeof(IPRouteEntry) * ipSnmpInfo.ipsi_numroutes;
                ipRouteEntry = (IPRouteEntry *) calloc(ipRouteEntryBufSize, 1);

                result = WsControl(IPPROTO_TCP, WSCTL_TCP_QUERY_INFORMATION, &tcpRequestQueryInfoEx, &tcpRequestBufSize,
                                   ipRouteEntry, &ipRouteEntryBufSize);

                if (result) {
                    fprintf(stderr, "%s(%d) WsControl failed (%d)\n", __FILE__, __LINE__, WSAGetLastError());
                    WSACleanup();
                    FreeLibrary(wsock32);
                    return NULL;
                }

                /* print route table */
                for (j = 0; j < ipSnmpInfo.ipsi_numroutes; j++) {

                    unsigned char *addr = (unsigned char *) &ipRouteEntry[j].ire_addr;
                    unsigned char *gw = (unsigned char *) &ipRouteEntry[j].ire_gw;
                    unsigned char *mask = (unsigned char *) &ipRouteEntry[j].ire_mask;

                    fprintf(stdout, "Route %d.%d.%d.%d  "
                                    "IF %ld  "
                                    "GW %d.%d.%d.%d  "
                                    "Mask %d.%d.%d.%d  "
                                    "Metric %ld\n", addr[0], addr[1], addr[2], addr[3], ipRouteEntry[j].ire_index,
                            gw[0], gw[1], gw[2], gw[3], mask[0], mask[1], mask[2], mask[3], ipRouteEntry[j].ire_metric);
                }
            }
        }
    }
    return NULL;
}
