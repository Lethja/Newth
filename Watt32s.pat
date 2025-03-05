--- Watt32s/src/config.h	2025-03-05 15:57:59.636202810 +1100
+++ Watt32s/src/config.h	2025-03-05 15:57:52.377177819 +1100
@@ -21,10 +21,10 @@
 #undef USE_DEBUG       /* Include debug code */
 #undef USE_MULTICAST   /* Include IP multicast code by Jim Martin */
 #undef USE_BIND        /* Include Bind resolver code */
-#undef USE_BSD_API     /* Include BSD-sockets */
+#define USE_BSD_API     /* Include BSD-sockets */
 #undef USE_BSD_FATAL   /* Exit application on BSD-socket fatal errors */
 #undef USE_BOOTP       /* Include BOOTP client code */
-#undef USE_DHCP        /* Include DHCP boot client code */
+#define USE_DHCP        /* Include DHCP boot client code */
 #undef USE_RARP        /* Include RARP boot client code. Contributed by Dan Kegel. */
 #undef USE_GEOIP       /* Include GeoIP support. From Tor's geoip.c */
 #undef USE_IPV6        /* Include IPv6 dual-stack support */
