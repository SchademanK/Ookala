
## Process this file with automake to produce Makefile.in

INCLUDES = @LIBXML2_CFLAGS@              \
           @WX_CXXFLAGS@                 \
           @X_CFLAGS@                    \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  = -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@ @WX_LIBS@ 

lib_LTLIBRARIES = libWxLutSweep.la


libWxLutSweep_la_LDFLAGS = -module
libWxLutSweep_la_LIBADD   = ../../libookala/libookala.la
libWxLutSweep_la_SOURCES = \
	WxLutSweep.cpp         \
	WxLutSweep.h

