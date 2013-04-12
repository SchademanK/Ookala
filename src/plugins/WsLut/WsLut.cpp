// --------------------------------------------------------------------------
// $Id: WsLut.cpp 135 2008-12-19 00:49:58Z omcf $
// --------------------------------------------------------------------------
// Copyright (c) 2008 Hewlett-Packard Development Company, L.P.
// 
// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
// OTHER DEALINGS IN THE SOFTWARE.
// --------------------------------------------------------------------------

#ifdef _WIN32
#pragma warning(disable: 4786)
#endif


#include <stdio.h>
#include <math.h>

#include <string>
#include <algorithm>

#ifdef __linux__
// Linux/XF86 specific includes
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h> 
#endif

#ifdef _WIN32
#include <windows.h>
#define COMPILE_MULTIMON_STUBS
#include <MultiMon.h>
#endif

#include "Types.h"
#include "PluginRegistry.h"
#include "PluginChain.h"
#include "Dict.h"
#include "DictHash.h"
#include "WsLut.h"

//
// Simple interface for handing window-system lookup tables. These 
// are host side, not monitor side.
//
// 

// --------------------------------------

BEGIN_PLUGIN_REGISTER(1)
    PLUGIN_REGISTER(0, Ookala::WsLut)
END_PLUGIN_REGISTER

// --------------------------------------

Ookala::WsLut::WsLut():
    Plugin(),
    mLutSize(0),
    mLutMax(0)
{
    setName("WsLut");
}

// --------------------------------------
//
Ookala::WsLut::WsLut(const WsLut &src):
    Plugin(src)
{
    mLutSize = src.mLutSize;
    mLutMax  = src.mLutMax;       
}

// --------------------------------------
//
// virtual
Ookala::WsLut::~WsLut()
{
}


// ----------------------------
//
Ookala::WsLut &
Ookala::WsLut::operator=(const WsLut &src)
{
    if (this != &src) {
        Plugin::operator=(src);

        mLutSize = src.mLutSize;
        mLutMax  = src.mLutMax;
    }

    return *this;
}

// --------------------------------------
//
// virtual
std::vector<uint32_t>
Ookala::WsLut::luts()
{
    std::vector<uint32_t> keys;

    keys.push_back(1);

    return keys;
}

// --------------------------------------
//
// virtual
uint32_t
Ookala::WsLut::getSize(const uint32_t lutId /* = 0 */)
{
    return mLutSize;
}

// --------------------------------------
//
// virtual
uint32_t
Ookala::WsLut::getMax(const uint32_t lutId /* = 0 */)
{
    return mLutMax;
}

// --------------------------------------
//
// virtual
bool
Ookala::WsLut::set(
           const std::vector<uint32_t> &redLut,
           const std::vector<uint32_t> &greenLut,
           const std::vector<uint32_t> &blueLut,
           const uint32_t               lutId /* = 0 */)
{
    setErrorString("");

    if ((redLut.size() != getSize()) ||
        (greenLut.size() != getSize()) ||
        (blueLut.size() != getSize())) {
        setErrorString("Invalid Lut size.");
        return false;
    }
    
    return _wsSetLut(redLut, greenLut, blueLut);
}

// --------------------------------------
//
// virtual
bool
Ookala::WsLut::set(
           const std::vector<double> &redLut,
           const std::vector<double> &greenLut,
           const std::vector<double> &blueLut,
           const uint32_t             lutId /* = 0 */)
{
    std::vector<uint32_t> redIntLut, greenIntLut, blueIntLut;
    std::vector<double>::const_iterator theDoubleVal;

    setErrorString("");

    if ((redLut.size() != getSize()) ||
        (greenLut.size() != getSize()) ||
        (blueLut.size() != getSize())) {
        setErrorString("Invalid Lut size.");
        return false;
    }
    
    for (theDoubleVal = redLut.begin();
                theDoubleVal != redLut.end(); ++theDoubleVal) {
        double val = (*theDoubleVal);

        if (val < 0) val = 0;
        if (val > 1) val = 1;

        redIntLut.push_back(
            static_cast<uint32_t>(val * mLutMax + 0.5));
    }

    for (theDoubleVal = greenLut.begin();
                theDoubleVal != greenLut.end(); ++theDoubleVal) {
        double val = (*theDoubleVal);

        if (val < 0) val = 0;
        if (val > 1) val = 1;

        greenIntLut.push_back(
            static_cast<uint32_t>(val * mLutMax + 0.5));
    }


    for (theDoubleVal = blueLut.begin();
                theDoubleVal != blueLut.end(); ++theDoubleVal) {
        double val = (*theDoubleVal);

        if (val < 0) val = 0;
        if (val > 1) val = 1;

        blueIntLut.push_back(
            static_cast<uint32_t>(val * mLutMax + 0.5));
    }

    return _wsSetLut(redIntLut, greenIntLut, blueIntLut);
}

// --------------------------------------
//
// virtual
bool
Ookala::WsLut::get(
           std::vector<uint32_t> &redLut,
           std::vector<uint32_t> &greenLut,
           std::vector<uint32_t> &blueLut,
           const uint32_t         lutId /* = 0 */)
{
    setErrorString("");

    redLut.clear();
    greenLut.clear();
    blueLut.clear();
    
    return _wsGetLut(redLut, greenLut, blueLut);
}

// --------------------------------------
//
// virtual
bool
Ookala::WsLut::get(
           std::vector<double> &redLut,
           std::vector<double> &greenLut,
           std::vector<double> &blueLut,
           const uint32_t       lutId /* = 0 */)
{
    std::vector<uint32_t> redIntLut, greenIntLut, blueIntLut;
    std::vector<uint32_t>::iterator theVal;

    setErrorString("");

    redLut.clear();
    greenLut.clear();
    blueLut.clear();
    
    if (!_wsGetLut(redIntLut, greenIntLut, blueIntLut)) {
        return false;
    }

    for (theVal= redIntLut.begin();
                theVal != redIntLut.end(); ++theVal) {
        redLut.push_back(static_cast<double>(*theVal) / 
                         static_cast<double>(mLutMax));
    }
        
    for (theVal = greenIntLut.begin();
                theVal != greenIntLut.end(); ++theVal) {
        greenLut.push_back(static_cast<double>(*theVal) / 
                         static_cast<double>(mLutMax));
    }

    for (theVal = blueIntLut.begin();
                theVal != blueIntLut.end(); ++theVal) {
        blueLut.push_back(static_cast<double>(*theVal) / 
                         static_cast<double>(mLutMax));
    }

    return true;
}

// --------------------------------------
//
// virtual
bool 
Ookala::WsLut::reset(const uint32_t lutId /* = 0 */)
{
    std::vector<double> redLut, greenLut, blueLut;

    for (uint32_t i=0; i<mLutSize; ++i) {
        
        double val = static_cast<double>(i)/
                     static_cast<double>(mLutSize-1);

        redLut.push_back(val);
        greenLut.push_back(val);
        blueLut.push_back(val);
    }
    
    return set(redLut, greenLut, blueLut, lutId);
}



// --------------------------------------
//
// virtual
bool
Ookala::WsLut::randomize(const uint32_t lutId /* = 0 */)
{
    std::vector<double> redLut, greenLut, blueLut;

    for (uint32_t i=0; i<mLutSize; ++i) {
        
        double val = static_cast<double>(i)/
                     static_cast<double>(mLutSize-1);

        redLut.push_back(val);
        greenLut.push_back(val);
        blueLut.push_back(val);
    }
    
    std::random_shuffle(redLut.begin(),   redLut.end());
    std::random_shuffle(greenLut.begin(), greenLut.end());
    std::random_shuffle(blueLut.begin(),  blueLut.end());

    return set(redLut, greenLut, blueLut, lutId);
}

// --------------------------------------
//
// virtual protected
bool
Ookala::WsLut::_wsSetLut(
                 const std::vector<uint32_t> &redLut,
                 const std::vector<uint32_t> &greenLut,
                 const std::vector<uint32_t> &blueLut)
{
    bool     debug = true;


    if ((redLut.size()   != mLutSize) ||
        (greenLut.size() != mLutSize) ||
        (blueLut.size()  != mLutSize)) {
        setErrorString("Lut data is wrong size.");
        return false;
    }

#ifdef __linux__

    //----------------------------
    // XF-86 LUT loading
    Display *dpy;
    int ver, err, len;
    unsigned short *ramp[3];
 
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsSetLut() Error: Can't open display %s\n", XDisplayName(NULL));
        }   
        setErrorString("Can't open display");
        return false;
    }

    if (XF86VidModeQueryExtension(dpy, &ver, &err) == False) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsSetLut() Error: no LUT support\n");
        }
        setErrorString("No LUT support");
        XCloseDisplay(dpy);
        return false;
    }

    if (XF86VidModeGetGammaRampSize(dpy, 0, &len) == False) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsSetLut() Error: Can't retrieve LUT size\n");
        }
        setErrorString("Can't retrieve LUT size");
        XCloseDisplay(dpy);
        return false;
    }

    if (len != (int)mLutSize) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsSetLut() Error: LUT size mismatch\n");
        }
        setErrorString("LUT size mismatch");
        XCloseDisplay(dpy);
        return false;
    }
    
    for (int i=0; i<3; i++) {
        ramp[i] = new unsigned short[mLutSize];
    }

    for (uint32_t i=0; i<mLutSize; i++) {
        ramp[0][i] = static_cast<unsigned short>(redLut[i]);
        ramp[1][i] = static_cast<unsigned short>(greenLut[i]);
        ramp[2][i] = static_cast<unsigned short>(blueLut[i]);
    }

    if (XF86VidModeSetGammaRamp(dpy, 0, mLutSize, ramp[0], ramp[1], ramp[2]) 
            == False) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsSetLut() Error: Can't set LUT\n");
        }
        setErrorString("Can't set LUT");
        XCloseDisplay(dpy);
        return false;
    }

    for (int i=0; i<3; i++) {
        delete[] ramp[i];
    }

    XCloseDisplay(dpy);
    return true;
    //----------------------------

#elif defined WIN32
    
    HDC       dc;
    POINT     pnt;
    TCHAR     errMsg[1024]; 
    WORD      oldLuts[768], luts[768];
    uint32_t  idx;

    printf("_WsSetLut\n");

    pnt.x = pnt.y = 0;
    HMONITOR mon = MonitorFromPoint( pnt, MONITOR_DEFAULTTONULL);   
    if (mon == NULL) {
        setErrorString("Unable to get a monitor.");
        return false;
    }

    MONITORINFOEX info; 
    info.cbSize = sizeof(info);
    if (!GetMonitorInfo(mon, &info)) {      
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)errMsg, 1023, NULL);

        setErrorString( std::string("Unable to get monitor info.") + 
                                std::string(errMsg));
        fprintf(stderr, "%s\n", errorString().c_str());        
    }

    
    fprintf(stderr, "%d\n", info.cbSize);
    fprintf(stderr, "%d %d %d %d\n", info.rcMonitor.left, info.rcMonitor.right, info.rcMonitor.top, info.rcMonitor.bottom);
    fprintf(stderr, "%d %d %d %d\n", info.rcWork.left, info.rcWork.right, info.rcWork.top, info.rcWork.bottom);
    fprintf(stderr, "%d\n", info.dwFlags);
    fprintf(stderr, "%s\n", info.szDevice);

    dc = CreateDC(TEXT("DISPLAY"), info.szDevice, NULL, NULL);
    //dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    if (dc == NULL) {       
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)errMsg, 1023, NULL);  
        setErrorString( std::string("Unable to get DC for LUT. GetDC() failed with ") + 
                                std::string(errMsg));
        fprintf(stderr, "%s\n", errorString().c_str());
        return false;
    }
    
    if ((int)mLutSize != 256) {
        setErrorString("LUT size mismatch");
        ReleaseDC(NULL, dc);
        return false;
    }

    
    for (idx=0; idx<256; ++idx) {
        luts[idx]     = redLut[idx];
        luts[256+idx] = greenLut[idx];
        luts[512+idx] = blueLut[idx];
    }

    for (idx=0; idx<768; ++idx) {
        if (luts[idx] > 255) luts[idx] = 255;
        if (luts[idx] <   0) luts[idx] =   0;
    }

    
    for (int i=0; i<256; ++i) {
        luts[i]     = (luts[i] << 8) | luts[i];
        luts[256+i] = (luts[256+i] << 8) | luts[256+i];
        luts[512+i] = (luts[512+i] << 8) | luts[512+i];
    }   

    // Windows has draconian restrictions that keep you from
    // doing what you really want.. Like making random
    // LUTs.
    // XXX: ??

    for (int i=0; i<128; ++i) {
        if (luts[i]     > (128+i) << 8) luts[i]     = (128+i)<<8;
        if (luts[256+i] > (128+i) << 8) luts[256+i] = (128+i)<<8;
        if (luts[512+i] > (128+i) << 8) luts[512+i] = (128+i)<<8;

        if (luts[127] > 254 << 8)     luts[127]     = 254<<8;
        if (luts[127+256] > 254 << 8) luts[127+256] = 254<<8;
        if (luts[127+512] > 254 << 8) luts[127+512] = 254<<8;
    }

    for (int i=1; i<256; ++i) {
        if (luts[i]     < luts[i-1])     luts[i]     = luts[i-1];
        if (luts[i+256] < luts[i-1+256]) luts[i+256] = luts[i-1+256];
        if (luts[i+512] < luts[i-1+512]) luts[i+512] = luts[i-1+512];
    }

    if (!SetDeviceGammaRamp(dc, luts)) {        
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)errMsg, 1023, NULL);      
        setErrorString( 
            std::string("Unable to set video card LUTs. SetDeviceGammaRamp failed with  ") + 
                                std::string(errMsg));
        fprintf(stderr, "Set was bad: %s\n", errorString().c_str());
        ReleaseDC(NULL, dc);
        return false;
    } else {
        printf("Set was ok\n");
    }
        
    ReleaseDC(NULL, dc);

    return true;

#else
    if (debug) {
        fprintf(stderr, "WSLut::_wsSetLut() Error: No OS support for LUTs\n");
    }
    setErrorString("No OS LUT support");
    return false;
#endif
}

// -----------------------------------
//+
// This should set mLutSize and mLutMax when we query the data.
//
// virtual protected
bool
Ookala::WsLut::_wsGetLut(
                 std::vector<uint32_t> &redLut,
                 std::vector<uint32_t> &greenLut,
                 std::vector<uint32_t> &blueLut)
{
    bool debug = true;

    redLut.clear();
    greenLut.clear();
    blueLut.clear();

#ifdef __linux__

    //----------------------------
    // XF-86 LUT loading
    Display *dpy;
    int ver, err, len;
    unsigned short *ramp[3];
 
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsGetLut() Error: Can't open display %s\n", XDisplayName(NULL));
        }   
        setErrorString("Can't open display.");
        return false;
    }

    if (XF86VidModeQueryExtension(dpy, &ver, &err) == False) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsGetLut() Error: no LUT support\n");
        }
        setErrorString("No LUT support.");
        XCloseDisplay(dpy);
        return false;
    }

    if (XF86VidModeGetGammaRampSize(dpy, 0, &len) == False) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsGetLut() Error: Can't retrieve LUT size\n");
        }
        setErrorString("Can't find LUT size.");
        XCloseDisplay(dpy);
        return false;
    }

    mLutMax = 65535;
    mLutSize = len;
    
    for (int i=0; i<3; i++) {
        ramp[i] = new unsigned short[mLutSize];
    }

    if (XF86VidModeGetGammaRamp(dpy, 0, mLutSize, ramp[0], ramp[1], ramp[2]) 
            == False) {
        if (debug) {
            fprintf(stderr, "WSLut::_wsGetLut() Error: Can't retrieve LUT\n");
        }
        setErrorString("Can't find LUT.");
        XCloseDisplay(dpy);
        return false;
    }

    for (uint32_t i=0; i<mLutSize; i++) {
        redLut.push_back(static_cast<uint32_t>(ramp[0][i]));
        greenLut.push_back(static_cast<uint32_t>(ramp[1][i]));
        blueLut.push_back(static_cast<uint32_t>(ramp[2][i]));
    }

    for (int i=0; i<3; i++) {
        delete[] ramp[i];
    }

    XCloseDisplay(dpy);
    return true;
    //----------------------------

#elif defined _WIN32

    mLutMax = 255;
    mLutSize = 256;

    HDC    dc = GetDC(NULL);
    TCHAR  errMsg[1024];    
    WORD luts[768];
    uint32_t idx;

    if (dc == NULL) {       
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)errMsg, 1023, NULL);

        setErrorString( std::string("Unable to get DC for LUT. GetDC() failed with ") + 
                            std::string(errMsg));       
        return false;
    }


    if (!GetDeviceGammaRamp(dc, luts)) {    
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)errMsg, 1023, NULL);      

        setErrorString( std::string(
                "Unable to get video card LUTs. GetDeviceGammaRamp() failed with ") + 
                            std::string(errMsg));       

        ReleaseDC(NULL, dc);
        return false;
    }

    for (idx=0; idx<256; ++idx) {
        redLut.push_back(luts[idx]>>8);
        greenLut.push_back(luts[256+idx]>>8);
        blueLut.push_back(luts[512+idx]>>8);
    }

    ReleaseDC(NULL, dc);
    return true;

#else
    if (debug) {
        fprintf(stderr, "WSLut::_wsGetLut() Error: No OS support for LUTs\n");
    } 
    setErrorString("No OS LUT support.");
    return false;
#endif
}

// -----------------------------------
//
// virtual protected
bool
Ookala::WsLut::_postLoad()
{
    std::vector<uint32_t> redLut, greenLut, blueLut;

    return _wsGetLut(redLut, greenLut, blueLut);
}


// -----------------------------------
//
// virtual protected
bool
Ookala::WsLut::_run(PluginChain *chain)
{
    DictItem              *item;
    BoolDictItem          *boolItem;
    DoubleDictItem        *doubleItem;
    Dict                  *dict = NULL;
    DictHash              *hash = NULL;
    std::vector<Plugin *>  plugins;

    printf("WsLut::_run()\n");

    if (mRegistry == NULL) {
        setErrorString("No registry in WsLut plugin.");
        return false;
    }

    printf("Going to look for DictHash\n");
    printf("Registry has %d plugins\n", mRegistry->numPlugins());

    plugins = mRegistry->queryByName("DictHash");
    printf("Success\n");
    if (plugins.empty()) {
        return false;
    }

    printf("Going to try dynamic cast\n");
    hash = dynamic_cast<DictHash *>(plugins[0]);
    printf("back\n");
    if (!hash) {
        return false;
    }
    printf("ok\n");

    // If we don't find a chain dict, that's not fatal, just
    // ignore it and do nothing.
    printf("Trying to getDict\n");

    std::string name = chain->getDictName();

    printf("dictName: %s\n", name.c_str());

    dict = hash->getDict(name.c_str());
    if (!dict) {
        return true;
    }

printf("WsLut::_run(), found dict %s\n", chain->getDictName().c_str());

    // Look in the chain dict for:
    //     WsLut::randomize <bool>
    //     WsLut::reset     <bool>
    //     WsLut::pow       <double>

    printf("looking for WsLut::randomize\n");
    item = dict->get("WsLut::randomize");
    if (item) {
        printf("Found the item\n");
        boolItem = dynamic_cast<BoolDictItem *>(item);
        if (boolItem) {
            if (boolItem->get()) {
                printf("randomize!\n");
                randomize();
            }
        }    
    }

    item = dict->get("WsLut::reset");
    if (item) {
        boolItem = dynamic_cast<BoolDictItem *>(item);
        if (boolItem) {
            if (boolItem->get()) {
                reset();
            }
        }    
    }

    item = dict->get("WsLut::pow");
    if (item) {
        doubleItem = dynamic_cast<DoubleDictItem *>(item);
        if (doubleItem) {

            std::vector<double> lut;
            double              expon = doubleItem->get();
            double              x;

            for (uint32_t i=0; i<mLutSize; ++i) {
                x = static_cast<double>(i) / 
                    static_cast<double>(mLutSize-1);

                lut.push_back(pow(x, expon));
            }

            set(lut, lut, lut);
        }    
    }


    return true;
}


