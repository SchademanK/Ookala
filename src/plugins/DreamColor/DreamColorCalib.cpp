// --------------------------------------------------------------------------
// $Id: DreamColorCalib.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <math.h>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <time.h>
#endif

#include "Dict.h"
#include "DictHash.h"
#include "PluginRegistry.h"
#include "PluginChain.h"
#include "DataSavior.h"
#include "Sensor.h"
#include "Ddc.h"
#include "Color.h"
#include "Interpolate.h"

#include "plugins/DreamColor/DreamColorSpaceInfo.h"
#include "plugins/DreamColor/DreamColorCtrl.h"
#include "plugins/WsLut/WsLut.h"

#include "DreamColorCalib.h"

namespace Ookala {

struct DataPoint
{
    Yxy  measuredYxy;
    Rgb  linearRgb;
    uint32_t     backlightReg[4];
};

};

// =====================================
//
// DreamColorCalib
//
// -------------------------------------

Ookala::DreamColorCalib::DreamColorCalib():
    Plugin()
{
    setName("DreamColorCalib");

    mNumGraySamples = 16;
}

// -------------------------------------

Ookala::DreamColorCalib::DreamColorCalib(const DreamColorCalib &src):
    Plugin(src)
{
    mNumGraySamples = src.mNumGraySamples;
}   


// -------------------------------------
//
// virtual
Ookala::DreamColorCalib::~DreamColorCalib()
{
}

// ----------------------------
//
Ookala::DreamColorCalib &
Ookala::DreamColorCalib::operator=(const DreamColorCalib &src)
{
    if (this != &src) {
        Plugin::operator=(src);

        mNumGraySamples = src.mNumGraySamples;
    }

    return *this;
}

// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCalib::checkCalibRecord(CalibRecordDictItem *calibRec)
{
    DreamColorCalibRecord *myCalibRec;

    if (calibRec->getCalibrationPluginName() != 
                std::string("DreamColorCalib")) {
        return false;
    }

    myCalibRec = dynamic_cast<DreamColorCalibRecord *>(calibRec);
    if (!myCalibRec) {
        return false;
    }

    // Check the preset and the brightness register on the 
    // display.
    //
    // XXX: We should really be scanning by EDID here, but 
    //      we're not serializing that yet, so just pick a display
    //      for now.
    
    if (!mRegistry) {
        setErrorString("No Registry in DreamColorCalib.");
        return false;
    }
    std::vector<Plugin *> plugins = mRegistry->queryByName("DreamColorCtrl");
    if (plugins.empty()) {
        setErrorString("No DreamColorCtrl plugin found.");
        return false;
    }
    DreamColorCtrl *disp = dynamic_cast<DreamColorCtrl *>(plugins[0]);
    if (!disp) {
        setErrorString("No DreamColorCtrl plugin found.");
        return false;
    }

    // Go ahead and just re-enumerate now, in case this hasn't happened,
    // or someone has been re-plugging their displays.
    disp->enumerate();

        
    // Now check the display to see how its' doing.
    uint32_t currCsIdx, regValues[4];

    if (!disp->getColorSpace(currCsIdx)) {
        setErrorString("Unable to get color space from display.");
        return false;
    }

    if (!disp->getBacklightRegRaw(regValues[0], regValues[1], 
                                  regValues[2], regValues[3])) {
        setErrorString("Unable to get backlight registers from display.");
        return false;
    }

    if (myCalibRec->getPreset() != currCsIdx) {
        setErrorString("Color space preset does not match calibration.");
        return false;
    }

    if (myCalibRec->getBrightnessReg() != regValues[3]) {
        setErrorString("Color space brightness does not match calibration.");
        return false;
    }

    return true;
}


// -------------------------------------
//
// protected
void
Ookala::DreamColorCalib::gatherOptions(PluginChain *chain)
{
    std::vector<Plugin *>      plugins; 
    DictHash                  *hash;
    Dict                      *dict;
    DictItem                  *item;
    IntDictItem               *intItem;
    DoubleArrayDictItem       *doubleArrayItem;

    // Reset default values
    mNumGraySamples = 16;

    // Look for the proper dict
    if (!mRegistry) {
        return;
    }

    plugins = mRegistry->queryByName("DictHash");
    if (plugins.empty()) {
        return;
    }
    hash = dynamic_cast<DictHash *>(plugins[0]);
    if (!hash) {
        return;
    }

    dict = hash->getDict(chain->getDictName());
    if (dict == NULL) {
        return;
    }

    // Look for the items we care about
    item = dict->get("DreamColorCalib::graySamples");
    intItem = dynamic_cast<IntDictItem *>(item);
    if (intItem) {
        mNumGraySamples = intItem->get();

        // Just for sanity...
        if (mNumGraySamples > 1024) {
            mNumGraySamples = 1024;
        }
    }

    // Figure out measurement tolerances. Look in the provided dictionary for:
    // 
    //    DreamColorCalib::whiteTolerance    (vec3, Yxy) -> same value for +-
    //    DreamColorCalib::redTolerance      (vec2, xy)  -> same value for +-
    //    DreamColorCalib::greenTolerance    (vec2, xy)  -> same value for +-
    //    DreamColorCalib::blueTolerance     (vec2, xy)  -> same value for +-

    //    DreamColorCalib::whiteTolerancePlus  (vec3, Yxy) -> White + tolerance
    //    DreamColorCalib::whiteToleranceMinus (vec3, Yxy) -> White - tolerance
    //
    //    DreamColorCalib::redTolerancePlus  (vec2, xy) -> Red + tolerance
    //    DreamColorCalib::redToleranceMinus (vec2, xy) -> Red - tolerance
    //
    //    DreamColorCalib::greenTolerancePlus  (vec2, xy) -> Green + tolerance
    //    DreamColorCalib::greenToleranceMinus (vec2, xy) -> Green - tolerance
    //
    //    DreamColorCalib::blueTolerancePlus  (vec2, xy) -> Blue + tolerance
    //    DreamColorCalib::blueToleranceMinus (vec2, xy) -> Blue - tolerance


    mWhiteTolerancePlus.Y   =  1.0;
    mWhiteToleranceMinus.Y  = 

    mWhiteTolerancePlus.x  = 
    mWhiteToleranceMinus.x = 
    mWhiteTolerancePlus.y  = 
    mWhiteToleranceMinus.y = 0.0025;

    mRedTolerancePlus.x  = 
    mRedToleranceMinus.x = 
    mRedTolerancePlus.y  = 
    mRedToleranceMinus.y = 0.0025;

    mGreenTolerancePlus.x  = 
    mGreenToleranceMinus.x = 
    mGreenTolerancePlus.y  = 
    mGreenToleranceMinus.y = 0.0025;

    mBlueTolerancePlus.x  = 
    mBlueToleranceMinus.x = 
    mBlueTolerancePlus.y  = 
    mBlueToleranceMinus.y = 0.0025;

    // White user tolerances
    item = dict->get("DreamColorCalib::whiteTolerance");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 3) {
                mWhiteTolerancePlus.Y  = 
                mWhiteToleranceMinus.Y = (doubleArrayItem->get())[0];
                mWhiteTolerancePlus.x  = 
                mWhiteToleranceMinus.x = (doubleArrayItem->get())[1];
                mWhiteTolerancePlus.y  = 
                mWhiteToleranceMinus.y = (doubleArrayItem->get())[2];
            }
        }
    }
    item = dict->get("DreamColorCalib::whiteTolerancePlus");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 3) {
                mWhiteTolerancePlus.Y  = (doubleArrayItem->get())[0];
                mWhiteTolerancePlus.x  = (doubleArrayItem->get())[1];
                mWhiteTolerancePlus.y  = (doubleArrayItem->get())[2];
            }
        }
    }
    item = dict->get("DreamColorCalib::whiteToleranceMinus");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 3) {
                mWhiteToleranceMinus.Y  = (doubleArrayItem->get())[0];
                mWhiteToleranceMinus.x  = (doubleArrayItem->get())[1];
                mWhiteToleranceMinus.y  = (doubleArrayItem->get())[2];
            }
        }
    }

    // Red user tolerances
    item = dict->get("DreamColorCalib::redTolerance");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mRedTolerancePlus.x  = 
                mRedToleranceMinus.x = (doubleArrayItem->get())[0];
                mRedTolerancePlus.y  = 
                mRedToleranceMinus.y = (doubleArrayItem->get())[1];
            }
        }
    }
    item = dict->get("DreamColorCalib::redTolerancePlus");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mRedTolerancePlus.x  = (doubleArrayItem->get())[0];
                mRedTolerancePlus.y  = (doubleArrayItem->get())[1];
            }
        }
    }
    item = dict->get("DreamColorCalib::redToleranceMinus");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mRedToleranceMinus.x  = (doubleArrayItem->get())[0];
                mRedToleranceMinus.y  = (doubleArrayItem->get())[1];
            }
        }
    }


    // Green user tolerances
    item = dict->get("DreamColorCalib::greenTolerance");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mGreenTolerancePlus.x  = 
                mGreenToleranceMinus.x = (doubleArrayItem->get())[0];
                mGreenTolerancePlus.y  = 
                mGreenToleranceMinus.y = (doubleArrayItem->get())[1];
            }
        }
    }
    item = dict->get("DreamColorCalib::greenTolerancePlus");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mGreenTolerancePlus.x  = (doubleArrayItem->get())[0];
                mGreenTolerancePlus.y  = (doubleArrayItem->get())[1];
            }
        }
    }
    item = dict->get("DreamColorCalib::greenToleranceMinus");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mGreenToleranceMinus.x  = (doubleArrayItem->get())[0];
                mGreenToleranceMinus.y  = (doubleArrayItem->get())[1];
            }
        }
    }

    // Blue user tolerances
    item = dict->get("DreamColorCalib::blueTolerance");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mBlueTolerancePlus.x  = 
                mBlueToleranceMinus.x = (doubleArrayItem->get())[0];
                mBlueTolerancePlus.y  = 
                mBlueToleranceMinus.y = (doubleArrayItem->get())[1];
            }
        }
    }

    item = dict->get("DreamColorCalib::blueTolerancePlus");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mBlueTolerancePlus.x  = (doubleArrayItem->get())[0];
                mBlueTolerancePlus.y  = (doubleArrayItem->get())[1];
            }
        }
    }
    item = dict->get("DreamColorCalib::blueToleranceMinus");
    if (item) {
        doubleArrayItem = dynamic_cast<DoubleArrayDictItem *>(item);
        if (doubleArrayItem) {
            if (doubleArrayItem->get().size() >= 2) {
                mBlueToleranceMinus.x  = (doubleArrayItem->get())[0];
                mBlueToleranceMinus.y  = (doubleArrayItem->get())[1];
            }
        }
    }
}

// -------------------------------------
//
// protected
std::vector<Ookala::DreamColorSpaceInfo>
Ookala::DreamColorCalib::gatherTargets(PluginChain *chain)
{
    std::vector<DreamColorSpaceInfo>    targets;
    std::vector<Plugin *>               plugins; 
    DictHash                           *hash;
    Dict                               *dict;
    StringArrayDictItem                *nameListItem;
    std::vector<std::string>            nameList;

    if (!mRegistry) {
        return targets;
    }

    plugins = mRegistry->queryByName("DictHash");
    if (plugins.empty()) {
        setErrorString("No DictHash found");
        return targets;
    }
    hash = dynamic_cast<DictHash *>(plugins[0]);
    if (!hash) {
        setErrorString("No DictHash cast");
        return targets;
    }

    dict = hash->getDict(chain->getDictName());
    if (dict == NULL) {
        setErrorString("No chain dict found.");
        return targets;
    }
    
    nameListItem = dynamic_cast<StringArrayDictItem *>(
                        dict->get("DreamColorCalib::targets"));
    if (!nameListItem) {
        return targets;
    }

    nameList = nameListItem->get();

    for (std::vector<std::string>::iterator name = nameList.begin();
                name != nameList.end(); ++name) {

        DreamColorSpaceInfo *targetItem = 
                        dynamic_cast<DreamColorSpaceInfo *>(dict->get(*name));
        if (!targetItem) {
            continue;
        }

        // If we're not the first one, check the device/connection id
        if (!targets.empty()) {
            if (targets[0].getConnId() != targetItem->getConnId()) {
                continue;
            }
        }

        targets.push_back(*targetItem);
    }

    return targets;
}

// -------------------------------------
//
// virtual protected
bool
Ookala::DreamColorCalib::gatherPlugins(PluginData  &pi,
                                       PluginChain *chain)     
{
    std::vector<Plugin *>plugins;

    setErrorString("");

    if (mRegistry == NULL) {
        setErrorString("No registry found.");
        return false;
    }

    // Check for a DreamColorCtrl
    plugins = mRegistry->queryByName("DreamColorCtrl");
    if (plugins.empty()) {
        setErrorString("No DreamColorCtrl plugin found.");
        return false;
    }
    pi.disp = dynamic_cast<DreamColorCtrl *>(plugins[0]);
    if (!(pi.disp)) {
        setErrorString("No DreamColorCtrl plugin found.");
        return false;
    }

    pi.dispId = 0;


    // Check for a sensor in the chain
    bool testReg = true;

    plugins = chain->queryByAttribute("sensor");
    if (!plugins.empty()) {
        pi.sensor = dynamic_cast<Sensor *>(plugins[0]);
        if ((pi.sensor) != NULL) {
            testReg = false;
        }
    }

    if (testReg) {
        plugins = mRegistry->queryByAttribute("sensor");
        if (!plugins.empty()) {
            pi.sensor = dynamic_cast<Sensor *>(plugins[0]);
            if ((pi.sensor) == NULL) {
                setErrorString("No sensor found.");
                return false;
            }
        } else {
            setErrorString("No sensor found.");
            return false;
        }
    }

    // And make sure the sensor is ready to go - it's ok to enumerate
    // sensors if we haven't done that yet
    (pi.sensor)->actionTaken(SENSOR_ACTION_DETECT, chain);

    if ( (pi.sensor)->sensors().empty() ) {
        setErrorString("No sensors found.");
        return false;
    }

    if (!(pi.sensor)->actionNeeded(chain).empty()) {
        setErrorString("Sensor is not ready for measuring.");
        return false;
    }
    
    pi.sensorId = 0;

    // Check for a color space plugin - it's build it, so it had
    // best be there, less extreme badness has occurred.
    plugins = mRegistry->queryByName("Color");
    if (plugins.empty()) {
        setErrorString("No Color plugin found.");
        return false;
    }
    pi.color = dynamic_cast<Color *>(plugins[0]);
    if (!(pi.color)) {
        setErrorString("No Color plugin found.");
        return false;
    }
    
    // Similarly, for an interpolator
    plugins = mRegistry->queryByName("Interpolate");
    if (plugins.empty()) {
        setErrorString("No Interpolate plugin found.");
        return false;
    }
    pi.interp = dynamic_cast<Interpolate *>(plugins[0]);
    if (!(pi.interp)) {
        setErrorString("No Interpolate plugin found.");
        return false;
    }

    

    // Make sure we have a WsLut plugin as well; and that it 
    // has some data    
    plugins = mRegistry->queryByName("WsLut");
    if (plugins.empty()) {
        setErrorString("No WsLut plugin found.");
        return false;
    }
    pi.lut = dynamic_cast<WsLut *>(plugins[0]);
    if (!pi.lut) {
        setErrorString("No WsLut plugin found.");
        return false;
    }
    if (pi.lut->luts().empty()) {
        setErrorString("WsLut plugin has not Luts available.");
        return false;
    }

    return true;
}

// -------------------------------------
//
// virtual protected
bool
Ookala::DreamColorCalib::_run(PluginChain *chain)
{
    PluginData                   pi;
    DreamColorCalibrationData    calib;
    Yxy                          goalWhite;
    Npm                          panelNpm;
    uint32_t                     backlightReg[4];

    // Mapping from the display connection id to the preset
    // state that the device is in when we start out.
    std::map<uint32_t, uint32_t> origPresets;

    setErrorString("");

    gatherOptions(chain);

    if (!gatherPlugins(pi, chain)) {
        return false;
    }

    // Gather all the calibration targets we're going to try
    // and deal with. NOTE: we're not going to accept targets
    // on different devices for now.
    std::vector<DreamColorSpaceInfo> targets = gatherTargets(chain);

    printf("%d targets to calibrate\n", (int)targets.size());
    if (targets.size() == 0) {   
        return true;
    }

    // Run over all the targets and record the initial state of 
    // the preset for the displays we're going to touch
    for (std::vector<DreamColorSpaceInfo>::iterator theTarget = targets.begin();
            theTarget != targets.end(); ++theTarget) {
        std::map<uint32_t, uint32_t>::iterator thePreset;

        thePreset = origPresets.find((*theTarget).getConnId());
        if (thePreset == origPresets.end()) {
            uint32_t csIdx;

            if (pi.disp->getColorSpace(csIdx, (*theTarget).getConnId(), chain)) {
                origPresets[(*theTarget).getConnId()] = csIdx;
            }
        }
    }


    // Run over all our targets. If we have multiple targets, we don't need
    // to re-measure the gray ramp each time. Just do it once and that should
    // be sufficient.
    std::map<double, double> grayRampR, grayRampG, grayRampB;
    for (uint32_t targetIdx=0; targetIdx<targets.size(); ++targetIdx) {

        if (wasCancelled(pi, chain, false)) return false;

        if (chain) {
            char buf[1024];
            
#ifdef _WIN32
            sprintf_s(buf, 1023, "Calibrating target %d of %d", targetIdx+1, (int)targets.size());
#else
            sprintf(buf, "Calibrating target %d of %d", targetIdx+1, (int)targets.size());
#endif
            chain->setUiString("Ui::status_string_major", std::string(buf));
            chain->setUiString("Ui::status_string_minor", std::string(""));
        }

        // Set the target primaries + white in the display
        if (chain) {
            chain->setUiYxy("Ui::target_red_Yxy",   targets[targetIdx].getRed());
            chain->setUiYxy("Ui::target_green_Yxy", targets[targetIdx].getGreen());
            chain->setUiYxy("Ui::target_blue_Yxy",  targets[targetIdx].getBlue());
            chain->setUiYxy("Ui::target_white_Yxy", targets[targetIdx].getWhite());
        }


        // Restruct data for the approprate format
        calib.setColorSpaceInfo(targets[targetIdx]);

        pi.dispId  = targets[targetIdx].getConnId();
        
        goalWhite  = targets[targetIdx].getWhite();

        // Switch into the color space if interest
        if (!pi.disp->setColorSpace(targets[targetIdx].getPresetId(), pi.dispId)) {
            setErrorString( std::string("Unable to select calibrated color space. ") +
                                pi.disp->errorString() );
            return false;
        }
        if (!idle(5, chain)) {
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (wasCancelled(pi, chain, false)) {
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        // Disable color processing
        if (!pi.disp->setColorProcessingEnabled(false, pi.dispId)) {
            setErrorString( pi.disp->errorString() );
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

    
        if (chain) {
            chain->setUiString("Ui::status_string_minor", "Enabling pattern generator.");
        }

        // Turn on the pattern generator - take care to disable
        //       it when we return in this method then.
        if (!pi.disp->setPatternGeneratorEnabled(true, pi.dispId)) {
            setErrorString("ERROR: Can't enable pattern generator.");

            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }
        printf("Waiting %d seconds...\n",
                    pi.disp->patternGeneratorEnableTime(pi.dispId));

        if (!idle(pi.disp->patternGeneratorEnableTime(pi.dispId), chain)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, NULL);
            }
            return false;
        }

        if (wasCancelled(pi, chain, true)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (chain) {
            chain->setUiString("Ui::status_string_minor", "Calibrating backlight.");
        }

        // Run the backlight processing loop + hang onto the registers
        if (!backlightLoop(pi, chain, goalWhite, backlightReg,
                                               pi.panelWhite)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }
        calib.setRegP0(backlightReg[0]);
        calib.setRegP1(backlightReg[1]);
        calib.setRegP2(backlightReg[2]);
        calib.setRegBrightness(backlightReg[3]);

        if (wasCancelled(pi, chain, true)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (chain) {
            chain->setUiString("Ui::status_string_minor", 
                                  "Measuring primaries for matrix correction.");
        }

        // Measure the primaries + find the native NPM
        if (!measurePrimaries(pi,        chain,
                              pi.panelRed,
                              pi.panelGreen, 
                              pi.panelBlue)) { 
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (wasCancelled(pi, chain, true)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (!pi.color->computeNpm(pi.panelRed,  
                                  pi.panelGreen, 
                                  pi.panelBlue, 
                                  pi.panelWhite,
                                  panelNpm)) {
            setErrorString("Unable to compute native NPM.");
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        printf("Actual white: %f %f %f\n", pi.panelWhite.Y,
                    pi.panelWhite.x, pi.panelWhite.y);

        // Compute the 3x3
        if (!compute3x3CalibMatrix(pi, calib, panelNpm)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (wasCancelled(pi, chain, true)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        // Measure the gray ramp
        if (targetIdx == 0) {
            if (!measureGrayRamp(pi, 
                                 chain,
                                 calib,
                                 panelNpm, 
                                 mNumGraySamples,
                                 grayRampR, 
                                 grayRampG, 
                                 grayRampB, 
                                 calib.getMatrix())) {
                pi.disp->setColorProcessingEnabled(true, pi.dispId);
                pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
                idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
                for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                        thePreset != origPresets.end(); ++thePreset) {
                    pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
                }
                    return false;
            }
        }

        if (wasCancelled(pi, chain, true)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        // Turn off the pattern generator
        if (!pi.disp->setPatternGeneratorEnabled(false, pi.dispId)) {

            setErrorString( std::string("ERROR: Can't disable pattern generator: ") + 
                                pi.disp->errorString());

            pi.disp->setColorProcessingEnabled(true, pi.dispId);

            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }

            // If this happens, well, that sucks. I'm not sure we
            // can recover without power-cycling
            return false;
        }
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);


        computeLuts(pi, calib, grayRampR, grayRampG, grayRampB, panelNpm);

        if (wasCancelled(pi, chain, false)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);

            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        // Upload!
        if (!pi.disp->setCalibration(targets[targetIdx].getPresetId(),
                                                 calib, pi.dispId, chain)) {

            setErrorString( std::string("Unable to upload calibration data: ") + 
                                pi.disp->errorString());

            pi.disp->setColorProcessingEnabled(true, pi.dispId);

            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (wasCancelled(pi, chain, false)) {
            pi.disp->setColorProcessingEnabled(true, pi.dispId);

            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        //  Enable color processing somehow and plop us back
        //  in the right color space
        if (!pi.disp->setColorProcessingEnabled(true, pi.dispId)) {
            setErrorString(pi.disp->errorString());

            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (wasCancelled(pi, chain, false)) {
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }


        // For some reason, we end up in a wierd state on some systems if we don't
        // re-select the current color space. But just re-selecting does not 
        // seem to be sufficent. We first need to flip the pattern generator
        // on and off. How odd? Yes indeed.

#if 1
        // Begin workaround:

        if (!pi.disp->setPatternGeneratorEnabled(true, targets[targetIdx].getConnId())) {
            setErrorString("ERROR: Can't enable pattern generator.");
            return false;
        }
        if (!idle(pi.disp->patternGeneratorEnableTime(pi.dispId), chain)) {
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            return false;
        }
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);


        printf("Re-selecting color space\n");
        if (!pi.disp->setColorSpace(targets[targetIdx].getPresetId(), pi.dispId)) {
            setErrorString( std::string("Unable to select calibrated color space. ") +
                                pi.disp->errorString() );
            return false;
        }
        if (!idle(5, chain)) {
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        // End workaround: 
#else
        fprintf(stderr, "\tStuck luminance workaround disabled!!\n");
#endif

        if (chain) {
            chain->setUiString("Ui::status_string_minor", "Verifing calibration.");
        }

        // Reset any other Luts that sit in the way.
        if (!pi.lut->reset()) {
            setErrorString(pi.lut->errorString());
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (!validate(pi, targets[targetIdx], chain)) {
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }

        if (wasCancelled(pi, chain, false)) {
            for (std::map<uint32_t, uint32_t>::iterator thePreset=origPresets.begin();
                    thePreset != origPresets.end(); ++thePreset) {
                pi.disp->setColorSpace((*thePreset).second, (*thePreset).first, chain);
            }
            return false;
        }
    }


    return true;
}

// -------------------------------------
//
// virtual protected
bool
Ookala::DreamColorCalib::_checkDeps()
{
    std::vector<Plugin *> plugins;

    if (mRegistry == NULL) {
        setErrorString("No PluginRegistry found for DreamColorCalib.");
        return false;
    }

    plugins = mRegistry->queryByName("DreamColorCtrl");
    if (plugins.empty()) {
        setErrorString("No DreamColorCtrl plugin found for DreamColorCalib.");
        return false;
    }

    plugins = mRegistry->queryByName("WsLut");
    if (plugins.empty()) {
        setErrorString("No WsLut plugin found for DreamColorCalib.");
        return false;
    }

    return true;
}


// -------------------------------------
//
// Before discussing how to find a good configuration for the 
// backlight, it's worthwhile to touch upon what /is/ a good
// configuration for the backlight.
//
// With this display, the goal is not necessarily to just minimize
// dE of the native backlight Yxy w.r.t. the target Yxy. It's 
// entirely possible that even with a very small dE, it would 
// still be impossible to reach the goal Yxy.
//
// The reason is headroom. Recall that the backlight is not 
// the sole determinate of white point - the rest of the color
// processing chain comes into play. This is handy for the 
// really fine tweaking to get close to the goal Yxy.
//
// Consider the 3x3 matrix that maps target colorimetry onto
// measured colorimetry. For mapping an incoming (1,1,1) value
// all the components of the output value must be less than 1 - 
// otherwise we're clipping. In other words, if we measure
// the panel's native RGB chromaticies and build an NPM 
// matrix using these and the backlight w.p (NPM = RGB -> XYZ)
// we'll see:
//
//          [R]    [       ]^-1  [Target White X]
//          [G] =  [  NPM  ]     [Target White Y]
//          [B]    [       ]     [Target White Z]
//
// (XXX: An example would be good here).
//
// If the resulting R,G,B > 1, we're out of luck.
//
// So, a valid backlight white point must allow for the mapping of
// the target XYZ back to RGB, where max(R,G,B) <= 1.0.
//
// Now, that said, we don't want to waste bits. So we also want
// to make sure that min(R,G,B) >= some threshold. 
//
// Combining these two conditions, we can classify a "solution" as
// a backlight white point where max(R,G,B) <= 1.0 and 
// min(R,G,B) >= T, where T is some threshold and R,G,B result
// from the target XYZ and the NPM matrix formed by the backlight
// white point and the native panel chromaticies.
//
// Ok. So that's a solution. But what's a _good_ solution, and how
// to we find it?
//
// First, we need to give ourselves a better shot at finding a
// backlight w.p with max(R,G,B) <= 1.0. This means adding in a little
// headroom to the luminance goal we're trying to hit. In our 
// search, we're not really searching for the target w.p., but
// rather the target w.p. plus some Y headroom - say, 2% headroom.
//
// Now - if we have a target with some headroom, this can become
// the object of our dE affection. We'll drive towards this goal,
// and we'll stop if we find a valid solution that has a really 
// good dE (where really good is some threshold that you pick).
//
// If we've searched for a fair number of iterations, but haven't
// found a really good dE, we should stop and not run forever.
// But, if we haven't found a really good dE, what should our
// backlight white be? It makes sense to pick the best (in a dE sense)
// "valid solution" that we found over the course of the iterations.
// We'll then force the matrix to re-balance to the eventual target w.p.
// If we haven't found any valid solutions, with max(R,G,B) <= 1.0 and
// min(R,G,B) >= T), then we're out of luck and we're going to fail.
// Alternativly, if this ends up being a problem, increase the max
// number of iterations, or increase them only if we don't have any
// valid solutions.
//
// The backlight has a set of 4 registers that control it. One register
// is 'brightness' and is tied to the OSD brightness control. It's 
// a 1-byte register. At 0xff, the OSD reads out 250 cd/m^2. At 
// 0x51, the OSD reads out 50 cd/m^2. Note that the scaling is not
// quite 1 code value / cd/m^2, so we should manually figure out the
// OSD setting that maps to a goal luminance. The remaining 3 backlight
// registers control (approximately) Y, x, and y. There is some cross
// talk. For the x and y controls, there's approx. .001 change per
// code value. For the Y control, there's approx 1 cd/m^2 change per
// code value when brightness = 0xff.
//
// To try and approach a good solution, we iterate over adjusting
// the Y, x, and y, backlight registers. To figure how far to
// move, we keep a running estimate of how much change per register
// code value we hit. Then, using this, we estimate how far to 
// adjust each register. From this, we move the register with the
// largest step, and continue the process.
//
// It's fairly brain-dead, but it usually converges fairly quickly.
// We have experimented with gradient-descent type stratagies, but
// they're often rather sensitive to measurement noise - which has
// a tendency to send us off in the wrong direction and diverge.
//
//
// NOTE: Color processing should be disabled before entering this function.
//
// NOTE: Pattern generator should be enabled before entering.
//
// virtual protected
bool 
Ookala::DreamColorCalib::backlightLoop(
                               PluginData     &pi,
                               PluginChain    *chain,
                               const Yxy      &goalWhite,    
                               uint32_t       *backlightReg,
                               Yxy            &currWhite)
{
    uint32_t               lastReg[4], origReg[4];
    Yxy                    lastWhite, red, green, blue;
    Yxy                    headroomGoal;
    Rgb                    linearRgb;
    int32_t                chromaRegStep[3];    
    double                 diff[3];

    DataPoint              point;
    std::vector<DataPoint> solutions;

    bool                   convergedOnGoal   = false;
                                 
                                 
    // ---------------------------------------------------------
    // Tweakable parameters:
    //

    // We should consider ourselves good enough if a solution
    // is within some threshold of a dE from our headroom goal.
    double   dEStoppingThreshold = 1;

    // If we don't converge because of small dE, we should stop
    // after some fixed number of attempts. 
    uint32_t maxIter           = 12;

    // The amount of luminance headroom to start aiming for.
    double initialHeadroom = 1.02;

    // The current lower threshold on the linear RGB of our 
    // goalYxy - given the native RGB of the panel and the current
    // white of the backlight - is 2.0x the difference from 1 given
    // 2% headroom. We'll update this below to adapt if 
    // the headroom increases.
    double lowerLinearRgbThresh = 1. -  2.0*(initialHeadroom - 1.);

    double upperLinearRgbThresh = 1. - 0.50*(initialHeadroom - 1.);

    // One step in the Y register is appromatly a change of
    // 1 cd/m^2 at max brightness. Lets assume that it scales
    // linearly as brighteness decreases. These are only
    // initial estimates. As we take steps below, we'll update
    // these estimates to reflect reality. It won't be perfect,
    // thanks to noise and cross-channel issues (e.g. we're 
    // really have dX/dRegister, dY/dRegister, dZ/dRegister), 
    // but in practice it works fairly well.
    double unitsPerStep[3];

    //
    // ---------------------------------------------------------

    unitsPerStep[0] = goalWhite.Y / 250.0;
    unitsPerStep[1] = 1./1000.;
    unitsPerStep[2] = 1./1000.;


    // The color we _actually_ want to hit in this loop has
    // some headroom in it.
    // 
    // For now, we'll pick a fixed % of the goal Y
    headroomGoal.Y = initialHeadroom * goalWhite.Y;
    headroomGoal.x = goalWhite.x;
    headroomGoal.y = goalWhite.y;


    // Enforce some limits on the goals. We probably don't want to
    // dip much below 40 cd/m^2, and probably not much above 250 cd/m^2
    if ((goalWhite.Y < 40) || (goalWhite.Y > 250)) {
        setErrorString("Target luminance is out of range [40,250].");
        return false;
    }

    if (!measurePrimaries(pi, chain, red, green, blue)) {
        return false;
    }

    if (!pi.disp->setPatternGeneratorColor(1023, 1023, 1023, pi.dispId)) {
        setErrorString( std::string("Can't set pattern generator color: ") +
                                pi.disp->errorString());
        return false;
    }
    if (!idle(pi.disp->patternGeneratorSettleTime(pi.dispId), chain)) {
        return false;
    }
        
    if (wasCancelled(pi, chain, true)) return false;


    // It's probably best to start the backlight registers in
    // their current position (except for a change in the
    // brightness register.
    if (!pi.disp->getBacklightRegRaw(origReg[0], origReg[1],
                                origReg[2], origReg[3], pi.dispId)) {
        setErrorString( std::string("Can't read initial backlight registers.") +
                            pi.disp->errorString());
        return false;
    }

    lastReg[0] = backlightReg[0] = origReg[0]; 
    lastReg[1] = backlightReg[1] = origReg[1];
    lastReg[2] = backlightReg[2] = origReg[2];
    lastReg[3] = backlightReg[3] = pi.disp->brightnessRegister(goalWhite.Y, pi.dispId);

    // Set an initial position for the backlight registers
    if (!pi.disp->setBacklightRegRaw(backlightReg[0], backlightReg[1],
                                 backlightReg[2], backlightReg[3],
                                 true, true, true, true, pi.dispId)) {
        setErrorString( std::string("Can't set backlight registers: ") +
                            pi.disp->errorString());
        return false;
    }

    if (wasCancelled(pi, chain, true)) return false;

    // Wait for the backlight to settle - we could be smarter about
    // this by examining the current values. It takes ~15-20 sec to 
    // settle over a min -> max brightness swing. But, if we're
    // not swinging that long, no need to wait that long.
    if (!idle(pi.disp->backlightBrightnessSettleTime(pi.dispId), chain)) {
        return false;
    }

    for (uint32_t mainIter=0; mainIter<maxIter; ++mainIter) {

        if (wasCancelled(pi, chain, true)) return false;

        // Measure where we are
        lastWhite = currWhite;

        if (!pi.sensor->measureYxy(currWhite, chain, pi.sensorId)) {
            setErrorString( std::string("Can't read sensor: ") +
                            pi.sensor->errorString());
            return false;
        }

        if (wasCancelled(pi, chain, true)) return false;

        // If our headroom has increased, we'll need to set our lower
        // tolerance of linear RGB accordingly. This sets it to 
        // 2x the range of being dead on the current head room
        if (!computeGoalRgb(pi.color, red, green, blue,
                        headroomGoal, goalWhite, linearRgb)) {
            return false;
        }

        lowerLinearRgbThresh = 1.0 - 2.0*(1.0-linearRgb.r);

        printf(
         "If dead on headroom goal, real goal rgb: %f [thresh: %f - %f]\n",
                linearRgb.r, upperLinearRgbThresh, lowerLinearRgbThresh);



        if (!computeGoalRgb(pi.color, red, green, blue,
                                currWhite, goalWhite, linearRgb)) {
            return false;
        }

        double rgbDat[3];

        rgbDat[0] = linearRgb.r;
        rgbDat[1] = linearRgb.g;
        rgbDat[2] = linearRgb.b;

        int minRgb = 0;
        int maxRgb = 0;
        for (int idx=1; idx<3; ++idx) {
            if (rgbDat[idx] > rgbDat[maxRgb]) maxRgb = idx;
            if (rgbDat[idx] < rgbDat[minRgb]) minRgb = idx;
        }
        printf("Linear RGB: %f %f %f\n", linearRgb.r, linearRgb.g, linearRgb.b);

        // If we have enough head room (that is, all RGB < 1) but yet
        // aren't wasting too may bits (min RGB is too low), then
        // stop because we'll fix the rest in the 3x3 matrix.
        //
        // The min threshold probably shouldn't be fixed - we should
        // look at what it would be if we hit our headroom goal 
        // dead-nuts on, and then back off a little.
        if ((rgbDat[maxRgb] <= upperLinearRgbThresh) && 
                        (rgbDat[minRgb] > lowerLinearRgbThresh)) {

            // If we've found a valid solution which is pretty darn
            // close to where we want to be, go ahead and stop.
            // Otherwise, just record the solution so we can
            // find the best one later on if we get stuck

            point.linearRgb   = linearRgb;
            point.measuredYxy = currWhite;
            for (int idx=0; idx<4; ++idx) {
                point.backlightReg[idx] = backlightReg[idx];
            }
            solutions.push_back(point);

            if (pi.color->dELab(headroomGoal, currWhite, 
                                headroomGoal) < dEStoppingThreshold) {
                printf("Converged because dE < %f\n", dEStoppingThreshold);
                convergedOnGoal = true;
                break;
            }
        }
            
        printf("%4d %4d %4d %4d    %f %f %f\n", 
                backlightReg[0], backlightReg[1], backlightReg[2], backlightReg[3],
                currWhite.Y, currWhite.x, currWhite.y);


        // And re-estimate how much the display changes with response
        // to a given adjustment in value. Only update our estimate
        // if its a sane value thought.

        diff[0] = currWhite.Y - lastWhite.Y;
        diff[1] = currWhite.x - lastWhite.x;
        diff[2] = currWhite.y - lastWhite.y;

        for (int idx=0; idx<3; ++idx) {
            if (backlightReg[idx] != lastReg[idx]) {

                double dx     = (double)backlightReg[idx] - (double)lastReg[idx];

                double change = diff[idx] / dx;

                // Limit the change to .1 Y change / unit for luminance
                // and .0005/unit in the x,y registers.
                if ((idx == 0) && (change > .1)) {
                     unitsPerStep[idx] = change;
                } else if ((idx > 0) && (change > 5e-4)) {
                     unitsPerStep[idx] = change;
                }
            }
        }

        // Approximate which direction should move the farthest

        diff[0] = headroomGoal.Y - currWhite.Y;
        diff[1] = headroomGoal.x - currWhite.x;
        diff[2] = headroomGoal.y - currWhite.y;

        chromaRegStep[0] = chromaRegStep[1] = chromaRegStep[2] = 0;
        int maxIdx = -1;
        for (int idx=0; idx<3; ++idx) {
            chromaRegStep[idx] =  static_cast<int32_t>(diff[idx] / unitsPerStep[idx] + .5);                                 
            if (chromaRegStep[idx] != 0) {

                if (maxIdx == -1) {                 
                    maxIdx = idx;
                } else if ( abs(chromaRegStep[idx]) > abs(chromaRegStep[maxIdx])) {
                    maxIdx = idx;
                }
            }
        }

        printf("\t\t\t\t\tchroma reg step: %d %d %d [%.2f %.4f %.4f]\n",
                         chromaRegStep[0], chromaRegStep[1], chromaRegStep[2],
                         headroomGoal.Y - currWhite.Y,
                         headroomGoal.x - currWhite.x,
                         headroomGoal.y - currWhite.y);
        printf("\t\t\t\t\tdE: %f\n", 
            pi.color->dELab(headroomGoal, currWhite, headroomGoal));

        // If we got stuck and we're still not in a good RGB spot,
        // then what? Raise the headroom...
        if (maxIdx == -1) {
            //printf("Converged because of lack of movement.. breaking for success!\n");
            //break;
            printf("It appears that we got stuck - increasing headroom\n");
            headroomGoal.Y += 0.01*goalWhite.Y;
        }

       // Only move in the most important direction.
        for (uint32_t idx=0; idx<3; ++idx) {
            lastReg[idx]       = backlightReg[idx];
        }
        if (maxIdx != -1) {
            // If we're already close to our goal, be conservative in
            // our step size so we don't overshoot
            if (pi.color->dELab(headroomGoal, currWhite, headroomGoal) < 2) {
                backlightReg[maxIdx] += chromaRegStep[maxIdx] / 
                                        abs(chromaRegStep[maxIdx]);
            } else {
                backlightReg[maxIdx] += chromaRegStep[maxIdx];
            }
        }

        // These should probably have better upper bounds?
        if ((backlightReg[0] < 0) || (backlightReg[0] >  400) || //was 350
            (backlightReg[1] < 0) || (backlightReg[1] > 2000) ||
            (backlightReg[2] < 0) || (backlightReg[2] > 2000)) {
            fprintf(stderr, "ERROR: Backlight registers out of range 0<%d<350 0<%d<2000 0<%d<2000\n", backlightReg[0], backlightReg[1], backlightReg[2]);
            setErrorString("Backlight registers out of range.");
            return false;
        }

        // Update the backlight registers
        if (!pi.disp->setBacklightRegRaw(backlightReg[0], backlightReg[1],
                                     backlightReg[2], backlightReg[3],
                                     true, true, true, false)) {
            fprintf(stderr, "ERROR: Can't set backlight registers\n");
            setErrorString( std::string("Can't set backlight register: ") + 
                                pi.disp->errorString());
            return false;
        }

        if (wasCancelled(pi, chain, true)) return false;

        if (!idle(pi.disp->backlightGenericSettleTime(pi.dispId), chain)) {
            return false;
        }
    }

    // If we didn't happen to stop because we found a good point, 
    // hopefully we at least found some valid solutions (where the
    // linearRGB of our goal point was within our tolerance. If we
    // did find some of these "solution" points, pick the one with
    // the lowest dE to the real goal (not the goal with headroom).
    // If we didn't find any solutions, then we have to fail.
    if (!convergedOnGoal) {

        if (solutions.empty()) {
            setErrorString("No solutions found.");
            return false;
        }    

        std::vector<DataPoint>::iterator bestSoln = solutions.begin();
        for (std::vector<DataPoint>::iterator soln = solutions.begin();
                soln != solutions.end(); ++soln) {

            if (pi.color->dELab(goalWhite, (*soln).measuredYxy, goalWhite) <
                pi.color->dELab(goalWhite, (*bestSoln).measuredYxy, goalWhite)) {
                bestSoln = soln;
            }
        }
        
        if (!pi.disp->setBacklightRegRaw((*bestSoln).backlightReg[0], 
                                     (*bestSoln).backlightReg[1],
                                     (*bestSoln).backlightReg[2], 0,
                                     true, true, true, false, pi.dispId)) {
            fprintf(stderr, "ERROR: Can't set backlight registers\n");
            setErrorString( std::string("Can't set backlight register: ") +
                        pi.disp->errorString());
            return false;
        }
        if (!idle(pi.disp->backlightGenericSettleTime(pi.dispId), chain)) {
            return false;
        }

    }

    return true;
}

// ------------------------------------------------
//
// NOTE: Pattern generator should be enabled before
//        entering this function.
//
// virtual
bool 
Ookala::DreamColorCalib::measurePrimaries(
                                  PluginData      &pi, 
                                  PluginChain     *chain,
                                  Yxy              &red,
                                  Yxy              &green,
                                  Yxy              &blue)
{
    // Setup for measuring red
    if (!pi.disp->setPatternGeneratorColor(1023, 0, 0, pi.dispId)) {
        setErrorString("ERROR: Can't set pattern generator color.");
        return false;
    }
    if (!idle(pi.disp->patternGeneratorSettleTime(pi.dispId), chain)) {
        return false;
    }
    if (wasCancelled(pi, chain, true)) return false;


    // Measure red
    if (!pi.sensor->measureYxy(red, chain, pi.sensorId)) {
                                            
        setErrorString( std::string("Can't read sensor: ") + 
                                pi.sensor->errorString());
        return false;
    }
    if (wasCancelled(pi, chain, true)) return false;

    // Setup for measuring green
    if (!pi.disp->setPatternGeneratorColor(0, 1023, 0, pi.dispId)) {
        fprintf(stderr, "ERROR: Can't set pattern generator color\n");
        return false;
    }
    if (!idle(pi.disp->patternGeneratorSettleTime(pi.dispId), chain)) {
        return false;
    }
    if (wasCancelled(pi, chain, true)) return false;



    // Measure green
    if (!pi.sensor->measureYxy(green,  chain,       pi.sensorId)) {
        setErrorString( std::string("Can't read sensor: ")  +
                    pi.sensor->errorString());
        return false;
    }
    if (wasCancelled(pi, chain, true)) return false;


    // Setup for measuring blue
    if (!pi.disp->setPatternGeneratorColor(0, 0, 1023, pi.dispId)) {
        fprintf(stderr, "ERROR: Can't set pattern generator color\n");
        return false;
    }
    if (!idle(pi.disp->patternGeneratorSettleTime(pi.dispId), chain)) {
        return false;
    }
    if (wasCancelled(pi, chain, true)) return false;



    // Measure blue
    if (!pi.sensor->measureYxy(blue, chain,  pi.sensorId)) {
        setErrorString( std::string("Can't read sensor: ") + 
                            pi.sensor->errorString());
        return false;
    }
    if (wasCancelled(pi, chain, true)) return false;

    return true;
}

// ------------------------------------------------
//
// NOTE: Pattern generator should be enabled and 
//       color processing disabled.
//
// We'll take the raw grey measurements and conver them 
// back to panel RGB. Then, we'll store the panel RGB
//  (linear, not nonlinear) in the maps ramp{R,G,B}.
//
// This won't measure white - because presumable we've
// already done that to get the XYZ->RGB matrix. And
// if we process that value, then we're just going
// to get 1.0, so there isn't much point!!
//
// virtual protected
bool 
Ookala::DreamColorCalib::measureGrayRamp(
                                 PluginData                &pi,
                                 PluginChain               *chain,
                                 DreamColorCalibrationData &calib,
                                 const Npm                 &panelNpm, 
                                 uint32_t                   numSteps,
                                 std::map<double, double>  &rampR,
                                 std::map<double, double>  &rampG,
                                 std::map<double, double>  &rampB,
                                 Mat33                      calibMat) 
{
    std::vector<uint32_t> grayVals;

    int32_t  val, max;
    uint32_t stepSize;
    Yxy      adjMeasure, black;
    Yxy      measure;
    Rgb      linearRgb;

    // Maximum code value
    max = (1 << pi.disp->patternGeneratorBitDepth(pi.dispId)) - 1;
 
    // Somehow need to set this based on user data.
    stepSize = (max) / (numSteps-1);

    // Don't even bother trying to read black, just go ahead and
    // estimate what it should be.
    if (!estimateBlack(pi, chain, pi.panelRed, 
                                  pi.panelGreen, 
                                  pi.panelBlue,
                                  black)) {
        black.Y = pi.panelWhite.Y / 1100.0;
        black.x = 0.3333;
        black.y = 0.3333;
    }

    printf("black: %f %f %f\n", black.Y, black.x, black.y);


    rampR[0.0] = 0.0;
    rampG[0.0] = 0.0;
    rampB[0.0] = 0.0;

    // Don't bother pushing white, just put it where its' supposed to go.
    //grayVals.push_back(max);
    rampR[1.0] = 1.0;
    rampG[1.0] = 1.0;
    rampB[1.0] = 1.0;


    
    // If we have a sane matrix, figure out where 'white' is going
    // to land, and make sure we sample in that area.
    Vec3 colorVec;
    colorVec.v0 = colorVec.v1 = colorVec.v2 = 1.0;

    Vec3 colorVecAdj = pi.color->mat33VectorMul(calibMat, colorVec);

    colorVecAdj.v0 = (double)((uint32_t)(max*colorVecAdj.v0 + 0.5));
    colorVecAdj.v1 = (double)((uint32_t)(max*colorVecAdj.v1 + 0.5));
    colorVecAdj.v2 = (double)((uint32_t)(max*colorVecAdj.v2 + 0.5));

    // If someone has passed in an idenity matrix, don't worry
    // about measuring, because we'll catch the max value below.
    if (colorVecAdj.v1 < max-5) {
        
        val = static_cast<uint32_t>(colorVecAdj.v1);
        grayVals.push_back(val);
    }

    val = max-stepSize;
    while (val > 0) {
        if (val > 24) {
            grayVals.push_back(val);
        }

        val -= stepSize;
    }

    std::sort(grayVals.begin(), grayVals.end());
    std::reverse(grayVals.begin(), grayVals.end());

    uint32_t grayCnt = 1;

    for (std::vector<uint32_t>::iterator theGray = grayVals.begin();
                theGray != grayVals.end(); ++theGray) {

        if (chain) {
            char buf[1024];

#ifdef _WIN32
            sprintf_s(buf, 1023, "Measuring gray ramp, %d of %d", 
                            (int)grayCnt, (int)grayVals.size());
#else
            sprintf(buf, "Measuring gray ramp, %d of %d", 
                            (int)grayCnt, (int)grayVals.size());
#endif
            chain->setUiString("Ui::status_string_minor", std::string(buf));
        }

        val = *theGray;

        if (!pi.disp->setPatternGeneratorColor(val, val, val, pi.dispId)) {
            setErrorString("ERROR: Can't set pattern generator color.");
            return false;
        }
        if (wasCancelled(pi, chain, true)) return false;
        if (!idle(pi.disp->patternGeneratorSettleTime(pi.dispId), chain)) {
            return false;
        }

        
        // If this fails, it's probably because we're too dark for
        // the sensor to measure. If we already have some data, this
        // is probably ok.
        if (!pi.sensor->measureYxy(measure, chain, pi.sensorId)) {

            if (rampR.size() < 6) {
                setErrorString( std::string("Can't read sensor: ") + 
                                    pi.sensor->errorString());
                return false;
            } else {
                return true;
            }
        }
        if (wasCancelled(pi, chain, true)) return false;

        adjMeasure = pi.color->subYxy(measure, black);

        linearRgb = pi.color->cvtYxyToRgb(adjMeasure, panelNpm);

        printf("%d -> %f %f %f [%f %f %f]\n", 
                val, adjMeasure.Y, adjMeasure.x, adjMeasure.y,
                    linearRgb.r, linearRgb.g, linearRgb.b);

        rampR[ static_cast<double>(val) / static_cast<double>(max) ] = linearRgb.r;
        rampG[ static_cast<double>(val) / static_cast<double>(max) ] = linearRgb.g;
        rampB[ static_cast<double>(val) / static_cast<double>(max) ] = linearRgb.b;

        grayCnt++;
    }

    return true;
}

// ------------------------------------------------
//
// Call this assuming that the pattern generator is
// enabled
//
// protected
bool
Ookala::DreamColorCalib::estimateBlack(
                               PluginData  &pi,
                               PluginChain *chain,
                               const Yxy   &measuredRed,
                               const Yxy   &measuredGreen,
                               const Yxy   &measuredBlue,
                               Yxy         &estimatedBlack)
{
    std::vector<uint32_t> targetPnts;
    std::vector<Yxy>      rampRed, rampGreen, rampBlue;
    bool                  readRed, readGreen, readBlue;
    Yxy                   curr, newBlack[6];
    XYZ                   tmpXYZ, estimatedBlackXYZ, grad;
    double                err[6], step, delta, currErr, lastErr, len;
    int32_t               count;

    // Maximum code value
    uint32_t max = (1 << pi.disp->patternGeneratorBitDepth(pi.dispId)) - 1;
 
   for (int32_t i=static_cast<int32_t>(0.2*max); 
                                 i>static_cast<int32_t>(0.15*max); i-=20) {
        targetPnts.push_back(i);
    }

    readRed = readGreen = readBlue = true;
    count   = 0;
    for (std::vector<uint32_t>::iterator theVal = targetPnts.begin();
                    theVal != targetPnts.end(); ++theVal) {
        if (chain) {
            char buf[1024];

#ifdef _WIN32
            sprintf_s(buf, 1023, "Estimating black point, pass %d of %d",
                        count+1, static_cast<int>(targetPnts.size()));
#else
            sprintf(buf, "Estimating black point, pass %d of %d",
                        count+1, static_cast<int>(targetPnts.size()));
#endif
            chain->setUiString("Ui::status_string_minor", std::string(buf));
        }
        count++;


        printf("Measuring rgb for %d\n", (*theVal));
        // Measure the red.
        if (readRed) {
            if (!pi.disp->setPatternGeneratorColor((*theVal), 0, 0, pi.dispId)) {
                setErrorString("ERROR: Can't set pattern generator color.");
                return false;
            }

            if (wasCancelled(pi, chain, true)) return false;

            if (!idle(pi.disp->patternGeneratorSettleTime(pi.dispId), chain)) {
                return false;
            }

printf("Reading red..\n");
            if (!pi.sensor->measureYxy(curr, chain, pi.sensorId)) {
                readRed = false;
printf("Failed to read red %d: %s\n", (*theVal), pi.sensor->errorString().c_str());
            } else {
printf("Read red %d ok (%f %f %f)\n", (*theVal), curr.x, curr.y, curr.Y);
                rampRed.push_back(curr);
            }
        }

        // Measure the green.
        if (readGreen) {
            if (!pi.disp->setPatternGeneratorColor(0, (*theVal), 0, pi.dispId)) {
                setErrorString("ERROR: Can't set pattern generator color.");
                return false;
            }

            if (wasCancelled(pi, chain, true)) return false;

            if (!idle(pi.disp->patternGeneratorSettleTime(pi.dispId), chain)) {
                return false;
            }
printf("Reading green..\n");
            if (!pi.sensor->measureYxy(curr, chain, pi.sensorId)) {
                readGreen = false;
printf("Failed to read green %d: %s\n", (*theVal), pi.sensor->errorString().c_str());
            } else {
                rampGreen.push_back(curr);
printf("Read green %d ok (%f %f %f)\n", (*theVal), curr.x, curr.y, curr.Y);


            }
        }

        // And measure the blue.
        if (readBlue) {
            if (!pi.disp->setPatternGeneratorColor(0, 0, (*theVal), pi.dispId)) {
                setErrorString("ERROR: Can't set pattern generator color.");
                return false;
            }

            if (wasCancelled(pi, chain, true)) return false;

            if (!idle(pi.disp->patternGeneratorSettleTime(pi.dispId), chain)) {
                return false;
            }

printf("Reading blue..\n");

            if (!pi.sensor->measureYxy(curr, chain, pi.sensorId)) {
                readBlue = false;
printf("Failed to read blue %d: %s\n", (*theVal), pi.sensor->errorString().c_str());
            } else {
                rampBlue.push_back(curr);
printf("Read blue %d ok (%f %f %f)\n", (*theVal), curr.x, curr.y, curr.Y);


            }
        }

        if ((!readRed) && (!readGreen) && (!readBlue)) break;
    }
    
    if (rampRed.size() + rampGreen.size() + rampBlue.size() == 0) {
        return false;
    }

    estimatedBlack.x = 0.333;
    estimatedBlack.y = 0.333;
    estimatedBlack.Y = 0.04;

    step   =  0.001;
    delta  = 0.0001;
    grad.X = grad.Y = grad.Z = 0;

    lastErr = 1e60;

    for (uint32_t iter=0; iter<2500; ++iter) {

   
        for (int idx=0; idx<6; ++idx) {
            tmpXYZ = pi.color->cvtYxyToXYZ(estimatedBlack);
            switch (idx) {
                case 0:
                    tmpXYZ.X += delta;
                    break;
                
                case 1:
                    tmpXYZ.X -= delta;
                    break;

                case 2:
                    tmpXYZ.Y += delta;
                    break;
                
                case 3:
                    tmpXYZ.Y -= delta;
                    break;

                case 4:
                    tmpXYZ.Z += delta;
                    break;
                
                case 5:
                    tmpXYZ.Z -= delta;
                    break;
            }
            newBlack[idx] = pi.color->cvtXYZToYxy(tmpXYZ);
        }

        // Compute the gradient of the error, using central differences
        err[0] = err[1] = err[2] = err[3] = err[4] = err[5] = currErr = 0;

        for (std::vector<Yxy>::iterator theVal = rampRed.begin(); 
                theVal != rampRed.end(); ++theVal) {

            currErr += estimateBlackError(pi, measuredRed, estimatedBlack, *theVal);
            for (int idx=0; idx<6; ++idx) {
                err[idx] += estimateBlackError(pi, measuredRed, newBlack[idx], *theVal);
            }
        }

        for (std::vector<Yxy>::iterator theVal = rampGreen.begin(); 
                theVal != rampGreen.end(); ++theVal) {
            currErr += estimateBlackError(pi, measuredGreen, estimatedBlack, *theVal);
            for (int idx=0; idx<6; ++idx) {
                err[idx] += estimateBlackError(pi, measuredGreen, newBlack[idx], *theVal);
            }
        }

        
        for (std::vector<Yxy>::iterator theVal = rampBlue.begin(); 
                theVal != rampBlue.end(); ++theVal) {
            currErr += estimateBlackError(pi, measuredBlue, estimatedBlack, *theVal);
            for (int idx=0; idx<6; ++idx) {
                err[idx] += estimateBlackError(pi, measuredBlue, newBlack[idx], *theVal);
            }
        }

        if (currErr > lastErr) {
            estimatedBlackXYZ = pi.color->cvtYxyToXYZ(estimatedBlack);
            estimatedBlackXYZ.X -= grad.X;
            estimatedBlackXYZ.Y -= grad.Y;
            estimatedBlackXYZ.Z -= grad.Z;
            estimatedBlack = pi.color->cvtXYZToYxy(estimatedBlackXYZ);

            step  *= 0.5;
            delta *= 0.5;
    
            grad.X = grad.Y = grad.Z = 0;
            continue;
        } 

        if (lastErr - currErr < 1e-10) break;

        lastErr = currErr;
        
        // And take a small step towards the gradient
        grad.X = (err[0] - err[1]) / (2.0*delta);
        grad.Y = (err[2] - err[3]) / (2.0*delta);
        grad.Z = (err[4] - err[5]) / (2.0*delta);

        len = sqrt(grad.X*grad.X + grad.Y*grad.Y + grad.Z*grad.Z);
        
        if (fabs(len) > 1e-6) {
            grad.X = grad.X / len * step;
            grad.Y = grad.Y / len * step;
            grad.Z = grad.Z / len * step;
        }

        estimatedBlackXYZ = pi.color->cvtYxyToXYZ(estimatedBlack);
        estimatedBlackXYZ.X -= grad.X;
        estimatedBlackXYZ.Y -= grad.Y;
        estimatedBlackXYZ.Z -= grad.Z;
        estimatedBlack = pi.color->cvtXYZToYxy(estimatedBlackXYZ);        
    }
         
    return true;
}

// ------------------------------------------------
//

double 
Ookala::DreamColorCalib::estimateBlackError(
                                    PluginData  &pi,
                                    const Yxy   &goal,
                                    const Yxy   &estimatedBlack,
                                    const Yxy   &measurement)
{
    // subtract black from the measurement
    Yxy measMinusBlack = pi.color->subYxy(measurement, estimatedBlack);

    // And compute the chromaticity error
    return  sqrt(pow(measMinusBlack.x - goal.x, 2.0) + 
                 pow(measMinusBlack.y - goal.y, 2.0));
}

// ------------------------------------------------
//
// Computes the 3x3 matrix for calibration, and sets it
// in the CalibrationData, ready for uploading.
//
// This assumes that we've already computed the panel
// XYZ -> rgb matrix (which we would need for the gray
// ramp measurements.
bool 
Ookala::DreamColorCalib::compute3x3CalibMatrix(
                       PluginData                &pi,
                       DreamColorCalibrationData &calib,
                       const Npm                 &panelNpm)
{
    Npm   modelNpm;

    if (!pi.color->computeNpm(calib.getColorSpaceInfo().getRed(),  
                              calib.getColorSpaceInfo().getGreen(),
                              calib.getColorSpaceInfo().getBlue(), 
                              calib.getColorSpaceInfo().getWhite(),
                              modelNpm)) {
        setErrorString("Unable to compute model NPM.");
        return false;
    }

    // Now, the calibration matrix is just:
    //
    //    [       ]     [ panel  ] [ model  ]
    //    [ calib ] =   [ XYZ -> ] [ rgb -> ]
    //    [       ]     [  rgb   ] [   XYZ  ]      
    calib.setMatrix(pi.color->mat33MatMul( panelNpm.invRgbToXYZ,  
                                           modelNpm.rgbToXYZ));

    printf("Model RGB -> XYZ:\n");
    printf("%f %f %f\n", modelNpm.rgbToXYZ.m00,
                         modelNpm.rgbToXYZ.m01,
                         modelNpm.rgbToXYZ.m02);
    printf("%f %f %f\n", modelNpm.rgbToXYZ.m10,
                         modelNpm.rgbToXYZ.m11,
                         modelNpm.rgbToXYZ.m12);
    printf("%f %f %f\n", modelNpm.rgbToXYZ.m20,
                         modelNpm.rgbToXYZ.m21,
                         modelNpm.rgbToXYZ.m22);

    printf("Panel XYZ -> RGB:\n");
    printf("%f %f %f\n", panelNpm.invRgbToXYZ.m00,
                         panelNpm.invRgbToXYZ.m01,
                         panelNpm.invRgbToXYZ.m02);
    printf("%f %f %f\n", panelNpm.invRgbToXYZ.m10,
                         panelNpm.invRgbToXYZ.m11,
                         panelNpm.invRgbToXYZ.m12);
    printf("%f %f %f\n", panelNpm.invRgbToXYZ.m20,
                         panelNpm.invRgbToXYZ.m21,
                         panelNpm.invRgbToXYZ.m22);


    printf("Calib 3x3 matrix:\n");
    printf("%f %f %f [%f]\n", 
            calib.getMatrix().m00,  calib.getMatrix().m01,  calib.getMatrix().m02,
            calib.getMatrix().m00 + calib.getMatrix().m01 + calib.getMatrix().m02);

    printf("%f %f %f [%f]\n", 
            calib.getMatrix().m10,  calib.getMatrix().m11,  calib.getMatrix().m12,
            calib.getMatrix().m10 + calib.getMatrix().m11 + calib.getMatrix().m12);


    printf("%f %f %f [%f]\n", 
            calib.getMatrix().m20,  calib.getMatrix().m21,  calib.getMatrix().m22,
            calib.getMatrix().m20 + calib.getMatrix().m21 + calib.getMatrix().m22);

    return true;

}

// ------------------------------------------------
//
// Compute the pre and post luts, and store them in 
// the CalibrationData struct.
bool
Ookala::DreamColorCalib::computeLuts(
                             PluginData                &pi,
                             DreamColorCalibrationData &calib,
                             std::map<double, double>  &rampR,
                             std::map<double, double>  &rampG,
                             std::map<double, double>  &rampB,
                             const Npm                 &panelNpm)
{
    std::vector<uint32_t> preLut[3], postLut[3];

    // Compute the pre-lut
    for (uint32_t cv=0; cv<pi.disp->getPreLutLength(pi.dispId); ++cv) {
        double max = static_cast<double>(
                        (1 << pi.disp->getPreLutBitDepth(pi.dispId)) - 1);

        double x   = static_cast<double>(cv) /
                     static_cast<double>(pi.disp->getPreLutLength(pi.dispId)-1);

        uint32_t val = static_cast<uint32_t>(
                          0.5 + max *
                             pi.disp->evalTrcToLinear(x, 
                                  calib.getColorSpaceInfo(), pi.dispId));

        preLut[0].push_back(val);
        preLut[1].push_back(val);
        preLut[2].push_back(val);
    }

    // If we want, try and figure out Ro,Go,Bo that maintains 
    // the neutral chromaticity.    
    Rgb minRgb;

    minRgb.r = 
    minRgb.g = 
    minRgb.b = 0;

    for (uint32_t cv=0; cv<pi.disp->getPostLutLength(pi.dispId); ++cv) {
        double rgb[3];
        double max = static_cast<double>(
                        (1 << pi.disp->getPostLutBitDepth(pi.dispId)) - 1);
        double x   = static_cast<double>(cv) /
                     static_cast<double>(pi.disp->getPostLutLength(pi.dispId)-1);

        if (x > 1) x = 1;
        if (x < 0) x = 0;

        rgb[0] = max * pi.interp->catmullRomInverseInterp(x, rampR); 
        rgb[1] = max * pi.interp->catmullRomInverseInterp(x, rampG);
        rgb[2] = max * pi.interp->catmullRomInverseInterp(x, rampB);

        if (cv == 0) {
            printf("postlut black goes to %f %f %f\n", rgb[0], rgb[1], rgb[2]);
        }

        for (int idx=0; idx<3; ++idx) {
            if (rgb[idx] < 0)   rgb[idx] = 0;
            if (rgb[idx] > max) rgb[idx] = max;
        }

        postLut[0].push_back( static_cast<uint32_t>(rgb[0] + 0.5) );
        postLut[1].push_back( static_cast<uint32_t>(rgb[1] + 0.5) );
        postLut[2].push_back( static_cast<uint32_t>(rgb[2] + 0.5) );
    }

    calib.setPreLutRed(preLut[0]);
    calib.setPreLutGreen(preLut[1]);
    calib.setPreLutBlue(preLut[2]);

    calib.setPostLutRed(postLut[0]);
    calib.setPostLutGreen(postLut[1]);
    calib.setPostLutBlue(postLut[2]);

    return true;
}


// ----------------------------------------
//
bool
Ookala::DreamColorCalib::computeGoalRgb(
                            Color *color,
                            const Yxy &red,       const Yxy  &green,
                            const Yxy &blue,      const Yxy  &white,
                            const Yxy &goalWhite, Rgb        &rgb)
{
    Npm npm;

    if (!color->computeNpm(red, green, blue, white, npm)) {
        return false;
    }

    rgb = color->cvtYxyToRgb(goalWhite, npm);

    return true;
}

// ----------------------------------------
//
bool
Ookala::DreamColorCalib::validate(
                          PluginData                   &pi,
                          DreamColorSpaceInfo           target,
                          PluginChain                  *chain)
{
    Yxy                      actualWhite,
                             actualRed, 
                             actualGreen, 
                             actualBlue;
    Yxy                      minWhite, maxWhite, 
                             minRed,   maxRed,
                             minGreen, maxGreen,
                             minBlue,  maxBlue;
    Npm                      panelNpm;
    std::map<double, double> grayRampR, 
                             grayRampG, 
                             grayRampB;

    setErrorString("");

    // Figure out some limits for the color target we're trying to hit.
    minWhite = target.getWhite();
    minRed   = target.getRed();
    minGreen = target.getGreen();
    minBlue  = target.getBlue();

    maxWhite = target.getWhite();
    maxRed   = target.getRed();
    maxGreen = target.getGreen();
    maxBlue  = target.getBlue();
   
    minWhite.Y -= mWhiteToleranceMinus.Y;
    maxWhite.Y += mWhiteTolerancePlus.Y;

    minWhite.x -= mWhiteToleranceMinus.x;
    maxWhite.x += mWhiteTolerancePlus.x;
    minRed.x   -= mRedToleranceMinus.x;
    maxRed.x   += mRedTolerancePlus.x;
    minGreen.x -= mGreenToleranceMinus.x;
    maxGreen.x += mGreenTolerancePlus.x;
    minBlue.x  -= mBlueToleranceMinus.x; 
    maxBlue.x  += mBlueTolerancePlus.x;

    minWhite.y -= mWhiteToleranceMinus.y;
    maxWhite.y += mWhiteTolerancePlus.y;
    minRed.y   -= mRedToleranceMinus.y;
    maxRed.y   += mRedTolerancePlus.y;
    minGreen.y -= mGreenToleranceMinus.y;
    maxGreen.y += mGreenTolerancePlus.y;
    minBlue.y  -= mBlueToleranceMinus.y; 
    maxBlue.y  += mBlueTolerancePlus.y;


    // Turn on the pattern generator - take care to disable
    //       it when we return in this method then.
    if (!pi.disp->setPatternGeneratorEnabled(true, target.getConnId())) {
        setErrorString("ERROR: Can't enable pattern generator.");
        return false;
    }
    if (!idle(pi.disp->patternGeneratorEnableTime(pi.dispId), chain)) {
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false;
    }
    if (!pi.disp->setPatternGeneratorColor(1023, 1023, 1023, pi.dispId)) {
        setErrorString("ERROR: Can't set pattern generator.");
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false;
    }
    if (!idle(3, chain)) {
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false;
    }   

    if (!pi.sensor->measureYxy(actualWhite, chain, pi.sensorId)) {
        setErrorString( std::string("Can't read sensor [white]: ") + 
                            pi.sensor->errorString());
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false;
    }


    if (!measurePrimaries(pi,        chain,
                          actualRed, actualGreen, actualBlue)) { 
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false;
    }

    printf("Measured wht: %f %f %f\n", actualWhite.x, actualWhite.y, actualWhite.Y);

    printf("Measured red: %f %f\n", actualRed.x,   actualRed.y);
    printf("Measured grn: %f %f\n", actualGreen.x, actualGreen.y);
    printf("Measured blu: %f %f\n", actualBlue.x,  actualBlue.y);
    printf("\n");
    printf("Target wht:   %f %f %f\n", target.getWhite().x, target.getWhite().y,
                                                           target.getWhite().Y);
    printf("Target red:   %f %f\n", target.getRed().x,   target.getRed().y);
    printf("Target grn:   %f %f\n", target.getGreen().x, target.getGreen().y);
    printf("Target blu:   %f %f\n", target.getBlue().x,  target.getBlue().y);

    
    // Test against our target threshold
    if ((actualWhite.Y < minWhite.Y) || (actualWhite.Y > maxWhite.Y)) {
        setErrorString("WhitelLuminance out of range.");
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false; 
    }

    if ((actualWhite.x < minWhite.x) || (actualWhite.x > maxWhite.x) ||
        (actualWhite.y < minWhite.y) || (actualWhite.y > maxWhite.y)) {
        setErrorString("White chromaticity out of range.");
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false; 
    }

    if ((actualRed.x < minRed.x) || (actualRed.x > maxRed.x) ||
        (actualRed.y < minRed.y) || (actualRed.y > maxRed.y)) {
        setErrorString("Red chromaticity out of range.");
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false; 
    }

    if ((actualGreen.x < minGreen.x) || (actualGreen.x > maxGreen.x) ||
        (actualGreen.y < minGreen.y) || (actualGreen.y > maxGreen.y)) {
        setErrorString("Green chromaticity out of range.");
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false; 
    }

    if ((actualBlue.x < minBlue.x) || (actualBlue.x > maxBlue.x) ||
        (actualBlue.y < minBlue.y) || (actualBlue.y > maxBlue.y)) {
        setErrorString("Blue chromaticity out of range.");
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false; 
    }

    if (!pi.color->computeNpm(actualRed,  actualGreen, 
                              actualBlue, actualWhite,
                              panelNpm)) {
        setErrorString("Unable to compute native NPM.");
        pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
        return false;
    }

    printf("Actual white: %f %f %f\n", actualWhite.Y,
                actualWhite.x, actualWhite.y);


    // Drop a calibration record in the approprate spot so people
    // can track this later on
    DictItem                  *item     = NULL;
    DreamColorCalibRecord     *calibRec = NULL;

    //item = mRegistry->createDictItem("calibRecord");
    item = mRegistry->createDictItem("DreamColorCalibRecord");
    if (item) {

        std::vector<uint32_t> redLut, greenLut, blueLut;

        if (!pi.lut->get(redLut, greenLut, blueLut)) {
            setErrorString("Unable to retrieve luts.");
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
            idle(pi.disp->patternGeneratorDisableTime(pi.dispId), NULL);
            return false;
        }

        calibRec = (DreamColorCalibRecord *)(item);
        if (calibRec) {

            DictHash *hash      = NULL;
            Dict     *chainDict = NULL;
            Dict     *dstDict   = NULL;

            std::vector<Plugin *> plugins = mRegistry->queryByName("DictHash");
            if (!plugins.empty()) {
                hash = dynamic_cast<DictHash *>(plugins[0]);
                if (hash) {
                    chainDict = hash->getDict(chain->getDictName());

                    if (chainDict) {
                        StringDictItem *stringItem;

                        stringItem = static_cast<StringDictItem *>(
                            chainDict->get("DreamColorCalib::calibRecordDict"));
                        if (stringItem) {

                            dstDict = hash->newDict(stringItem->get());
                        }
                    }
                }
            }

            // How do we get a meaningful device id?
            calibRec->setDeviceId("none");
            calibRec->setCalibrationTime(time(NULL));
            calibRec->setCalibrationPluginName("DreamColorCalib");

            calibRec->setPreset(target.getPresetId());
            calibRec->setPresetName(target.getName());
            
            calibRec->setTargetWhite(target.getWhite());
            calibRec->setTargetRed(target.getRed());
            calibRec->setTargetGreen(target.getGreen());
            calibRec->setTargetBlue(target.getBlue());

            calibRec->setMeasuredWhite(actualWhite);
            calibRec->setMeasuredRed(actualRed);
            calibRec->setMeasuredGreen(actualGreen);
            calibRec->setMeasuredBlue(actualBlue);
                   
            calibRec->setLut("red",   redLut);
            calibRec->setLut("green", greenLut);
            calibRec->setLut("blue",  blueLut);

            if (pi.disp) {
                uint32_t regValues[4];

                if (pi.disp->getBacklightRegRaw(regValues[0], regValues[1],
                                                regValues[2], regValues[3],
                                                pi.dispId, chain)) {
                    calibRec->setBrightnessReg(regValues[3]);
                }
            }

            if ((dstDict) && 
                        (target.getCalibRecordName() != std::string(""))) {                
                dstDict->set(target.getCalibRecordName(), calibRec);

                dstDict->debug();
            }
        }
    }

    // Turn off the pattern generator
    if (!pi.disp->setPatternGeneratorEnabled(false, pi.dispId)) {

        // If this happens, well, that sucks. I'm not sure we
        // can recover without power-cycling
        setErrorString("ERROR: Can't disable pattern generator.");
        return false;
    }
    if (!idle(pi.disp->patternGeneratorDisableTime(pi.dispId), chain)) {
        return false;
    }


    return true;
}


// ----------------------------------------
//
// protected
bool
Ookala::DreamColorCalib::wasCancelled(
                              PluginData    &pi,
                              PluginChain   *chain,
                              bool           patternGenEnabled)
{
    if (chain->wasCancelled()) {
        setErrorString("Execution cancelled.");
        
        if (patternGenEnabled) {
            pi.disp->setPatternGeneratorEnabled(false, pi.dispId);
        }

        return true;
    }

    return false;
}

// -------------------------------------
//
// Periodically test if we've been cancelled or not. If so,
// return false.
//
// protected
bool
Ookala::DreamColorCalib::idle(double seconds, PluginChain *chain)
{
    uint32_t sleepUs = static_cast<uint32_t>( (seconds - floor(seconds)) * 1000000.0);

#ifdef _WIN32
    Sleep(sleepUs / 1000.0);
#else
    usleep(sleepUs);
#endif

    if (chain) {
        if (chain->wasCancelled()) {
            setErrorString("Execution cancelled.");
            return false;
        }
    } 

    for (uint32_t i=0; i<static_cast<uint32_t>(floor(seconds)); ++i) {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
        if (chain) {
            if (chain->wasCancelled()) {
                setErrorString("Execution cancelled.");
                return false;
            }
        } 
    }

    if (chain) {
        if (chain->wasCancelled()) {
            setErrorString("Execution cancelled.");
            return false;
        }
    }
    return true;
}


