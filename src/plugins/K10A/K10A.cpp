// --------------------------------------------------------------------------
// $Id: K10A.cpp 135 2008-12-19 00:49:58Z omcf $
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

//K10A - headers ----------------------------------------------------
#include "K10A.h"
#include "KClmtr.h"

// ----------------------------------

BEGIN_PLUGIN_REGISTER(1)
	PLUGIN_REGISTER(0, Ookala::K10A)
END_PLUGIN_REGISTER

// ----------------------------------

KClmtr *K = new KClmtr();


// ----------------------------------
//
// Utils

bool errorCheck(int error) {
	std::string stringError;

	//Avergring, just needs to display it
	error &= ~KClmtr::AVERAGING_LOW_LIGHT;
	//Resetting the FFT data
	error &= ~KClmtr::FFT_PREVIOUS_RANGE;
	//The data isn't ready to display yet
	error &= ~KClmtr::FFT_INSUFFICIENT_DATA;
	if(false) {
		error &= ~KClmtr::AIMING_LIGHTS;
	}
	if(true) {
		error &= ~KClmtr::BOTTOM_UNDER_RANGE;
		error &= ~KClmtr::TOP_OVER_RANGE;
		error &= ~KClmtr::OVER_HIGH_RANGE;
	}
	if(error) {
		if(error & KClmtr::CONVERTED_NM) {
			if (stringError.length()>0) { stringError += ", "; }
			stringError += "converting to NM failed";
		}
		if(error & KClmtr::KELVINS) {
			if (stringError.length()>0) { stringError += ", "; }
			stringError += "converting to Kelvins failed";
		}
		if(error & KClmtr::AIMING_LIGHTS) {
			if (stringError.length()>0) { stringError += ", "; }
			stringError += "the Aiming lights are on";
		}
		if(error & (KClmtr::BOTTOM_UNDER_RANGE | KClmtr::TOP_OVER_RANGE | KClmtr::OVER_HIGH_RANGE)) {
			if (stringError.length()>0) { stringError += ", "; }
			stringError +=  "out of range";
		}
		if(error & KClmtr::FFT_BAD_STRING) {
			if (stringError.length()>0) { stringError += ", "; }
			stringError += "the Flicker string from the Klein device was bad";
		}
		if(error & (KClmtr::NOT_OPEN | KClmtr::TIMED_OUT | KClmtr::LOST_CONNECTION)) {
			K->closePort();
			if (stringError.length()>0) { stringError += ", "; }
			stringError += "the the Klein device as been unplugged";
		}

		std::cout << "[K10A error " << error << "] " << stringError << "\n";
		return false;
	} else {
		return true;
	}
}


// ==================================
//
// K10A 
//
// ----------------------------------

Ookala::K10A::K10A():
  Sensor()
{
	setName("K10A");

	mDeviceOpen		= false;
	mNextSensorKey	 = 1;
	mReadScreenRefresh = false;
}

// ----------------------------------
//

Ookala::K10A::K10A(const K10A  &src):
	Sensor(src)
{
	mDeviceOpen		= src.mDeviceOpen;
	mNextSensorKey	 = src.mNextSensorKey;
	mDeviceKey		 = src.mDeviceKey;
	mReadScreenRefresh = src.mReadScreenRefresh;
}

// ----------------------------------
//
// virtual
Ookala::K10A::~K10A()
{
	// If we're left with an open device, close it.
	if (mDeviceOpen) {

		// Close the device handle.
		std::cout << " disposing of open sensor\n";
		K->closePort();

		mDeviceOpen = false;
	}
}

// ----------------------------
//
Ookala::K10A &
Ookala::K10A::operator=(const K10A &src)
{
	if (this != &src) {
		Sensor::operator=(src);

		mNextSensorKey	 = src.mNextSensorKey;
		mDeviceOpen		= src.mDeviceOpen;
		mDeviceKey		 = src.mDeviceKey;
		mReadScreenRefresh = src.mReadScreenRefresh;
	}

	return *this;
}

// ----------------------------------
//
// virtual
std::vector<uint32_t>
Ookala::K10A::sensors()
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
Ookala::K10A::measureYxy(Yxy	   &value,
				  PluginChain *chain	 /* = NULL */,
				  uint32_t	 sensorId  /* = 0 */)
{
	DictOptions opts;

	setErrorString("");

	if (!getDictOptions(opts, chain)) {
		return false;
	}

	// Read a measurement in cd/m&2 into the value parameter.
	if(K->isPortOpen()){
		Measurement measurement = K->getNextMeasurement(opts.measurements);

		errorCheck(measurement.errorcode);
		value.Y = measurement.bigy;
		value.x = measurement.x;
		value.y = measurement.y;
	} else {
		std::cout << "measureXY: Port is not open\n";
	}

	if (chain) {
		chain->setUiYxy("Ui::measured_Yxy", value);
	}

	return true;
}

// ----------------------------------
//
// virtual
std::vector<uint32_t> 
Ookala::K10A::actionNeeded(
					  PluginChain *chain	/* = NULL */,
					  uint32_t	 sensorId /* = 0 */)
{
	std::vector<uint32_t> actions;
	DictOptions		   opts;

	setErrorString("");

	if (getDictOptions(opts, chain)) {
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
Ookala::K10A::actionTaken(uint32_t   actionKey, 
				   PluginChain *chain	/* = NULL */,
				   uint32_t	 sensorId /* = 0 */)
{
	bool connected = false;
    DictOptions opts;

    if (!getDictOptions(opts, chain)) {
        return false;
    }
	
	setErrorString("");
	
	switch (actionKey) {

		// Here we need to probe for attached devices.
		case SENSOR_ACTION_DETECT:
			
			if (mDeviceOpen) {

				// Close the device if it's already open
				std::cout << " actionTaken: close open sensor...\n";
				K->closePort();

				mDeviceOpen = false;
			}

			// Open the device
			std::cout << " actionTaken: detect and open sensor device...\n";
			connected = K->connect(opts.port);
			if (!connected) {
				setErrorString("Unable to open K10A sensor.");
				std::cout << "Error: Unable to open K10A sensor.\n";
				return false;
			}

			mDeviceOpen = true;

			// Set Calibration File on the device
			K->setCalFileID(opts.calFileID);

			std::cout <<
				"port        : " << K->getPort() << "\n" <<
				"calFileID   : " << K->getCalFileID() << " " << K->getCalFileName() << "\n" <<
				"measurements: " << opts.measurements << "\n";

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
//	  K10A::port             [STRING]  Serial port name
//	  K10A::calFileID        [INT]     Which of the Cal files stored on
//	                                   the device to use, can be 0-96.
//	  K10A::measurements     [INT]     Number of measurements to take.
//	                                   The result will be an average of
//	                                   all measurements taken.
//
// protected
bool
Ookala::K10A::getDictOptions(DictOptions &opts, 
								PluginChain *chain)
{
	Dict			*chainDict	 = NULL;
	DictItem		*item		 = NULL;
	StringDictItem	*stringItem	 = NULL;
	IntDictItem		*intItem	 = NULL;
	DoubleDictItem	*doubleItem	 = NULL;

	opts.port			 = "";
	opts.calFileID		 = 0;
	opts.measurements	 = 1;

	if (!chain) {

		std::cout << " DictOptions: no chain detected...\n";

		return false;
	}
	
	chainDict = chain->getDict();

	// Get the serial port to use
    item = chainDict->get("K10A::port");
    if (item) {
        stringItem = dynamic_cast<StringDictItem *>(item);
        if (stringItem) {
            opts.port = stringItem->get();
		}
	}

	// Get the ID of the Cal file to use
	item = chainDict->get("K10A::calFileID");
	if (item) {
		intItem = dynamic_cast<IntDictItem *>(item);
		if (intItem) {
			if ((intItem->get() < 0) || (intItem->get() > 96))  {
				setErrorString(
				 "Invalid setting for K10A::calFileID, it must be in the range 0-96");
				return false;
			}

			opts.calFileID = intItem->get();

		}
	}

	// Pull out the number of measurements to take
	item = chainDict->get("K10A::measurements");
	if (item) {
		intItem = dynamic_cast<IntDictItem *>(item);
		if (intItem) {
			opts.measurements = intItem->get();
		}
	}

	return true;
}


