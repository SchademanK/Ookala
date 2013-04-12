// --------------------------------------------------------------------------
// $Id: WxSensorPrep.cpp 135 2008-12-19 00:49:58Z omcf $
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

#ifdef __linux__
#include <unistd.h>
#endif
//#include <usb.h>

#include "Sensor.h"
#include "PluginChain.h"
#include "PluginRegistry.h"

#include "WxSensorPrep.h"


#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif


// ----------------------------------

BEGIN_PLUGIN_REGISTER(1)
    PLUGIN_REGISTER(0, Ookala::WxSensorPrep)
END_PLUGIN_REGISTER

// ----------------------------------



// ==================================
//
// WxSensorPrep
//
// ----------------------------------

Ookala::WxSensorPrep::WxSensorPrep():
  Plugin()
{
    setName("WxSensorPrep");
}

// ----------------------------------
//

Ookala::WxSensorPrep::WxSensorPrep(const WxSensorPrep &src):
    Plugin(src)
{
}

// ----------------------------------
//

Ookala::WxSensorPrep::~WxSensorPrep()
{

}

// ----------------------------
//
Ookala::WxSensorPrep &
Ookala::WxSensorPrep::operator=(const WxSensorPrep &src)
{
    if (this != &src) {
        Plugin::operator=(src);
    }

    return *this;
}

// ----------------------------------
//
// virtual protected
bool
Ookala::WxSensorPrep::_run(PluginChain *chain)
{
    std::string msg, caption;
    int32_t ret;

    setErrorString("");

    if (!chain) {
        setErrorString("No chain found in WxSensorPrep::_run()");
        return false;
    }

    // Here, we should look for sensors specified in the chain. Then,   
    // we'll need to probe them to make sure they're ready for
    // measurement. Finally, tell the user to stick it on 
    // the screen.

    std::vector<Plugin *> plugins = chain->queryByAttribute("sensor");
    if (plugins.empty()) {
        setErrorString("No sensor plugins found in chain.");
        return false;
    }

    Sensor *sensor = dynamic_cast<Sensor *>(plugins[0]);
    if (!sensor) {
        setErrorString("Found something that wasn't a sensor.");
        return false;
    }

    if (!sensor->actionTaken(SENSOR_ACTION_DETECT, chain)) {
        setErrorString(std::string("Unable to detect sensors: ") +
                       sensor->errorString());
        return false;
    }

    if (sensor->sensors().empty()) {
        setErrorString("No sensors detected.");
        return false;
    }

    std::vector<uint32_t> actions = sensor->actionNeeded(chain);
    for (std::vector<uint32_t>::iterator theAction = actions.begin();
            theAction != actions.end(); ++theAction) {
        switch (*theAction) {
            case SENSOR_ACTION_DETECT:
                if (!sensor->actionTaken(SENSOR_ACTION_DETECT, chain)) {
                    setErrorString(std::string("Unable to detect sensors: ") +
                                    sensor->errorString());
                    return false;
                }
                break;

            case SENSOR_ACTION_DARK_READING:
                msg     = "Please place the sensor in the dark for dark current measurement.";
                caption = "Sensor Setup: ";
                ret = wxMessageBox(msg, caption, wxOK | wxCANCEL | wxICON_INFORMATION);
                if (ret != wxOK) {
                    if (chain) {
                        chain->cancel();
                        return true;
                    }
                } 
                if (!sensor->actionTaken(SENSOR_ACTION_DARK_READING, chain)) {
                    setErrorString("Unable to measure dark current.");
                    return false;
                }
                break;

            case SENSOR_ACTION_MEASURE_REFRESH:
                setErrorString("Sensor wants to measure display refresh, currently unhandled.");
                return false;
                break;

            default:
                {
                    char buf[4096];
            
                    sprintf(buf, "Unhandled case in WxSensorPrep::_run(): %d.", *theAction);
                    setErrorString(buf);
                    return false;
                }
        }
    }    
    

    msg     = "Please aim the sensor at the screen";
    caption = "Sensor Setup: ";
    
    printf("wxSensorPrep - trying to pop up message box\n");
    ret = wxMessageBox(msg, caption,
                         wxOK | wxCANCEL | wxICON_INFORMATION);

    if (ret == wxOK) {
        return true;
    } else {
        chain->cancel();
        return false;
    }


    return true;
}


