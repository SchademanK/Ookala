
## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects
AM_CPPFLAGS = @LIBXML2_CFLAGS@              \
           @K10A_CFLAGS@              \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      \
	   -Wno-unused-result


AM_LDFLAGS  = @K10A_LIBS@ -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@

if K10A_ENABLED
lib_LTLIBRARIES = libK10A.la
else
lib_LTLIBRARIES = 
endif

libK10A_la_LDFLAGS = -module
libK10A_la_LIBADD   = ../../libookala/libookala.la
libK10A_la_SOURCES = \
	K10A.cpp		\
	K10A.h			\
	../../kclmtr/KClmtr.cpp		\
	../../kclmtr/KClmtr.h		\
	../../kclmtr/serialPort.cpp	\
	../../kclmtr/serialPort.h	\
	../../kclmtr/Flicker.cpp	\
	../../kclmtr/Flicker.h		\
	../../kclmtr/Measurement.cpp	\
	../../kclmtr/Measurement.h	\
	../../kclmtr/matrix.h		\
	../../kclmtr/ErrorCodes.h	
	



