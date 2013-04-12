
## Process this file with automake to produce Makefile.in

INCLUDES = @LIBXML2_CFLAGS@              \
           @WX_CXXFLAGS@                 \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src


LDADD    = @LIBXML2_LIBS@   \
           @WX_LIBS@        \
           -L$(top_srcdir)/src/libookala -lookala -ldl 

bin_PROGRAMS = hpdc_util 

hpdc_util_SOURCES =   \
   main.cpp  
