#ifndef NEW_TH_WIN_SOCK_2_H
#define NEW_TH_WIN_SOCK_2_H

#include "../mscrtdl.h"

AdapterAddressArray *platformGetAdapterInformationIpv4(void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t,
                                                                       char *));

#endif /* NEW_TH_WIN_SOCK_2_H */