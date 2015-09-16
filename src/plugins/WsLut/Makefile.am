
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @LIBXML2_CFLAGS@              \
           @WX_CXXFLAGS@                 \
           @X_CFLAGS@                    \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  = @X_LIBS@ -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@ @WX_LIBS@ @X_XF86VIDMODE_LIBS@ 

lib_LTLIBRARIES = libWsLut.la


libWsLut_la_LDFLAGS = -module
libWsLut_la_LIBADD   = ../../libookala/libookala.la
libWsLut_la_SOURCES = \
	WsLut.cpp         \
	WsLut.h

