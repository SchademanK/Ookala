
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @LIBXML2_CFLAGS@           \
           @WX_CXXFLAGS@                 \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src           

LDADD    = @LIBXML2_LIBS@   \
           @WX_LIBS@        \
	   -lSM -lICE -lX11 \
           -L$(top_srcdir)/src/libookala -lookala

bin_PROGRAMS = ucal

ucal_SOURCES =       \
   CalibChecker.cpp  \
   CalibChecker.h    \
   Event.h           \
   main.cpp          \
   SessionManage.cpp \
   SessionManage.h   \
   SignalHandler.cpp \
   SignalHandler.h   \
   WxIconCtrl.cpp    \
   WxIconCtrl.h     
