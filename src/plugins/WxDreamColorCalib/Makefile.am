
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @LIBXML2_CFLAGS@              \
           @WX_CXXFLAGS@                 \
           @X_CFLAGS@                    \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  = -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@ @WX_LIBS@ 

lib_LTLIBRARIES = libWxDreamColorCalib.la


libWxDreamColorCalib_la_LDFLAGS = -module
libWxDreamColorCalib_la_LIBADD   = ../../libookala/libookala.la
libWxDreamColorCalib_la_SOURCES = \
	WxDreamColorCalib.cpp \
	WxDreamColorCalib.h

