README
===================

Ookala Modular Calibration Framework

Copyright (c) 2008 Hewlett-Packard Development Company, L.P.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

---------------------------------------------------------------------------

### Ookala Modular Calibration Framework [OMCF]

Directory navigation is as follows:
```
/ookala-mcf-VER
  |
  +-- configure.in                 # source to configure
  |                                # aclocal; autoconf; automake;
  |
  +-- /doc
  |   +-- OMCF-Overview-VER.pdf    # Technical reference overview
  |   +-- omcf_arch.pdf            # Internal OMCF architecture
  |
  +-- /icons
  |
  +-- /rpm
  |   +-- ookala-mcf.spec          # RPM packaging script
  |
  +-- /scripts
  |   +-- omcf-init # i2c-dev init script
  |   +-- ookala_manual_build.sh   # Simple manual build script
  |
  +-- /src
      +-- /apps
      |   +-- /hpdc_util           # HP DreamColor cmd line dev tool
      |   +-- /read_sensor         # libookala direct sensor example
      |   +-- /ucal                # OMCF color calibration app
      |       +-- ucal.xml.sample  #   configuration/runtime template
      |
      +-- /docs
      |   +-- omcf_arch            # LaTex source
      |   +-- ...
      |
      +-- /libookala               # Plugin, Dict, Registry, Sensor,
      |                            #   DDC/CI, DataSavior, Types
      |
      +-- /kclmtr
      |   				     # Object to talk to the K10/8/1
      +-- /plugins
          +-- ExampleSensor       # Color Sensor plugin example
          +-- Chroma5             # X-Rite Chroma5 sensor example
          +-- DreamColor          # HP DreamColor Display interface
          +-- DevI2c              # Basic I2C device interface
          +-- WsLut               # Workstation LUT modules
          |
          +-- WxBasicGui          # wxWidgets required
          +-- WxDreamColorCalib   # wxWidgets required
          +-- WxLutSweep          # wxWidgets required
          +-- WxSensorPrep        # wxWidgets required
```
## Packages needed
```bash
automake
build-essential
libwxgtk2.8-dev
libusb-dev
libxml2-dev
libxxf86vm-dev
freeglut3-dev
libsm-dev
libtool
```

----------

## pre-compile for K10
```
mv src/apps/read_sensor/main.cpp src/apps/read_sensor/main.cpp_ori
mv src/apps/read_sensor/main.cpp.K10A to src/apps/read_sensor/main.cpp
```

----------

##compile for K10
```bash
aclocal
autoconf
automake
autoreconf -i
./configure --prefix=/usr/local/ookala --libdir=/usr/local/ookala/lib[64] --with-k10a #64 depending on your system
make
```

----------

##post compile for K10
```bash
make install
cp -p ./icons/* /usr/local/ookala/icons
cp -p ./src/apps/ucal/ucal.xml.sample /usr/local/ookala/docs
cp -p ./README /usr/local/ookala/docs
cp -p ./scripts/omcf-init /usr/local/ookala/scripts
ldconfig /usr/local/lib
ldconfig /usr/local/ookala/lib[64]  #depending your your system
```
----------

## setting up the program
#### System wide settings
```bash
cp /usr/local/ookala/docs/ucal.xml.sample /etc/xdg/ucal/ucal.xml
$HOME/.config/ucal/ucal.xml for personalize
```

#### user settings
```bash
cp /usr/local/ookala/docs/ucal.xml.sample $HOME/.config/ucal/ucal.xml
```
### Changing the Settings
in the ucal.xml change these settings for the K10
Find
```bash
  <loadplugin>/usr/local/ookala/lib64/libMySENSOR.so</loadplugin>
```
replace
```bash
  <loadplugin>/usr/local/ookala/lib64/libK10A.so</loadplugin>
```
<br>
Find (should be two of them)
```bash
<dictitem type="string" name="MySENSOR::displayType">lcd</dictitem>
<dictitem type="double" name="MySENSOR::integrationTime">3.0</dictitem>
<dictitem type="int"    name="MySENSOR::calibrationIdx">1</dictitem>
```
replace
```bash
<dictitem type="string" name="K10A::port">/dev/ttyUSB0</dictitem>     #Change it to port is ours
<dictitem type="int" name="K10A::measurements">8</dictitem>           #Change how many number of measurements to average with
<dictitem type="int"    name="K10A::caliFileID">1</dictitem>          #Change the calfile the K10 needs to use
```

Find (should be two of them)
```bash
<plugin>MySENSOR</plugin>
```
Replace
```bash
<plugin>K10A</plugin>
```

## Run for K10
1. Plug in the Dreamcolor with DVI
2. run /usr/local/ookala/bin/ucal in a UI environment
3. an Icon show on the status bar

## TODO
Better README
Remove all post compile scripts
add a compile script for the K10
Fix a out of range error from K10

## Might also need
* i2c-dev
* install FTDI Drivers
* change permissions on the port