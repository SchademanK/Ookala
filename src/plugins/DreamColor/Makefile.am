
## Process this file with automake to produce Makefile.in

INCLUDES = @LIBXML2_CFLAGS@              \
           -I$(top_srcdir)/src/libookala \
           -I$(top_srcdir)/src      


AM_LDFLAGS  =  -L$(top_srcdir)/src/libookala 
LIBS        = @LIBXML2_LIBS@ 

lib_LTLIBRARIES = libDreamColor.la


libDreamColor_la_LDFLAGS = -module
libDreamColor_la_LIBADD   = ../../libookala/libookala.la
libDreamColor_la_SOURCES =    \
	DreamColorCalib.cpp            \
	DreamColorCalib.h              \
	DreamColorCalibRecord.cpp      \
	DreamColorCalibRecord.h        \
	DreamColorCalibrationData.cpp  \
	DreamColorCalibrationData.h    \
	DreamColorCtrl.cpp             \
	DreamColorCtrl.h               \
	DreamColorSpaceInfo.cpp        \
	DreamColorSpaceInfo.h    



