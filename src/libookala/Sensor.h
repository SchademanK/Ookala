// --------------------------------------------------------------------------
// $Id: Sensor.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef SENSOR_H_HAS_BEEN_INCLUDED
#define SENSOR_H_HAS_BEEN_INCLUDED

#include <vector>
#include <string>

#include "Types.h"
#include "Plugin.h"

namespace Ookala {

// Making these enums makes it hard for other folks to add in
// new values in external modules
#define SENSOR_ACTION_DETECT            1
#define SENSOR_ACTION_DARK_READING      2
#define SENSOR_ACTION_MEASURE_REFRESH   3

//
// Base class for interfacing with color sensors
//

class PluginChain;
class EXIMPORT Sensor: public Plugin
{
    public:
        Sensor();
        Sensor(const Sensor &src);
        virtual ~Sensor();
        Sensor & operator=(const Sensor &src);

        // Returns a vector of keys pointing to sensors
        // that we have detected. Keys should persist 
        // over re-enumeration.
        virtual std::vector<uint32_t> sensors();

        // Called to take a measurement given the current setup.
        // This can have the side effect that user actions are
        // prompted for, and their associated processing happens.
        //
        // Values are returned in units of cd/m^2.
        virtual bool measureYxy(Yxy         &value,
                                PluginChain *chain    = NULL,
                                uint32_t     sensorId  = 0);

        // Sensor actions are things that we need to convince someone
        // to do for us. Usually, its something like putting the
        // sensor in a bag for dark current measurement, or placing
        // the sensor on a white patch of CRT for measuring refresh 
        // rate (whats a CRT? oh right..). 
        //
        // We could try and allow the sensor to automatically handle
        // these things. However, this adds some weird cycles to 
        // the separation of classes. For example, a sensor could
        // need to access a Gui and Device Control object, to crank
        // the brightness on a CRT and display a white field, for
        // measuring refresh rate.
        //
        // Alternatively, we could put the burdon on upper levels
        // of logic to do this correctly. In this case, we'll 
        // need a way to communicate back what the sensor needs
        // to have done, and things have been setup to take action.
        //
        // This usually should be communicated to the Gui.
        //
        // Returns a vector of keys of actions that still need 
        // to be taken.
        virtual std::vector<uint32_t> actionNeeded(
                                    PluginChain *chain = NULL,
                                    uint32_t     sensorId = 0);
        
        // Call this once the user has followed instructions. This 
        // may cause some processing to happen, like taking a dark
        // current measurement. Returns false if the processing
        // failed for some reason. This could mean that the
        // screen color can change, if we need to measure a 
        // particular field on-screen.
        virtual bool actionTaken(uint32_t     actionKey, 
                                 PluginChain *chain = NULL,
                                 uint32_t     sensorId = 0);
};

}; // namespace Ookala

#endif
