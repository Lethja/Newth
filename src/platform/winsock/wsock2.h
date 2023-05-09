#ifndef NEW_TH_WIN_SOCK_2_H
#define NEW_TH_WIN_SOCK_2_H

#include "../mscrtdl.h"

void *wSock2Available();

AdapterAddressArray *wSock2GetAdapterInformation(void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t, char *));

#pragma region portable iphlpapi.h

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "UnusedLocalVariable"

#ifndef __C89_NAMELESS
#define __C89_NAMELESS
#endif /* __C89_NAMELESS */

#ifndef _DEF_WINBOOL_
#define _DEF_WINBOOL_
typedef int WINBOOL;
#endif /* _DEF_WINBOOL_ */

#ifndef _TIME32_T_DEFINED
#define _TIME32_T_DEFINED
typedef long __time32_t;
#endif /* _TIME32_T_DEFINED */

#ifndef _TIME64_T_DEFINED
#define _TIME64_T_DEFINED
typedef __int64 __time64_t;
#endif /* _TIME64_T_DEFINED */

#ifndef MSVC89
typedef unsigned __LONG32 ULONG;
typedef ULONG *PULONG;
#else
typedef unsigned char UINT8,*PUINT8;
typedef unsigned short UINT16,*PUINT16;
typedef unsigned int UINT32,*PUINT32;
typedef int socklen_t;
#endif /* !MSVC89 */

#define IF_TYPE_SOFTWARE_LOOPBACK 24
#define MAX_ADAPTER_ADDRESS_LENGTH 8
#define MAX_ADAPTER_DESCRIPTION_LENGTH 128
#define MAX_ADAPTER_NAME_LENGTH 256
#define MAX_DHCPV6_DUID_LENGTH 130
#define MAX_DNS_SUFFIX_STRING_LENGTH 256

typedef enum {
    IpPrefixOriginOther = 0,
    IpPrefixOriginManual,
    IpPrefixOriginWellKnown,
    IpPrefixOriginDhcp,
    IpPrefixOriginRouterAdvertisement,
    IpPrefixOriginUnchanged = 16
} NL_PREFIX_ORIGIN;

typedef enum {
    NlsoOther = 0,
    NlsoManual,
    NlsoWellKnown,
    NlsoDhcp,
    NlsoLinkLayerAddress,
    NlsoRandom,
    IpSuffixOriginOther = 0,
    IpSuffixOriginManual,
    IpSuffixOriginWellKnown,
    IpSuffixOriginDhcp,
    IpSuffixOriginLinkLayerAddress,
    IpSuffixOriginRandom,
    IpSuffixOriginUnchanged = 16
} NL_SUFFIX_ORIGIN;

typedef enum {
    NldsInvalid,
    NldsTentative,
    NldsDuplicate,
    NldsDeprecated,
    NldsPreferred,
    IpDadStateInvalid = 0,
    IpDadStateTentative,
    IpDadStateDuplicate,
    IpDadStateDeprecated,
    IpDadStatePreferred,
} NL_DAD_STATE;

typedef NL_PREFIX_ORIGIN IP_PREFIX_ORIGIN;
typedef NL_SUFFIX_ORIGIN IP_SUFFIX_ORIGIN;
typedef NL_DAD_STATE IP_DAD_STATE;

typedef struct _IP_ADAPTER_UNICAST_ADDRESS_XP {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD Flags;
        };
    };
    struct _IP_ADAPTER_UNICAST_ADDRESS_XP *Next;
    SOCKET_ADDRESS Address;
    IP_PREFIX_ORIGIN PrefixOrigin;
    IP_SUFFIX_ORIGIN SuffixOrigin;
    IP_DAD_STATE DadState;
    ULONG ValidLifetime;
    ULONG PreferredLifetime;
    ULONG LeaseLifetime;
} IP_ADAPTER_UNICAST_ADDRESS_XP, *PIP_ADAPTER_UNICAST_ADDRESS_XP;

typedef struct _IP_ADAPTER_UNICAST_ADDRESS_LH {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD Flags;
        };
    };
    struct _IP_ADAPTER_UNICAST_ADDRESS_LH *Next;
    SOCKET_ADDRESS Address;
    IP_PREFIX_ORIGIN PrefixOrigin;
    IP_SUFFIX_ORIGIN SuffixOrigin;
    IP_DAD_STATE DadState;
    ULONG ValidLifetime;
    ULONG PreferredLifetime;
    ULONG LeaseLifetime;
    UINT8 OnLinkPrefixLength;
} IP_ADAPTER_UNICAST_ADDRESS_LH, *PIP_ADAPTER_UNICAST_ADDRESS_LH;

#if (_WIN32_WINNT >= 0x0600)
typedef IP_ADAPTER_UNICAST_ADDRESS_LH IP_ADAPTER_UNICAST_ADDRESS;
typedef IP_ADAPTER_UNICAST_ADDRESS_LH *PIP_ADAPTER_UNICAST_ADDRESS;
#else /* _WIN32_WINNT >= 0x0501 */
typedef IP_ADAPTER_UNICAST_ADDRESS_XP   IP_ADAPTER_UNICAST_ADDRESS;
typedef IP_ADAPTER_UNICAST_ADDRESS_XP *PIP_ADAPTER_UNICAST_ADDRESS;
#endif

typedef struct _IP_ADAPTER_ANYCAST_ADDRESS_XP {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD Flags;
        };
    };
    struct _IP_ADAPTER_ANYCAST_ADDRESS_XP *Next;
    SOCKET_ADDRESS Address;
} IP_ADAPTER_ANYCAST_ADDRESS_XP, *PIP_ADAPTER_ANYCAST_ADDRESS_XP;
typedef IP_ADAPTER_ANYCAST_ADDRESS_XP IP_ADAPTER_ANYCAST_ADDRESS;
typedef IP_ADAPTER_ANYCAST_ADDRESS_XP *PIP_ADAPTER_ANYCAST_ADDRESS;

typedef struct _IP_ADAPTER_MULTICAST_ADDRESS_XP {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD Flags;
        };
    };
    struct _IP_ADAPTER_MULTICAST_ADDRESS_XP *Next;
    SOCKET_ADDRESS Address;
} IP_ADAPTER_MULTICAST_ADDRESS_XP, *PIP_ADAPTER_MULTICAST_ADDRESS_XP;
typedef IP_ADAPTER_MULTICAST_ADDRESS_XP IP_ADAPTER_MULTICAST_ADDRESS;
typedef IP_ADAPTER_MULTICAST_ADDRESS_XP *PIP_ADAPTER_MULTICAST_ADDRESS;

#define IP_ADAPTER_ADDRESS_DNS_ELIGIBLE 0x01
#define IP_ADAPTER_ADDRESS_TRANSIENT 0x02
#define IP_ADAPTER_ADDRESS_PRIMARY 0x04

typedef struct _IP_ADAPTER_DNS_SERVER_ADDRESS_XP {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD Reserved;
        };
    };
    struct _IP_ADAPTER_DNS_SERVER_ADDRESS_XP *Next;
    SOCKET_ADDRESS Address;
} IP_ADAPTER_DNS_SERVER_ADDRESS_XP, *PIP_ADAPTER_DNS_SERVER_ADDRESS_XP;
typedef IP_ADAPTER_DNS_SERVER_ADDRESS_XP IP_ADAPTER_DNS_SERVER_ADDRESS;
typedef IP_ADAPTER_DNS_SERVER_ADDRESS_XP *PIP_ADAPTER_DNS_SERVER_ADDRESS;

typedef struct _IP_ADAPTER_PREFIX_XP {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD Flags;
        };
    };
    struct _IP_ADAPTER_PREFIX_XP *Next;
    SOCKET_ADDRESS Address;
    ULONG PrefixLength;
} IP_ADAPTER_PREFIX_XP, *PIP_ADAPTER_PREFIX_XP;
typedef IP_ADAPTER_PREFIX_XP IP_ADAPTER_PREFIX;
typedef IP_ADAPTER_PREFIX_XP *PIP_ADAPTER_PREFIX;

typedef struct _IP_ADAPTER_WINS_SERVER_ADDRESS_LH {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD Reserved;
        };
    };
    struct _IP_ADAPTER_WINS_SERVER_ADDRESS_LH *Next;
    SOCKET_ADDRESS Address;
} IP_ADAPTER_WINS_SERVER_ADDRESS_LH, *PIP_ADAPTER_WINS_SERVER_ADDRESS_LH;
#if (_WIN32_WINNT >= 0x0600)
typedef IP_ADAPTER_WINS_SERVER_ADDRESS_LH IP_ADAPTER_WINS_SERVER_ADDRESS;
typedef IP_ADAPTER_WINS_SERVER_ADDRESS_LH *PIP_ADAPTER_WINS_SERVER_ADDRESS;
#endif

typedef struct _IP_ADAPTER_GATEWAY_ADDRESS_LH {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD Reserved;
        };
    };
    struct _IP_ADAPTER_GATEWAY_ADDRESS_LH *Next;
    SOCKET_ADDRESS Address;
} IP_ADAPTER_GATEWAY_ADDRESS_LH, *PIP_ADAPTER_GATEWAY_ADDRESS_LH;
#if (_WIN32_WINNT >= 0x0600)
typedef IP_ADAPTER_GATEWAY_ADDRESS_LH IP_ADAPTER_GATEWAY_ADDRESS;
typedef IP_ADAPTER_GATEWAY_ADDRESS_LH *PIP_ADAPTER_GATEWAY_ADDRESS;
#endif

typedef struct _IP_ADAPTER_DNS_SUFFIX {
    struct _IP_ADAPTER_DNS_SUFFIX *Next;
    WCHAR String[MAX_DNS_SUFFIX_STRING_LENGTH];
} IP_ADAPTER_DNS_SUFFIX, *PIP_ADAPTER_DNS_SUFFIX;

#define IP_ADAPTER_DDNS_ENABLED 0x01
#define IP_ADAPTER_REGISTER_ADAPTER_SUFFIX 0x02
#define IP_ADAPTER_DHCP_ENABLED 0x04
#define IP_ADAPTER_RECEIVE_ONLY 0x08
#define IP_ADAPTER_NO_MULTICAST 0x10
#define IP_ADAPTER_IPV6_OTHER_STATEFUL_CONFIG 0x20
#define IP_ADAPTER_NETBIOS_OVER_TCPIP_ENABLED 0x40
#define IP_ADAPTER_IPV4_ENABLED 0x80
#define IP_ADAPTER_IPV6_ENABLED 0x100
#define IP_ADAPTER_IPV6_MANAGE_ADDRESS_CONFIG 0x200

typedef ULONG NET_IFINDEX, *PNET_IFINDEX;
typedef UINT16 NET_IFTYPE, *PNET_IFTYPE;
typedef NET_IFINDEX IF_INDEX, *PIF_INDEX;
typedef ULONG IFTYPE;

typedef enum _IF_OPER_STATUS {
    IfOperStatusUp = 1,
    IfOperStatusDown,
    IfOperStatusTesting,
    IfOperStatusUnknown,
    IfOperStatusDormant,
    IfOperStatusNotPresent,
    IfOperStatusLowerLayerDown
} IF_OPER_STATUS;

typedef union _NET_LUID {
    ULONG64 Value;
    __C89_NAMELESS struct { /* bitfield with 64 bit types. */
        ULONG64 Reserved: 24;
        ULONG64 NetLuidIndex: 24;
        ULONG64 IfType: 16;
    } Info;
} NET_LUID, *PNET_LUID;

typedef enum _NET_IF_CONNECTION_TYPE {
    NET_IF_CONNECTION_DEDICATED = 1, NET_IF_CONNECTION_PASSIVE, NET_IF_CONNECTION_DEMAND, NET_IF_CONNECTION_MAXIMUM
} NET_IF_CONNECTION_TYPE, *PNET_IF_CONNECTION_TYPE;

typedef enum _TUNNEL_TYPE {
    TUNNEL_TYPE_NONE = 0,
    TUNNEL_TYPE_OTHER = 1,
    TUNNEL_TYPE_DIRECT = 2,
    TUNNEL_TYPE_6TO4 = 11,
    TUNNEL_TYPE_ISATAP = 13,
    TUNNEL_TYPE_TEREDO = 14,
    TUNNEL_TYPE_IPHTTPS = 15
} TUNNEL_TYPE, *PTUNNEL_TYPE;

typedef NET_LUID IF_LUID, *PIF_LUID;
typedef UINT32 NET_IF_COMPARTMENT_ID, *PNET_IF_COMPARTMENT_ID;
typedef GUID NET_IF_NETWORK_GUID;

typedef struct _IP_ADAPTER_ADDRESSES_LH {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            IF_INDEX IfIndex;
        };
    };
    struct _IP_ADAPTER_ADDRESSES_LH *Next;
    PCHAR AdapterName;
    PIP_ADAPTER_UNICAST_ADDRESS_LH FirstUnicastAddress;
    PIP_ADAPTER_ANYCAST_ADDRESS_XP FirstAnycastAddress;
    PIP_ADAPTER_MULTICAST_ADDRESS_XP FirstMulticastAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS_XP FirstDnsServerAddress;
    PWCHAR DnsSuffix;
    PWCHAR Description;
    PWCHAR FriendlyName;
    BYTE PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
    ULONG PhysicalAddressLength;
    __C89_NAMELESS union {
        ULONG Flags;
        __C89_NAMELESS struct {
            ULONG DdnsEnabled: 1;
            ULONG RegisterAdapterSuffix: 1;
            ULONG Dhcpv4Enabled: 1;
            ULONG ReceiveOnly: 1;
            ULONG NoMulticast: 1;
            ULONG Ipv6OtherStatefulConfig: 1;
            ULONG NetbiosOverTcpipEnabled: 1;
            ULONG Ipv4Enabled: 1;
            ULONG Ipv6Enabled: 1;
            ULONG Ipv6ManagedAddressConfigurationSupported: 1;
        };
    };
    ULONG Mtu;
    IFTYPE IfType;
    IF_OPER_STATUS OperStatus;
    IF_INDEX Ipv6IfIndex;
    ULONG ZoneIndices[16];
    PIP_ADAPTER_PREFIX_XP FirstPrefix;

    ULONG64 TransmitLinkSpeed;
    ULONG64 ReceiveLinkSpeed;
    PIP_ADAPTER_WINS_SERVER_ADDRESS_LH FirstWinsServerAddress;
    PIP_ADAPTER_GATEWAY_ADDRESS_LH FirstGatewayAddress;
    ULONG Ipv4Metric;
    ULONG Ipv6Metric;
    IF_LUID Luid;
    SOCKET_ADDRESS Dhcpv4Server;
    NET_IF_COMPARTMENT_ID CompartmentId;
    NET_IF_NETWORK_GUID NetworkGuid;
    NET_IF_CONNECTION_TYPE ConnectionType;
    TUNNEL_TYPE TunnelType;

    SOCKET_ADDRESS Dhcpv6Server;
    BYTE Dhcpv6ClientDuid[MAX_DHCPV6_DUID_LENGTH];
    ULONG Dhcpv6ClientDuidLength;
    ULONG Dhcpv6Iaid;
#if (NTDDI_VERSION >= 0x06000100) /* NTDDI_VISTASP1 */
    PIP_ADAPTER_DNS_SUFFIX FirstDnsSuffix;
#endif
} IP_ADAPTER_ADDRESSES_LH, *PIP_ADAPTER_ADDRESSES_LH;

typedef struct _IP_ADAPTER_ADDRESSES_XP {
    __C89_NAMELESS union {
        ULONGLONG Alignment;
        __C89_NAMELESS struct {
            ULONG Length;
            DWORD IfIndex;
        };
    };
    struct _IP_ADAPTER_ADDRESSES_XP *Next;
    PCHAR AdapterName;
    PIP_ADAPTER_UNICAST_ADDRESS_XP FirstUnicastAddress;
    PIP_ADAPTER_ANYCAST_ADDRESS_XP FirstAnycastAddress;
    PIP_ADAPTER_MULTICAST_ADDRESS_XP FirstMulticastAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS_XP FirstDnsServerAddress;
    PWCHAR DnsSuffix;
    PWCHAR Description;
    PWCHAR FriendlyName;
    BYTE PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD PhysicalAddressLength;
    DWORD Flags;
    DWORD Mtu;
    DWORD IfType;
    IF_OPER_STATUS OperStatus;
    DWORD Ipv6IfIndex;
    DWORD ZoneIndices[16];
    PIP_ADAPTER_PREFIX_XP FirstPrefix;
} IP_ADAPTER_ADDRESSES_XP, *PIP_ADAPTER_ADDRESSES_XP;

#if (_WIN32_WINNT >= 0x0600)
typedef IP_ADAPTER_ADDRESSES_LH IP_ADAPTER_ADDRESSES;
typedef IP_ADAPTER_ADDRESSES_LH *PIP_ADAPTER_ADDRESSES;
#else /* _WIN32_WINNT >= 0x0501 */
typedef IP_ADAPTER_ADDRESSES_XP   IP_ADAPTER_ADDRESSES;
  typedef IP_ADAPTER_ADDRESSES_XP *PIP_ADAPTER_ADDRESSES;
#endif

typedef struct {
    char String[4 * 4];
} IP_ADDRESS_STRING, *PIP_ADDRESS_STRING, IP_MASK_STRING, *PIP_MASK_STRING;

typedef struct _IP_ADDR_STRING {
    struct _IP_ADDR_STRING *Next;
    IP_ADDRESS_STRING IpAddress;
    IP_MASK_STRING IpMask;
    DWORD Context;
} IP_ADDR_STRING, *PIP_ADDR_STRING;

typedef struct _IP_ADAPTER_INFO {
    struct _IP_ADAPTER_INFO *Next;
    DWORD ComboIndex;
    char AdapterName[MAX_ADAPTER_NAME_LENGTH + 4];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
    DWORD Index;
    UINT Type;
    UINT DhcpEnabled;
    PIP_ADDR_STRING CurrentIpAddress;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    WINBOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
#ifdef _WIN64
    __time64_t LeaseObtained;
    __time64_t LeaseExpires;
#else
    __time32_t LeaseObtained;
    __time32_t LeaseExpires;
#endif
} IP_ADAPTER_INFO, *PIP_ADAPTER_INFO;

#define GAA_FLAG_SKIP_UNICAST 0x0001
#define GAA_FLAG_SKIP_ANYCAST 0x0002
#define GAA_FLAG_SKIP_MULTICAST 0x0004
#define GAA_FLAG_SKIP_DNS_SERVER 0x0008
#define GAA_FLAG_INCLUDE_PREFIX 0x0010
#define GAA_FLAG_SKIP_FRIENDLY_NAME 0x0020
#define GAA_FLAG_INCLUDE_WINS_INFO 0x0040
#define GAA_FLAG_INCLUDE_GATEWAYS 0x0080
#define GAA_FLAG_INCLUDE_ALL_INTERFACES 0x0100
#define GAA_FLAG_INCLUDE_ALL_COMPARTMENTS 0x0200
#define GAA_FLAG_INCLUDE_TUNNEL_BINDINGORDER 0x0400

#pragma clang diagnostic pop

#pragma endregion

#endif /* NEW_TH_WIN_SOCK_2_H */
