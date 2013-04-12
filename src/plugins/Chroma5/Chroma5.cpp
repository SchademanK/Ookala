// --------------------------------------------------------------------------
// $Id: Chroma5.cpp 135 2008-12-19 00:49:58Z omcf $
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

// This module requires a direct-licensed library interface from X-Rite, Inc.
// API and symbols are copyright (C) 2004 X-Rite, Inc.
//
// It is provided only for X-Rite licensed users.  All other applications 
// are strictly prohibited.
//
// Published with permission of X-Rite, Inc., October 2008

#include <stdio.h>
#include <stdlib.h>

#ifdef __linux__
#include <unistd.h>
#include <usb.h>
#endif

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Sensor.h"
#include "PluginChain.h"

#include <Chrfuncs.h>
#include "Chroma5.h"


// ----------------------------------

BEGIN_PLUGIN_REGISTER(1)
    PLUGIN_REGISTER(0, Ookala::Chroma5)
END_PLUGIN_REGISTER

// ----------------------------------


// ==================================
//
// Chroma5
//
// ----------------------------------

Ookala::Chroma5::Chroma5():
  Sensor()
{
    setName("Chroma5");

    mDeviceOpen        = false;
    mNextSensorKey     = 1;
    mReadScreenRefresh = false;

    mSIPerror = (long) 0;
}

// ----------------------------------
//

Ookala::Chroma5::Chroma5(const Chroma5  &src):
    Sensor(src)
{
    mDeviceOpen        = src.mDeviceOpen;
    mNextSensorKey     = src.mNextSensorKey;
    mDeviceKey         = src.mDeviceKey;
    mReadScreenRefresh = src.mReadScreenRefresh;
}

// ----------------------------------
//
// virtual
Ookala::Chroma5::~Chroma5()
{
    setErrorString("");

    // If we're left with an open device, close it.
    if (mDeviceOpen) {

        // XXX: Close the device handle.

        mSIPerror = sipCloseCalibrator();

        if (mSIPerror != SUCCESS)
        {
            std::ostringstream oss;
            oss << mSIPerror;

            setErrorString(
                    std::string("Unable to close Chroma5 [SIP error: ") +
                                oss.str() + std::string("]."));
        }

        mDeviceOpen = false;
    }
}

// ----------------------------
//
Ookala::Chroma5 &
Ookala::Chroma5::operator=(const Chroma5 &src)
{
    if (this != &src) {
        Sensor::operator=(src);

        mNextSensorKey     = src.mNextSensorKey;
        mDeviceOpen        = src.mDeviceOpen;
        mDeviceKey         = src.mDeviceKey;
        mReadScreenRefresh = src.mReadScreenRefresh;
    }

    return *this;
}

// ----------------------------------
//
// virtual
std::vector<uint32_t>
Ookala::Chroma5::sensors()
{
    std::vector<uint32_t> devices;
    setErrorString("");

    if (mDeviceOpen) {
        devices.push_back(mDeviceKey);
    }
    
    return devices;
}

// ----------------------------------
//
// virtual
bool
Ookala::Chroma5::measureYxy(Yxy       &value,
                  PluginChain *chain     /* = NULL */,
                  uint32_t     sensorId  /* = 0 */)
{
    DictOptions opts;
    dYxy        G5reading;

    setErrorString("");

    if (!getDictOptions(opts, chain)) {
        return false;
    }

    // Update the calibration, in case someone changed
    // it out from under us.
    
    // XXX: Choose the calibration matrix, based on what's selected
    //      in opts.calibration

    mSIPerror = sipSelectCalibration(
                         static_cast<WHICH_CALIBRATION>(opts.calibration));
    
    if (mSIPerror != SUCCESS)
    {
        setErrorString("Cannot set specified Chroma5 calibration.");
        return false;
    }

    // XXX: Choose the display type (e.g. LCD, CRT, etc) base
    //      on the value in opts.displayType

    // XXX: Choose the integration time based on the value in
    //      opts.integrationTime

    if (opts.displayType == MEASTYPE_LCD)
    {
        mSIPerror = sipSetLCDTargetTime(opts.integrationTime);

        if (mSIPerror != SUCCESS)
        {
            setErrorString("Cannot set specified Chroma5 integration time.");
            return false;
        }
    } 

    // XXX: Finally, read a measurement in cd/m^2 into the value
    //      parameter.

    mSIPerror = sipG5Measure(UNITS_CANDELAS, 
                             static_cast<MEASURE_TYPE>(opts.displayType), 
                             &G5reading);

    if (mSIPerror == WARNING_MEASUREMENT_TIMEOUT)
    {
        setErrorString("Chroma5 measurement timed out.");
        return false;
    } 
    else if (mSIPerror != SUCCESS)
    {
        std::ostringstream oss;
        oss << mSIPerror;

        // Format a string and return
        setErrorString(
            std::string("Chroma5 measurement error [SIP error: ") +
                                oss.str() + std::string("]."));
        return false;
    }

    // Transfer measurement to return value 
    value.Y = (double) G5reading.Y;
    value.x = (double) G5reading.x;
    value.y = (double) G5reading.y;

    if (chain) {
        chain->setUiYxy("Ui::measured_Yxy", value);
    }

    return true;
}

// ----------------------------------
//
// virtual
std::vector<uint32_t> 
Ookala::Chroma5::actionNeeded(
                      PluginChain *chain    /* = NULL */,
                      uint32_t     sensorId /* = 0 */)
{
    std::vector<uint32_t> actions;
    DictOptions           opts;

    setErrorString("");

    if (getDictOptions(opts, chain)) {

        // XXX: See if we need to do anything based on our options. 
        //      For example, if opts.displayType indicates CRT,
        //      we might want to do:
        //          actions.push_back(SENSOR_ACTION_MEASURE_REFRESH).
        //      YMMV.

        if ((opts.displayType == MEASTYPE_CRT) && 
            (mReadScreenRefresh == false))
        {
            // We're a CRT and we have not read refresh rate yet
            actions.push_back(SENSOR_ACTION_MEASURE_REFRESH);
        }
    }

    if (!mDeviceOpen) {
        actions.push_back(SENSOR_ACTION_DETECT);
    }
    
    return actions;
}

// ----------------------------------
//
// virtual
bool
Ookala::Chroma5::actionTaken(uint32_t   actionKey, 
                   PluginChain *chain    /* = NULL */,
                   uint32_t     sensorId /* = 0 */)
{
    setErrorString("");
    
    switch (actionKey) {

        // Here we need to probe for attached devices.
        case SENSOR_ACTION_DETECT:
            
            char versionString[32];

            if (mDeviceOpen) {

                // XXX: Close the device if it's already open
        
                mSIPerror = sipCloseCalibrator();

                if (mSIPerror != SUCCESS)
                {
                    std::ostringstream oss;
                    oss << mSIPerror;

                    setErrorString(
                        std::string("Unable to close Chroma5 [SIP error: ") +
                                oss.str() + std::string("]."));
                }

                mDeviceOpen = false;
            }

            // XXX: Open the device

            mSIPerror = sipOpenUSBChroma(versionString, NULL);

            if (mSIPerror != SUCCESS)
            {
                setErrorString("Unable to open Chroma5 sensor.");
                return false;
            }

            // XXX: And check that it's what we're expecting

            if (sipGetCalibratorType() != calCHROMA5U_LCD_CRT)
            {
                setErrorString("Unexpected sensor found - not Chroma5.");
                sipCloseCalibrator();
                mDeviceOpen = false;
                return false;
            }
            
            mDeviceOpen        = true;
            mReadScreenRefresh = false;
            break;

        default:
            return false;
            break;
    }


    return true;
}

// ----------------------------------
//
// Given a chain, lookup it's dict and pull out things that
// we care about:
//
//      Chroma5::displayType         [STRING]    {lcd, crt}
//      Chroma5::integrationTime     [INT]       Number of seconds to measure
//      Chroma5::calibrationIdx      [INT]       Override the calibration matrix
//                                               choice based on displayType.
//                                               Can be 0-3.
//
// protected
bool
Ookala::Chroma5::getDictOptions(DictOptions &opts, 
                                PluginChain *chain)
{
    Dict           *chainDict   = NULL;
    DictItem       *item        = NULL;
    StringDictItem *stringItem  = NULL;
    IntDictItem    *intItem     = NULL;
    DoubleDictItem *doubleItem  = NULL;
    std::string     displayType;

    // XXX: Set default values based on the appropriate types

    opts.displayType         = MEASTYPE_LCD;  // XXX:
    opts.integrationTime     = 1.0;
    opts.calibration         = WHICHCAL_LCD;  // XXX:

    if (!chain) {
        return false;
    }
    
    chainDict = chain->getDict();
    
    // Pull out the display type, and set a default calibration to match
    item = chainDict->get("Chroma5::displayType");
    if (item) {
        stringItem = dynamic_cast<StringDictItem *>(item);
        if (stringItem) {
            displayType = stringItem->get();

            std::transform(displayType.begin(), displayType.end(),
                           displayType.begin(), 
#ifdef _WIN32
                           tolower);
#else
                           (int(*)(int))std::tolower);
#endif

            if (displayType == "lcd") {
                // XXX: Set opts.displayType to LCD and opts.calibration
                //       to something that makes sense.
            
                opts.displayType = static_cast<int32_t>(MEASTYPE_LCD); // XXX:
                opts.calibration = static_cast<int32_t>(WHICHCAL_LCD); // XXX:

            } else if (displayType == "crt") {

                // XXX: Set opts.displayType to CRT and opts.calibration
                //       to something that makes sense.

                opts.displayType = static_cast<int32_t>(MEASTYPE_CRT); // XXX: 
                opts.calibration = static_cast<int32_t>(WHICHCAL_CRT); // XXX: 

            } else {
                setErrorString( std::string("Unknown display type: ") + 
                                        displayType + std::string("."));
                return false;
            }    
        }
    }

    // Pull out an integration time, in seconds
    item = chainDict->get("Chroma5::integrationTime");
    if (item) {
        doubleItem = dynamic_cast<DoubleDictItem *>(item);
        if (doubleItem) {
            opts.integrationTime = doubleItem->get();
        }
    }

    // Finally, get a calibration setting
    item = chainDict->get("Chroma5::calibrationIdx");
    if (item) {
        intItem = dynamic_cast<IntDictItem *>(item);
        if (intItem) {

            if ((intItem->get() < 0) || (intItem->get() > 3))  {
                setErrorString(
                    "Invalid calibration setting for Chroma5::calibration.");
                return false;
            }

            // XXX: Translate the integer that we get from intItem->get()
            //       to the appropriate value in opts.calibration.
            //      This may just need to be a cast.
            opts.calibration = static_cast<int32_t>(intItem->get());
        }
    }

    return true;
}


