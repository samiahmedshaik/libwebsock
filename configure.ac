#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([libwebsock], [1.0.7], [payden@paydensutherland.com])
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

AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_VERSION], ["$PACKAGE_VERSION"], [libwebsock version])
AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_STRING], ["$PACKAGE_STRING"], [libwebsock package string])
AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_NAME], ["$PACKAGE_NAME"], [libwebsock package name])
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])
AC_OUTPUT_FILES([Makefile src/Makefile examples/Makefile])
AC_OUTPUT

K_LIB([event_openssl], [bufferevent_openssl_socket_new], [
  				LIBS="-levent_openssl ${LIBS}"
  				have_ssl=yes
  			], [have_ssl=no])
  		], 
  		[have_ssl=no])
  	],
  	[have_ssl=no])
  	
AS_IF([test "x$have_ssl" = "xyes"],
	[
		AC_DEFINE([WEBSOCK_HAVE_SSL], [1], [Define if building SSL support])
	],
	[AS_IF([test "x$with_ssl" = "xyes"],
		[AC_MSG_ERROR([SSL support requested but not found])
	])])
  	
AM_CONDITIONAL([HAVE_SSL], [test "x$have_ssl" = "xyes"])
AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_VERSION], ["$PACKAGE_VERSION"], [libwebsock version])
AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_STRING], ["$PACKAGE_STRING"], [libwebsock package string])
AC_DEFINE_UNQUOTED([WEBSOCK_PACKAGE_NAME], ["$PACKAGE_NAME"], [libwebsock package name])
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])
AC_OUTPUT

_FILES([Makefile src/Makefile examples/Makefile])
AC_OUTPUT

