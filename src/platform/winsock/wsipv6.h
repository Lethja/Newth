#include "../mscrtdl.h"

void *wSockIpv6Available();

AdapterAddressArray *wSockIpv6GetAdapterInformation(sa_family_t family,
                                                       void (arrayAdd)(AdapterAddressArray*, char *, sa_family_t,
                                                                        char *), void (nTop)(const void *, char *));
