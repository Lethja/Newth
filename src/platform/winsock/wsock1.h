#ifndef NEW_TH_WIN_SOCK_H
#define NEW_TH_WIN_SOCK_H

#include "../mscrtdl.h"

void *wSock1Available();

AdapterAddressArray *wSock1GetAdapterInformation(void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t, char *));

#endif /* NEW_TH_WIN_SOCK_H */
