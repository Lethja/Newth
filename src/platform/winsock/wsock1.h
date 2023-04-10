#ifndef NEW_TH_WIN_SOCK_H
#define NEW_TH_WIN_SOCK_H

#include "../winsock.h"

AdapterAddressArray *platformGetAdapterInformationIpv4(void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t,
                                                                       char *));

#endif /* NEW_TH_WIN_SOCK_H */
