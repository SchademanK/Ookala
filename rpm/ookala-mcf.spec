#
# $Id: ookala-mcf.spec 135 2008-12-19 00:49:58Z omcf $
#
# RPM-based sample for enterprise deployment of Ookala-MCF

Summary: OMCF - an open color-calibration framework for displays
Name: ookala-mcf
Version: 0.7
Release: 1%{?dist}
#URL: http://

Vendor: Hewlett-Packard Company
License: HP, MIT/X11  
Group: System Environment/Libraries
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# May wish to disable dependency checking on install if support
# packages are manually installed for development
# AutoReqProv: no

Source: ookala-mcf-%{version}.tar.gz

BuildRequires: automake
BuildRequires: autoconf
BuildRequires: libtool
BuildRequires: pkgconfig

%description
Ookala Modular Calibration Framework [OMCF] is an open 
color-calibration platform designed to support extensible, 
standardized color calibration of color-critical display 
environments such as HP DreamColor Professional Displays.

The modular architecture allows for easy customization of 
a variety of display output and color sensor scenarios.

Copyright (c) 2008 Hewlett-Packard Development Company, LP.

# OMCF PLUGIN/CONFIG FLAGS ------------------------------------------

# Recommended installation location
%define	_ookaladir	/usr/local/ookala

# Disable debug build
%define debug_package   %{nil}

# Chroma5 color sensor by X-Rite (requires licensed SDK)
%define chroma5_flags --with-chroma5 --with-chroma5-headers=%{_ookaladir}/include --with-chroma5-libs=%{_ookaladir}/%{_lib} 

# HP DreamColor color sensor by X-Rite (requires licensed SDK)
%define i1d2hpdc_flags --with-i1d2hpdc --with-i1d2hpdc-headers=%{_ookaladir}/include --with-i1d2hpdc-libs=%{_ookaladir}/%{_lib}

# For g++ 4.x and wxWidgets - needs -fno-strict-aliasing
%define wx_gcc4_flag --with-wx-gcc4

# OMCF PRE ----------------------------------------------------------
%pre

# OMCF PREUN --------------------------------------------------------
%preun

# OMCF PREP ---------------------------------------------------------
%prep
%setup -q -n %name-%version


# OMCF BUILD --------------------------------------------------------
%build

aclocal ; automake ; autoreconf -fi

# Modify to match your sensor/runtime environment (including
# include and library locations - see examplesensor as default)
#
# Insert any/all of following plugin flag definitions as desired
# surrounded by %{..} in line below with-examplesensor-libs
# (note backslash for line continuation):
#     chroma5_flags 
#     i1d2hpdc_flags
#     wx_gcc4_flag
 
./configure \
	--bindir=%{_ookaladir}/bin \
	--libdir=%{_ookaladir}/%{_lib} \
	--with-examplesensor \
	--with-examplesensor-headers=%{_ookaladir}/include \
	--with-examplesensor-libs=%{_ookaladir}/%{_lib} \
	%{wx_gcc4_flag} \
	${CONFIGURE}

# build it
/usr/bin/make

# OMCF INSTALL ------------------------------------------------------
%install
if [ -e $RPM_BUILD_ROOT ]; then
    /bin/chmod -R +w $RPM_BUILD_ROOT
    /bin/rm -rf $RPM_BUILD_ROOT
fi
/bin/mkdir -p $RPM_BUILD_ROOT%{_ookaladir}/bin
/bin/mkdir -p $RPM_BUILD_ROOT%{_ookaladir}/docs
/bin/mkdir -p $RPM_BUILD_ROOT%{_ookaladir}/icons
/bin/mkdir -p $RPM_BUILD_ROOT%{_ookaladir}/include
/bin/mkdir -p $RPM_BUILD_ROOT%{_ookaladir}/%{_lib}
/bin/mkdir -p $RPM_BUILD_ROOT%{_ookaladir}/scripts

make install DESTDIR=$RPM_BUILD_ROOT

/bin/cp -p ./icons/* $RPM_BUILD_ROOT%{_ookaladir}/icons
/bin/cp -p ./src/apps/ucal/ucal.xml.sample $RPM_BUILD_ROOT%{_ookaladir}/docs
/bin/cp -p ./README $RPM_BUILD_ROOT%{_ookaladir}/docs


# On RHEL systems, this script is enabled with:
#    /bin/cp ./scripts/omcf-init /etc/init.d/omcf-init
#    /sbin/chkconfig --add omcf-init

/bin/cp -p ./scripts/omcf-init $RPM_BUILD_ROOT%{_ookaladir}/scripts

# OMCF CLEAN --------------------------------------------------------
%clean
if [ -e $RPM_BUILD_ROOT ]; then
    /bin/chmod -R +w $RPM_BUILD_ROOT
    /bin/rm -rf $RPM_BUILD_ROOT
fi

# OMCF FILES --------------------------------------------------------
%files
%defattr(755,root,root)
%{_ookaladir}/bin/hpdc_util
%{_ookaladir}/bin/read_sensor
%{_ookaladir}/bin/ucal
%{_ookaladir}/scripts/omcf-init
%defattr(555,root,root)
%{_ookaladir}/docs/README
%{_ookaladir}/docs/ucal.xml.sample
%{_ookaladir}/icons/709_*
%{_ookaladir}/icons/check-mon-*
%{_ookaladir}/icons/default_icon.xpm
%{_ookaladir}/include
%{_ookaladir}/%{_lib}/libDevI2c.*
%{_ookaladir}/%{_lib}/libDreamColor.*
%{_ookaladir}/%{_lib}/libExampleSensor.*
%{_ookaladir}/%{_lib}/libWsLut.*
%{_ookaladir}/%{_lib}/libWxBasicGui.*
%{_ookaladir}/%{_lib}/libWxDreamColorCalib.*
%{_ookaladir}/%{_lib}/libWxLutSweep.*
%{_ookaladir}/%{_lib}/libWxSensorPrep.*
%{_ookaladir}/%{_lib}/libookala.*

# Configure if support libs and plugins are available
#%{_ookaladir}/%{_lib}/libChroma5.*
#%{_ookaladir}/%{_lib}/libi1D2HPDC.*

# OMCF POST ---------------------------------------------------------
%post
/sbin/ldconfig %{_ookaladir}/%{_lib}

# OMCF POSTUN -------------------------------------------------------
%postun
/sbin/ldconfig 

# OMCF CHANGELOG ----------------------------------------------------
%changelog
* Fri Sep 26 2008 - HP - 0.6-1
- Initial spec file example for Ookala Modular Calibration Framework


