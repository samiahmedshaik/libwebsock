LIBWEBSOCK_LIBRARY_VERSION=3:0
lib_LTLIBRARIES=libwebsock.la
libwebsock_la_SOURCES = websock.c base64.c sha1.c frames.c default_callbacks.c api.c sha1.h base64.h utf.c util.c logger.c
libwebsock_la_LDFLAGS = -version-info $(LIBWEBSOCK_LIBRARY_VERSION)
libwebsock_la_CFLAGS = -Wall -Wmissing-prototypes
library_includedir=$(includedir)/websock
library_include_HEADERS = websock.h api.h default_callbacks.h frames.h util.h utf.h types.h websock_config.h logger.h
ACLOCAL_AMFLAGS = -I m4
