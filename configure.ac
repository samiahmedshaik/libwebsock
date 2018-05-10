#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([libwebsock], [2.0], [payden@paydensutherland.com])
AM_INIT_AUTOMAKE
AC_DISABLE_STATIC
AC_CONFIG_SRCDIR([src/websock.c])
AC_CONFIG_HEADERS([src/config.h src/websock_config.h])
AC_CONFIG_MACRO_DIR([m4])
# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.
AC_CHECK_HEADERS([netdb.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset socket strstr])

AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_VERSION], ["$PACKAGE_VERSION"], [libwebsock version])
AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_STRING], ["$PACKAGE_STRING"], [libwebsock package string])
AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_NAME], ["$PACKAGE_NAME"], [libwebsock package name])
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])
AC_OUTPUT

