#ifndef OPEN_WEB_PLATFORM_H
#define OPEN_WEB_PLATFORM_H

#ifndef __STDC_VERSION__
#define inline
#endif

#ifdef _WIN32
#include "ws2.h"
#else
#include "unix.h"
#endif

#endif /* OPEN_WEB_PLATFORM_H */

