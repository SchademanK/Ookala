// --------------------------------------------------------------------------
// $Id: Sensor.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include "Sensor.h"
#include "PluginChain.h"

//
// Base class for interfacing with color sensors
//

Ookala::Sensor::Sensor(): Plugin()
{
    addAttribute("Sensor");
    addAttribute("sensor");
}

// -------------------------------
// 
Ookala::Sensor::Sensor(const Sensor &src):
    Plugin(src)
{

}

// -------------------------------
// 
// virtual

Ookala::Sensor::~Sensor()
{
}

// ----------------------------
//
Ookala::Sensor &
Ookala::Sensor::operator=(const Sensor &src)
{
    if (this != &src) {
        Plugin::operator=(src);
    }

    return *this;
}


// -------------------------------
//
// Returns the number of sensors that we've detected.
//
// virtual
std::vector<uint32_t>
Ookala::Sensor::sensors()
{
    std::vector<uint32_t> theVector;

    return theVector;
}


// -------------------------------
//
// Called to take a measurement given the current setup.
// This can have the side effect that user actions are
// prompted for, and their associated processing happens.
//
// Values are returned in units of cd/m^2.
//
// virtual
bool
Ookala::Sensor::measureYxy(
                   Yxy &value,
                   PluginChain *chain     /* = NULL*/,
                   uint32_t     sensorIdx /* = 0 */)
{
    return false;
}


// -------------------------------
//
// returns a vector of keys of actions
// that still need to be taken.
//
// virtual
std::vector<uint32_t> 
Ookala::Sensor::actionNeeded(
                     PluginChain *chain    /* = NULL */,
                     uint32_t     sensorId /* = 0 */)
{
    std::vector<uint32_t> theVector;

    return theVector;
}


// -------------------------------
//
// Call this once the user has followed instructions. This 
// may cause some processing to happen, like taking a dark
// current measurement. Returns false if the processing
// failed for some reason. This could mean that the
// screen color can change, if we need to measure a 
// particular field on-screen.
//
// virtual 
bool 
Ookala::Sensor::actionTaken(
                    uint32_t actionKey, 
                    PluginChain *chain    /* = NULL */,
                    uint32_t     sensorId /* = 0 */)
{
    return false;
}

