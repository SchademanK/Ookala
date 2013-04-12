
## Process this file with automake to produce Makefile.in

INCLUDES = @LIBXML2_CFLAGS@              \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  =  -L$(top_srcdir)/src/libookala 
LIBS        = 

lib_LTLIBRARIES = libDevI2c.la

libDevI2c_la_LDFLAGS = -module
libDevI2c_la_LIBADD   = ../../libookala/libookala.la
libDevI2c_la_SOURCES = \
	DevI2c.cpp \
	DevI2c.h

