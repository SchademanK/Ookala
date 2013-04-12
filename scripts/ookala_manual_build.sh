#!/bin/sh
#
# Ookala Modular Calibration Framework 
#
# Simple build script - adapt to requirements (currently no error checking)
#    + this sample currently configured for RHEL4.6, x86_64, with Chroma5
#
# Script assumes that wxWidgets was configured with something like
#    ./configure --libdir=/usr/local/$LIB ... 
# to ensure compatible library locations with this script, e.g., lib64 or 
# not.

# -------------------------------------------------------------------------

# Base directory for Ookala -----------------------------------------------
BASEDIR=/usr/local/ookala

# Define for lib or lib64 -------------------------------------------------
LIB=lib64
LIBDIR=$BASEDIR/$LIB

# Configure flags ---------------------------------------------------------
TARGETFLAGS="--prefix=$BASEDIR --libdir=$LIBDIR"
COMPILELOG=/tmp/ookala_build.out

# -------------------------------------------------------------------------
# Alternate sensor CFGFLAGS if provided

CHROMA5="--with-chroma5 \
	--with-chroma5-headers=$BASEDIR/include \
	--with-chroma5-libs=$BASEDIR/$LIB"
I1D2HPDC="--with-i1d2hpdc \
	--with-i1d2hpdc-headers=$BASEDIR/include \
	--with-i1d2hpdc-libs=$BASEDIR/$LIB"

# -------------------------------------------------------------------------
# Red Hat Enterprise Linux (RHEL) useful definitions (RHEL4.6, RHEL5.2) 
# Select final flag by defining $CFGFLAG below

CFGFLAGS4="$CHROMA5 \
	--with-examplesensor \
	--with-examplesensor-headers=$BASEDIR/include \
	--with-examplesensor-libs=$BASEDIR/$LIB"

CFGFLAGS5="$CHROMA5 \
	--with-examplesensor \
	--with-examplesensor-headers=$BASEDIR/include \
	--with-examplesensor-libs=$BASEDIR/$LIB \
	--with-wx-gcc4"

CFGFLAGS=$CFGFLAGS4

# Define locations for X11 ------------------------------------------------
X11FLAGS="--x-includes=/usr/include/X11 --x-libraries=/usr/$LIB"

if [ ! -d $BASEDIR ]; then
    echo "Creating target directories for Ookala [$BASEDIR]..."
    mkdir -p $BASEDIR/bin
    mkdir -p $BASEDIR/docs
    mkdir -p $BASEDIR/icons
    mkdir -p $BASEDIR/include
    mkdir -p $BASEDIR/$LIB
    mkdir -p $BASEDIR/scripts
fi

# Possibly needed to fulfill AM_OPTIONS_WXCONFIG --------------------------
# if [ ! -f /usr/share/aclocal/wxwin.m4 -a \
#        -f /usr/local/share/aclocal/wxwin.m4 ]; then
#     echo "Fixing up wxwin.m4 for AM_OPTIONS_WXCONFIG..."
#     ln -s /usr/local/share/aclocal/wxwin.m4 \
#           /usr/share/aclocal/wxwin.m4
# fi

# Prepare to build --------------------------------------------------------
aclocal; autoconf; automake

# Configure ---------------------------------------------------------------
echo "Configuring source..."
echo "OOKALA Configure: " `date` "=========================" > $COMPILELOG 
echo ./configure $TARGETFLAGS $CFGFLAGS $X11FLAGS >> $COMPILELOG
./configure $TARGETFLAGS $CFGFLAGS $X11FLAGS >> $COMPILELOG 2>&1
echo "Done."

# Build -------------------------------------------------------------------
echo "Building source..."
echo "OOKALA Build: " `date` "===========================" >> $COMPILELOG 
make >> $COMPILELOG 2>&1 
echo "Done."

# Install -----------------------------------------------------------------
echo "Installing source..."
echo "OOKALA Install: " `date` "===========================" >> $COMPILELOG 
make install  >> $COMPILELOG 2>&1 

# Copy icons into place
cp -p ./icons/* $BASEDIR/icons
cp -p ./src/apps/ucal/ucal.xml.sample $BASEDIR/docs
cp -p ./README $BASEDIR/docs
cp -p ./scripts/omcf-init $BASEDIR/scripts

echo "Done."

# Post-install ------------------------------------------------------------
echo "Post-install of source..."
echo "OOKALA Post-Install: " `date` "======================" >> $COMPILELOG 
ldconfig $LIBDIR

# Check in this cache, too - where wxWidgets is usually installed
ldconfig /usr/local/$LIB


# Confirmation ------------------------------------------------------------
echo "Build completed:" `date` 
echo "Installed at:" $BASEDIR

exit 0

