
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @LIBXML2_CFLAGS@              \
           @i1D2_HPDC_CFLAGS@              \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  = @i1D2_HPDC_LIBS@ -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@

if i1D2_HPDC_ENABLED
lib_LTLIBRARIES = libi1D2HPDC.la
else
lib_LTLIBRARIES = 
endif

libi1D2HPDC_la_LDFLAGS = -module
libi1D2HPDC_la_LIBADD   = ../../libookala/libookala.la
libi1D2HPDC_la_SOURCES = \
	i1D2HPDC.cpp \
	i1D2HPDC.h



