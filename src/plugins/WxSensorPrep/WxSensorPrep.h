// --------------------------------------------------------------------------
// $Id: WxSensorPrep.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef WXSENSORPREP_H_HAS_BEEN_INCLUDED
#define WXSENSORPREP_H_HAS_BEEN_INCLUDED

#include "Types.h"
#include "Plugin.h"
#include "Dict.h"
#include "Sensor.h"

// This plugin is ment to be inserted into a chain before
// plugins that require a sensor. When run, it finds sensors
// in the chain and tests if they require any actions.
//
// If actions are required, and the actions are things that
// we understand, we'll try and handle them.
//
// For instance, if the sensor we want to use requires a
// dark current measurement, we should pop up a dialog box
// that prompts the user to put a sock over the sensor.
//
// This probably is not the place to handle setting sensor
// configs which don't require user action. Things like
// choosing the calibration matrix of a colorimeter would 
// best be served in the sensor plugin themselves.  The motivation
// for not including the functionality of WxSensorPrep in the
// sensor plugins is to remove a UI dependency from the 
// sensor plugins.
//
// This will also need to prompt users to point their sensor
// at the screen.
//

namespace Ookala {

class WxSensorPrep: public Plugin
{
    public:
        WxSensorPrep();
        WxSensorPrep(const WxSensorPrep &src);
        virtual ~WxSensorPrep();
        WxSensorPrep & operator=(const WxSensorPrep &src);

        PLUGIN_ALLOC_FUNCS(WxSensorPrep);

    protected:

        virtual bool _run(PluginChain *chain);
};

}; // namespace Ookala

#endif


