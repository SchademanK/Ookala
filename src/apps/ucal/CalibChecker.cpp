// --------------------------------------------------------------------------
// $Id: CalibChecker.cpp 135 2008-12-19 00:49:58Z omcf $
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
#include <stdlib.h>
#include <math.h>

#include "Dict.h"
#include "DictHash.h"
#include "DataSavior.h"
#include "PluginRegistry.h"
#include "PluginChain.h"


#include "CalibChecker.h"
#include "WxIconCtrl.h"

#include "plugins/WsLut/WsLut.h"

// -------------------------------------

CalibChecker::CalibChecker():
    Ookala::Plugin()
{
    setName("CalibChecker");
}

// -------------------------------------
//
CalibChecker::CalibChecker(const CalibChecker &src):
    Ookala::Plugin(src)
{
}

// -------------------------------------
//
// virtual
CalibChecker::~CalibChecker()
{
    
}

// ----------------------------
//
CalibChecker &
CalibChecker::operator=(const CalibChecker &src)
{
    if (this != &src) {
        Ookala::Plugin::operator=(src);
    }

    return *this;
}

// -------------------------------------
//
// This is looking for items in the chain dict:
//
//    CalibChecker::calibRecordDict -> The dict in which to
//                                     search for data items. [STRING]
//
//    CalibChecker::calibRecordName -> key to an item of type
//                                     CalibRecordDictItem.  [STRING]
//
//    CalibChecker::calibMetaRecordName -> key to an key to an item of type
//                                         CalibRecordDictItem.  [STRING]
//
//
//    CalibChecker::calibHours      -> The number of  hours a calibration
//                                     is valid. [INT]
//
// virtual protected
bool
CalibChecker::_run(Ookala::PluginChain *chain)
{
    Ookala::Dict                *dict       = NULL;
    Ookala::DictHash            *hash       = NULL;
    Ookala::DictItem            *item       = NULL;
    Ookala::IntDictItem         *intItem    = NULL;
    Ookala::StringDictItem      *stringItem = NULL;
    Ookala::CalibRecordDictItem *calibItem  = NULL;
    bool                         debug      = true;

    std::string          calibRecordDict;
    std::string          calibRecordName;
    std::string          calibMetaRecordName;

    bool                 hasMetaName, hasName;

    int32_t              calibHours  = 1000;


    setErrorString("");

    if (mRegistry == NULL) {
        setErrorString("No PluginRegistry for CalibChecker.");
        return false;
    }   

    if (chain == NULL) {
        setErrorString("No chain for CalibChecker.");
        return false;
    }

    // Retrieve the chain dictionary
    std::vector<Ookala::Plugin *> plugins = mRegistry->queryByName("DictHash");
    if (plugins.empty()) {
        if (debug) {
            fprintf(stderr, "No DictHash found for CalibChecker.");
        }
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }
    hash = dynamic_cast<Ookala::DictHash *>(plugins[0]);
    if (!hash) {
        if (debug) {
            fprintf(stderr, "No DictHash found for CalibChecker.");
        }       
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    dict = hash->getDict(chain->getDictName().c_str());
    if (!dict) {
        if (debug) {
            fprintf(stderr, "No chain Dict found for CalibChecker.");
        }
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }


    // Search the chain dict for CalibChecker::calibRecordDict
    item = dict->get("CalibChecker::calibRecordDict");
    if (!item) {
        if (debug) {
            fprintf(stderr, "No 'CalibChecker::calibRecordDict' [STRING] in Dict %s\n",
                    chain->getDictName().c_str());
        } 
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    stringItem = dynamic_cast<Ookala::StringDictItem *>(item);
    if (!stringItem) {
        if (debug) {
            fprintf(stderr, "No 'CalibChecker::calibRecordDict' [STRING] in Dict %s\n",
                    chain->getDictName().c_str());
        } 
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    calibRecordDict = stringItem->get();
    
    
    // Search the chain dict for CalibChecker::calibRecordName
    hasName = false;
    item = dict->get("CalibChecker::calibRecordName");
    if (item) {
        stringItem = dynamic_cast<Ookala::StringDictItem *>(item);
        if (stringItem) {
            hasName         = true;
            calibRecordName = stringItem->get();
        }
    }
    
    // Search the chain dict for CalibCheck::calibMetaRecordName
    hasMetaName = false;
    item = dict->get("CalibChecker::calibMetaRecordName");
    if (item) {
        stringItem = dynamic_cast<Ookala::StringDictItem *>(item);
        if (stringItem) {
            hasMetaName = true;
            calibMetaRecordName = stringItem->get();
        }
    }

    // If we don't have either a name or a meta name, we're toast
    if ((!hasName) && (!hasMetaName)) {
        if (debug) {
            fprintf(stderr, "Don't have a name or a metaName, giving up..\n");
        } 
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    // Search the chain dict for CalibChecker::calibHours
    item = dict->get("CalibChecker::calibHours");
    if (!item) {
        if (debug) {
            fprintf(stderr, "No 'CalibChecker::calibHours' [INT] in Dict %s\n",
                        chain->getDictName().c_str());
        } 
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    intItem = dynamic_cast<Ookala::IntDictItem *>(item);
    if (!intItem) {
        if (debug) {
            fprintf(stderr, "No 'CalibChecker::calibHours' [INT] in Dict %s\n",
                        chain->getDictName().c_str());
        } 
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    calibHours = intItem->get();




    // Search the specified dict for the calibration record
    dict = hash->getDict(calibRecordDict.c_str());  
    if (!dict) {
        if (debug) {
            fprintf(stderr, "CalibChecker was told to look in dict \"%s\"\n",
                            calibRecordDict.c_str());
            fprintf(stderr, "However, it doesn't seem to exist!\n");
        }
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    // If we have both, metaName wins and wipes out calibRecordName.
    if (hasMetaName) {

        item = dict->get(calibMetaRecordName.c_str());
        if (!item) {
            if (debug) {
                fprintf(stderr, "CalibChecker was told to look in dict \"%s\" for meta record \"%s\"\n",
                                calibRecordDict.c_str(), calibMetaRecordName.c_str());
                fprintf(stderr, "However, it doesn't seem to exist!\n");
                dict->debug();
            }
            setIcon(chain, calibRecordName + std::string(" notCalib"));
            return true;
        }

        stringItem = dynamic_cast<Ookala::StringDictItem *>(item);
        if (!stringItem) {
             if (debug) {
                fprintf(stderr, "CalibChecker was told to look in dict \"%s\" for record \"%s\"\n",
                                calibRecordDict.c_str(), calibMetaRecordName.c_str());
                fprintf(stderr, "However, it doesn't seem to exist!\n");
            }
            setIcon(chain, calibRecordName + std::string(" notCalib"));
            return true;
        }

        calibRecordName = stringItem->get();
    }

    item = dict->get(calibRecordName.c_str());
    if (!item) {
        if (debug) {
            fprintf(stderr, "CalibChecker was told to look in dict %s for record %s\n",
                            calibRecordDict.c_str(), calibRecordName.c_str());
            fprintf(stderr, "However, it doesn't seem to exist!\n");
        }
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    calibItem = (Ookala::CalibRecordDictItem *)(item);
    if (!calibItem) {
        if (debug) {
            fprintf(stderr, "CalibChecker was told to look in dict %s for record %s\n",
                            calibRecordDict.c_str(), calibRecordName.c_str());
            fprintf(stderr, "However, it doesn't seem to exist!\n");
        }
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    }

    // Finally! We have found our calibration record. Check the time
    // since it's been calibrated.
    int32_t elapsedSec = time(NULL) - calibItem->getCalibrationTime();
    if (elapsedSec > calibHours * 3600) {
        setIcon(chain, calibRecordName + std::string(" notCalib"));
        return true;
    } 

    // Next, check the WsLut and make sure that it
    // matches with our calibration state

    if (!checkLuts(calibItem)) {
        setIcon(chain, calibRecordName + std::string(" badLut"));
        return true;
    }

    // Now, check the display state to see if it matches to the calibrated state
    if (!checkDisplayState(calibItem)) {
        setIcon(chain, calibRecordName + std::string(" badDisplayState"));
        return true;
    }

    setIcon(chain, calibRecordName + std::string(" calib"));

    return true;
}

// -------------------------------------
//
// protected
bool
CalibChecker::checkLuts(Ookala::CalibRecordDictItem *calibItem)
{
    bool debug = true;
    std::vector<uint32_t> rCurrLut, gCurrLut, bCurrLut;


    std::vector<Ookala::Plugin *> plugins = mRegistry->queryByName("WsLut");
    if (plugins.empty()) {
        if (debug) {
            fprintf(stderr, "No WsLut found for CalibChecker.\n");
        }
        return false;
    }


    Ookala::WsLut *lut = (Ookala::WsLut *)(plugins[0]);
    if (!lut->get(rCurrLut, gCurrLut, bCurrLut)) {
        if (debug) {
            fprintf(stderr, "Unable to get LUT for CalibChecker.\n");
        }
        return false;
    }

    // XXX: We should probably enforce some sort of naming convention
    //      for storing luts. But, for now, they're just known
    //      as "red", "green", and "blue".
    bool     lutsOk = true;
    bool     lutsChecked[3];
    uint32_t idx;
    std::vector<uint32_t> calibLut;

    lutsChecked[0] = lutsChecked[1] = lutsChecked[2] = false;

    // Check red.
    calibLut = calibItem->getLut("red");
    lutsChecked[0] = true;
    if (calibLut.size() != rCurrLut.size()) {
        lutsOk = false;
    }
    for (idx=0; idx<calibLut.size(); ++idx) {
        if (calibLut[idx] != rCurrLut[idx]) {
            lutsOk = false;
            break;
        }
    }


    // Check green.
    calibLut = calibItem->getLut("green");
    lutsChecked[1] = true;
    if (calibLut.size() != gCurrLut.size()) {
        lutsOk = false;
    }
    for (idx=0; idx<calibLut.size(); ++idx) {
        if (calibLut[idx] != gCurrLut[idx]) {
            lutsOk = false;
            break;
        }
    }


    // Check blue.
    calibLut = calibItem->getLut("blue");
    lutsChecked[2] = true;
    if (calibLut.size() != bCurrLut.size()) {
        lutsOk = false;
    }
    for (idx=0; idx<calibLut.size(); ++idx) {
        if (calibLut[idx] != bCurrLut[idx]) {
            lutsOk = false;
            break;
        }
    }


    // Make sure that we've hit all the Luts that we're supposed to,
    // and that they're all good.
    if ((!lutsChecked[0]) || (!lutsChecked[1]) || (!lutsChecked[2])) {
        if (debug) {
            fprintf(stderr, "Not all Luts verified for CalibChecker.\n");
        }
        return false;
    }

    if (!lutsOk) {
        if (debug) {
            fprintf(stderr, "Luts not in calibrated state for CalibChecker.\n");
        }
        return false;
    }

    return true;
}

// -------------------------------------
//
// protected
bool
CalibChecker::checkDisplayState(Ookala::CalibRecordDictItem *calibItem)
{
    if (!calibItem) return false;

    std::vector<Ookala::Plugin *> plugins = mRegistry->queryByName(
                            calibItem->getCalibrationPluginName());
    if (plugins.empty()) {
        return false;
    }

    return (plugins[0])->checkCalibRecord(calibItem);
}

// -------------------------------------
//
// protected
bool
CalibChecker::setIcon(Ookala::PluginChain *chain, const std::string &iconName)
{
    std::vector<Ookala::Plugin *> plugins = mRegistry->queryByName("WxIconCtrl");
    if (plugins.empty()) {
        return false;
    }

    WxIconCtrl *ctrl = dynamic_cast<WxIconCtrl *>(plugins[0]);
    if (ctrl == NULL) {
        return false;
    }

    printf("setting icon to %s\n", iconName.c_str());
    return ctrl->setIcon(iconName);
}



