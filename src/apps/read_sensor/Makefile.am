
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @LIBXML2_CFLAGS@              \
	   @SENSOR_CFLAGS@            \
           @WX_CXXFLAGS@                 \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src

LDADD    = @LIBXML2_LIBS@   \
           @SENSOR_LIBS@ \
           @WX_LIBS@        \
           -L$(top_srcdir)/src/libookala -lookala -ldl 

LIBS = @LIBXML2_LIBS@

bin_PROGRAMS = read_sensor

read_sensor_SOURCES =   \
   main.cpp  
