#ifndef NEW_TH_WIN_SOCK_H
#define NEW_TH_WIN_SOCK_H

#include "../mscrtdl.h"

void *wSock1Available(void);

AdapterAddressArray *
wSock1GetAdapterInformation(void (arrayAdd)(AdapterAddressArray *, const char *, sa_family_t, char *));

#endif /* NEW_TH_WIN_SOCK_H */
