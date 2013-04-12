
## Process this file with automake to produce Makefile.in

INCLUDES = @LIBXML2_CFLAGS@              \
           @EXAMPLE_SENSOR_CFLAGS@              \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  = @EXAMPLE_SENSOR_LIBS@ -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@

if EXAMPLE_SENSOR_ENABLED
lib_LTLIBRARIES = libExampleSensor.la
else
lib_LTLIBRARIES = 
endif

libExampleSensor_la_LDFLAGS = -module
libExampleSensor_la_LIBADD   = ../../libookala/libookala.la
libExampleSensor_la_SOURCES = \
	ExampleSensor.cpp \
	ExampleSensor.h



