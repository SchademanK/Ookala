
## Process this file with automake to produce Makefile.in

INCLUDES = @LIBXML2_CFLAGS@              \
           @WX_CXXFLAGS@                 \
           @X_CFLAGS@                    \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  = -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@ @WX_LIBS@ 

lib_LTLIBRARIES = libWxSensorPrep.la


libWxSensorPrep_la_LDFLAGS = -module
libWxSensorPrep_la_LIBADD   = ../../libookala/libookala.la
libWxSensorPrep_la_SOURCES = \
	WxSensorPrep.cpp \
	WxSensorPrep.h

