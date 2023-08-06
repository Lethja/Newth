AC_PREREQ([2.71])
AC_INIT([Newth],[0.10])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_MACRO_DIRS([m4])

# Check if this is a Windows build
AC_MSG_CHECKING([whether to build with mingw for a Windows host])
AS_CASE([$host_os],
        [*mingw*], [windows_host=yes],
        [windows_host=no])
AC_MSG_RESULT([$windows_host])
AM_CONDITIONAL([WINDOWS_HOST], [test "x$windows_host" = "xyes"])

m4_include([m4/ax_check_enable_debug.m4])
AX_CHECK_ENABLE_DEBUG([no])

if test -z $CFLAGS; then
    CFLAGS='-O2 -s'
fi

# Checks for programs.
AC_PROG_CC
AC_SYS_LARGEFILE

# Checks for libraries.
PKG_CHECK_MODULES([CMOCKA], [cmocka], [have_cmocka=yes], [have_cmocka=no])
AM_CONDITIONAL([CMOCKA], [test "$have_cmocka" = "yes"])

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AM_PROG_AR

AC_OUTPUT
