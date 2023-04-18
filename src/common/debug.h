#ifndef NEW_TH_DEBUG_H
#define NEW_TH_DEBUG_H

#ifdef NDEBUG
#define DEBUGPRT(f_, ...)
#else
#ifdef _WIN32
#define DEBUGPRT(f_, ...) printf((f_), __VA_ARGS__); fflush(stdout); system("PAUSE"); puts("")
#else
#define DEBUGPRT(f_, ...) printf((f_), __VA_ARGS__)
#endif /* _WIN32 */
#endif /* NDEBUG */

#endif /* NEW_TH_DEBUG_H */
