// --------------------------------------------------------------------------
// $Id: ExampleSensor.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <unistd.h>
#include <usb.h>

#elif WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <iostream>

#include "Sensor.h"
#include "PluginChain.h"

//ExampleSensor - headers ----------------------------------------------------
#include "ExampleSensor.h"

// ----------------------------------

BEGIN_PLUGIN_REGISTER(1)
    PLUGIN_REGISTER(0, Ookala::ExampleSensor)
END_PLUGIN_REGISTER

// ----------------------------------


// ==================================
//
// ExampleSensor 
//
// ----------------------------------

Ookala::ExampleSensor::ExampleSensor():
  Sensor()
{
    setName("ExampleSensor");

    mDeviceOpen        = false;
    mNextSensorKey     = 1;
    mReadScreenRefresh = false;
}

// ----------------------------------
//

Ookala::ExampleSensor::ExampleSensor(const ExampleSensor  &src):
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
Ookala::ExampleSensor::~ExampleSensor()
{
    // If we're left with an open device, close it.
    if (mDeviceOpen) {

        // XXX: Close the device handle.

        std::cout << " disposing of open sensor\n";

        mDeviceOpen = false;
    }
}

// ----------------------------
//
Ookala::ExampleSensor &
Ookala::ExampleSensor::operator=(const ExampleSensor &src)
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
Ookala::ExampleSensor::sensors()
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
Ookala::ExampleSensor::measureYxy(Yxy       &value,
                  PluginChain *chain     /* = NULL */,
                  uint32_t     sensorId  /* = 0 */)
{
    DictOptions opts;

    setErrorString("");

    if (!getDictOptions(opts, chain)) {
        return false;
    }

    // Update the calibration, in case someone changed
    // it out from under us.
    
    
    // XXX: Choose the calibration matrix, based on what's selected
    //      in opts.calibration

    std::cout << " measureXY: selecting calibration matrix...\n";

    // XXX: Choose the display type (e.g. LCD, CRT, etc) base
    //      on the value in opts.displayType

    std::cout << " measureXY: setup LCD or CRT...\n";

    // XXX: Choose the integration time based on the value in
    //      opts.integrationTime

    std::cout << " measureXY: integration time applied...\n";

    // XXX: Finally, read a measurement in cd/m&2 into the value
    //      parameter.

    std::cout << " measureXY: reading measurement Yxy (candela)...\n";

    // XXX: Return measured value

    value.Y = (double) 1.0;		// Simulate (dummy) values
    value.x = (double) 0.312779;
    value.y = (double) 0.329183;

    std::cout << 
         " measureXY: read (dummy) values Yxy (1.0, 0.31, 0.32) ...\n";

    if (chain) {
        chain->setUiYxy("Ui::measured_Yxy", value);
    }

    return true;
}

// ----------------------------------
//
// virtual
std::vector<uint32_t> 
Ookala::ExampleSensor::actionNeeded(
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
        //      or for LCD:
        //          actions.push_back(SENSOR_ACTION_DARK_READING).
        //      YMMV.
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
Ookala::ExampleSensor::actionTaken(uint32_t   actionKey, 
                   PluginChain *chain    /* = NULL */,
                   uint32_t     sensorId /* = 0 */)
{
    setErrorString("");
    
    switch (actionKey) {

        // Here we need to probe for attached devices.
        case SENSOR_ACTION_DETECT:
            
            if (mDeviceOpen) {

                // XXX: Close the device if it's already open
    
                std::cout << " actionTaken: open sensor closed...\n";

                mDeviceOpen = false;
            }

            // XXX: Open the device

            std::cout << " actionTaken: detect and open sensor device...\n";

            // XXX: And check that it's what we're expecting

            std::cout << " actionTaken: confirm state...\n";

            // Simulate open (dummy) sensor succeeded
            mDeviceOpen = true;

            break;

        case SENSOR_ACTION_DARK_READING:

            // XXX: Establish a dark reading point

            std::cout << " actionTaken: conduct Dark Reading...\n";

            break;

        case SENSOR_ACTION_MEASURE_REFRESH:

            // XXX: Measure the refresh rate of monitor (CRT)

            std::cout << " actionTaken: conduct Refresh Reading (CRT)...\n";

            // Simulate refresh (dummy) action succeeded
            mReadScreenRefresh = true; 

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
//      ExampleSensor::displayType         [STRING] {lcd, crt}
//      ExampleSensor::integrationTime     [INT]  Number of seconds to measure
//      ExampleSensor::calibrationIdx      [INT]  Override the calibration 
//                                                matrix choice based on 
//                                                displayType.
//                                                Can be 0-3.
//
// protected
bool
Ookala::ExampleSensor::getDictOptions(DictOptions &opts, 
                                PluginChain *chain)
{
    Dict           *chainDict   = NULL;
    DictItem       *item        = NULL;
    StringDictItem *stringItem  = NULL;
    IntDictItem    *intItem     = NULL;
    DoubleDictItem *doubleItem  = NULL;
    std::string     displayType;

    // XXX: Set default values based on the appropriate types

    opts.displayType         = 0;  // XXX:
    opts.integrationTime     = 1;
    opts.calibration         = 0;  // XXX:

    std::cout << " DictOptions: setup base defaults...\n";

    if (!chain) {

        std::cout << " DictOptions: no chain detected...\n";

        return false;
    }
    
    chainDict = chain->getDict();
    
    // Pull out the display type, and set a default calibration to match
    item = chainDict->get("ExampleSensor::displayType");
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
            
                opts.displayType = 0; // XXX:
                opts.calibration = 0; // XXX:
        
                std::cout << " DictOptions: setup LCD parameters...\n";

            } else if (displayType == "crt") {

                // XXX: Set opts.displayType to CRT and opts.calibration
                //       to something that makes sense.

                opts.displayType = 0; // XXX: 
                opts.calibration = 0; // XXX: 

                std::cout << " DictOptions: setup CRT parameters...\n";

            } else {
                setErrorString( std::string("Unknown display type: ") + 
                                        displayType + std::string("."));

                std::cout << " DictOptions: detected Unknown DisplayType...\n";

                return false;
            }    
        }
    }

    // Pull out an integration time, in seconds
    item = chainDict->get("ExampleSensor::integrationTime");
    if (item) {
        doubleItem = dynamic_cast<DoubleDictItem *>(item);
        if (doubleItem) {
            opts.integrationTime = doubleItem->get();

            std::cout << " DictOptions: IntegrationTime set" << 
                         opts.integrationTime << "\n";;
        }
    }

    // Finally, get a calibration setting
    item = chainDict->get("ExampleSensor::calibrationIdx");
    if (item) {
        intItem = dynamic_cast<IntDictItem *>(item);
        if (intItem) {

            // XXX: Check range
            if ((intItem->get() < 0) || (intItem->get() > 3))  {
                setErrorString(
                 "Invalid calibration setting for ExampleSensor::calibration.");
                return false;
            }

            // XXX: Translate the integer that we get from intItem->get()
            //       to the appropriate value in opts.calibration.
            //      This may just need to be a cast.
            opts.calibration = intItem->get();

            std::cout << " DictOptions: Calibration Index set" << 
                           opts.calibration << "\n";;
        }
    }

    return true;
}

