#include "../winsock.h"

AdapterAddressArray *platformGetAdapterInformationIpv6(sa_family_t family,
                                                       void (*arrayAdd)(AdapterAddressArray*, char *, sa_family_t,
                                                                        char *), void (*nTop)(const void *, char *));
