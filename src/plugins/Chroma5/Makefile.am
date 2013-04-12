
## Process this file with automake to produce Makefile.in

INCLUDES = @LIBXML2_CFLAGS@              \
           @CHROMA5_CFLAGS@              \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  = @CHROMA5_LIBS@ -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@

if CHROMA5_ENABLED
lib_LTLIBRARIES = libChroma5.la
else
lib_LTLIBRARIES = 
endif

libChroma5_la_LDFLAGS = -module
libChroma5_la_LIBADD   = ../../libookala/libookala.la
libChroma5_la_SOURCES = \
	Chroma5.cpp \
	Chroma5.h



