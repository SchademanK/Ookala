// --------------------------------------------------------------------------
// $Id: DreamColorCtrl.cpp 135 2008-12-19 00:49:58Z omcf $
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

#ifndef _WIN32
// for htons()
#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <algorithm>

#include "Dict.h"
#include "DreamColorCalib.h"
#include "PluginRegistry.h"
#include "PluginChain.h"
#include "DreamColorCtrl.h"


// ----------------------------------

BEGIN_PLUGIN_REGISTER(2)
    PLUGIN_REGISTER(0, Ookala::DreamColorCtrl)
    PLUGIN_REGISTER(1, Ookala::DreamColorCalib)
END_PLUGIN_REGISTER

// ----------------------------------

namespace Ookala {
    enum MemoryBlocks {
        MemoryBlockPostLut           = 1,
        MemoryBlockMatrix            = 2,
        MemoryBlockPreLut            = 3,
        MemoryBlockBacklightRegister = 0x82,
    };
};

// ====================================
//
// Private Structs
//
// -------------------------------------

//
// Description of programmable memory blocks
//

namespace Ookala {
    struct PMBInfo
    {   
        uint32_t    pmbIndex;    // The index of the PMB.
        uint32_t    numChannels; // Should be 3.
        uint32_t    tableLength; // 1024 for a LUT
        uint32_t    bitDepth;    // e.g. 12 bits/entry
        std::string name;

        // Memory blocks include the pre-LUT, post-LUT, and matrix.
        // Register blocks control the values of Torino, Safe-Lite,
        // and the backlight.
        bool        isMemoryBlock;
        bool        isRegisterBlock;
    };
};

//
// Data associated with a connection to a display
//

namespace Ookala {
    struct DisplayConn
    {
        std::string    pluginName;     // The name of the plugin 
                                       // used for comm.
        uint32_t       pluginId;       // The key to the device 
                                       // used by the connection plugin.        
        struct Edid1_3 edid;           // The detected EDID.

        uint32_t       numColorspaces; // The number of color-space presets
                                       // supported on the device.
                                       // HP LP2480zx supports 7.

        uint32_t       patternGenBitDepth;

        uint32_t       firmwareVersion; 

        // Hold a map from memory block id (0x1, 0x2, 0x80, ...) 
        //  to PMB info struct
        std::map<uint32_t, struct PMBInfo> pmbInfoHash;
    };
};


// 
// Private chunk of data, to avoid exporting STL under windows
//

namespace Ookala {
    struct _DreamColorCtrl {
        // Running count of the next key to map to a device. The ID
        // of '0' is reserved for a default device (the first one that
        // we happen to find - in other words, the user doesn't care).
        uint32_t                        mNextKey;

        // Map of device id keys to their connections.
        std::map<uint32_t, DisplayConn> mConnections;
    };
};



// ====================================
//
// DreamColorCtrl
//
// -------------------------------------

Ookala::DreamColorCtrl::DreamColorCtrl():
    Plugin()
{
    setName("DreamColorCtrl");

    mDreamColorCtrlData           = new _DreamColorCtrl;
    mDreamColorCtrlData->mNextKey = 1;
}

// -------------------------------------
//

Ookala::DreamColorCtrl::DreamColorCtrl(const DreamColorCtrl &src):
    Plugin(src)
{

    mDreamColorCtrlData           = new _DreamColorCtrl;

    if (src.mDreamColorCtrlData) {
        mDreamColorCtrlData->mNextKey     = src.mDreamColorCtrlData->mNextKey;
        mDreamColorCtrlData->mConnections = src.mDreamColorCtrlData->mConnections;
    }
}

// -------------------------------------
//
// virtual
Ookala::DreamColorCtrl::~DreamColorCtrl()
{
}

// ----------------------------
//
Ookala::DreamColorCtrl &
Ookala::DreamColorCtrl::operator=(const DreamColorCtrl &src)
{
    if (this != &src) {
        Plugin::operator=(src);

        if (mDreamColorCtrlData) {
            delete mDreamColorCtrlData;
            mDreamColorCtrlData = NULL;
        }

        if (src.mDreamColorCtrlData) {
            mDreamColorCtrlData  = new _DreamColorCtrl();
            *mDreamColorCtrlData = *(src.mDreamColorCtrlData);
        }
    }

    return *this;
}

// -------------------------------------
//
// virtual
std::vector<uint32_t>
Ookala::DreamColorCtrl::devices()
{
    std::vector<uint32_t> devs;

    if (!mDreamColorCtrlData) {
        return devs;
    }

    for (std::map<uint32_t, DisplayConn>::iterator conn = 
                    mDreamColorCtrlData->mConnections.begin();
            conn != mDreamColorCtrlData->mConnections.end(); ++conn) {
        devs.push_back((*conn).first);
    }

    return devs;
}

// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::deviceValid(uint32_t connId)
{
    DisplayConn conn;

    return getConnFromKey(connId, &conn, NULL);
}

// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::enumerate(PluginChain *chain /* = NULL */)
{
    bool      debug = true;
    uint32_t  bufSize;
    uint8_t   msg[1024];

    setErrorString("");

    if (!mDreamColorCtrlData) return false;

    std::vector<DisplayConn> foundConnections;

    if (!mRegistry) {
        setErrorString("Plugin registry not found.");
        return false;
    }    

    // Grab all DDC communications channels
    std::vector<Plugin *> ddcPlugins = mRegistry->queryByAttribute("ddc/ci");

    // Search them all for all their EDIDs.
    // If we find an EDID we recognize, add the
    // edid to a temp connection list (foundConnections)
    for (std::vector<Plugin *>::iterator pi = ddcPlugins.begin();
                pi != ddcPlugins.end(); ++pi) {

        Ddc *ddcpi = dynamic_cast<Ddc *>(*pi);
        if (ddcpi == NULL) continue;

        if (!ddcpi->enumerate()) continue;

        std::vector<uint32_t> devs = ddcpi->devices();        
        for (std::vector<uint32_t>::iterator dev = devs.begin();
                dev != devs.end(); ++dev) {           
            Edid1_3 edid;

            if (!ddcpi->getEdid13(edid, *dev)) {
                continue;
            }
    
            if ((edid.manufacturer == 0xf022) &&  // HP 
                (edid.productCode  ==  0x26f3)) { // HP2480zx
                DisplayConn conn;
                uint16_t    currVal, maxVal;

                // Try to read torino state. If the display is off, this should
                // fail (even though edid reading would succeed). If the display
                // is off, we probably shouldn't enumerate it.
                if (!ddcpi->getVcpFeature(0xe2, currVal, maxVal, *dev)) {
                    continue;
                }
                conn.pluginName = ddcpi->name();
                conn.pluginId   = *dev;
                conn.edid       = edid;

                foundConnections.push_back(conn);
            }
        }
    }

    // Search all our currect connections for 
    // devices that are not in the found connections
    // list. If found, remove them from the mConnections.
    std::vector<uint32_t> badKeys;
    for (std::map<uint32_t, DisplayConn>::iterator conn = 
                        mDreamColorCtrlData->mConnections.begin();
                conn != mDreamColorCtrlData->mConnections.end(); ++conn) {

        bool found = false;

        for (std::vector<DisplayConn>::iterator goodConn = 
                                foundConnections.begin();
                   goodConn != foundConnections.end(); ++goodConn) {

            // We might want to expand this to check some EDID data
            // as well, assuming it's not lying..
            if (((*goodConn).pluginName == (*conn).second.pluginName) &&
                ((*goodConn).pluginId   == (*conn).second.pluginId)   &&
                (memcmp( (*goodConn).edid.rawEdidBlock, 
                         (*conn).second.edid.rawEdidBlock, 128) == 0)) {
                found = true;
            }
        }

        if (!found) {
            badKeys.push_back((*conn).first);
        }
    }
    for (std::vector<uint32_t>::iterator badKey = badKeys.begin();
                        badKey != badKeys.end(); ++badKey) {
        mDreamColorCtrlData->mConnections.erase(*badKey);
    }

    // Search mConnections for devices we now have
    // in the temp connection list, but are not
    // in mConnection (new devices). Add these to 
    // our connection list with new keys. Keep track
    // of the keys that we insert.
    std::vector<uint32_t> newKeys;
    for (std::vector<DisplayConn>::iterator newConn = 
                                foundConnections.begin();
                newConn != foundConnections.end(); ++newConn) {
        bool found = false;

        for (std::map<uint32_t, DisplayConn>::iterator conn = 
                        mDreamColorCtrlData->mConnections.begin();
                conn != mDreamColorCtrlData->mConnections.end(); ++conn) {

            // We might want to expand this to check some EDID data
            // as well, assuming it's not lying..
            if (((*newConn).pluginName == (*conn).second.pluginName) &&
                ((*newConn).pluginId   == (*conn).second.pluginId)   &&
                (memcmp( (*newConn).edid.rawEdidBlock, 
                         (*conn).second.edid.rawEdidBlock, 128) == 0)) {
                found = true;
            }
        }
            
        if (!found) {
            uint32_t key = mDreamColorCtrlData->mNextKey;
            mDreamColorCtrlData->mNextKey++;

            mDreamColorCtrlData->mConnections[key] = (*newConn);
            newKeys.push_back(key);
        }
    }

    // Run over all our new connections and fill out the
    // DreamColor specific data (PMBs, bit depth, etc).

    //for (std::map<uint32_t, DisplayConn>::iterator conn = 
    //                                    mConnections.begin();
    //        conn != mConnections.end(); ++conn) {
    for (std::vector<uint32_t>::iterator theNewKey = newKeys.begin();
                theNewKey != newKeys.end(); ++theNewKey) {

        uint32_t  devId;
        Ddc      *ddc;

        std::map<uint32_t, DisplayConn>::iterator conn;

        conn = mDreamColorCtrlData->mConnections.find(*theNewKey);
        if (conn == mDreamColorCtrlData->mConnections.end()) {
            continue;
        }

        (*conn).second.numColorspaces     = 7;
        (*conn).second.patternGenBitDepth = 10;

        // load the PMBs and get the firmware version
        ddc = getPluginFromKey((*conn).first, devId, chain);
        if (!ddc) {
            continue;
        }


        if (!getFirmwareVersion((*conn).second.firmwareVersion, (*conn).first)) {
            if (debug) {
                fprintf(stderr, "Problem getting firmware:\n");
            }
            continue;
        }

        uint32_t pmbIdx[] = {0x01, 0x02, 0x03, 0x80, 0x81, 0x82};
        PMBInfo  pmbInfo;
        for (uint32_t i=0; i<6; ++i) {

            pmbInfo.pmbIndex = pmbIdx[i];

            if ((pmbIdx[i]) < 0x80) {
                pmbInfo.isMemoryBlock   = true;
                pmbInfo.isRegisterBlock = false;
            } else {
                pmbInfo.isMemoryBlock   = false;
                pmbInfo.isRegisterBlock = true;
            }


            // Try and read data about the PMB.
            // Send:
            //       0xe2  [table read]
            //       0xe0  [PMB size + format]
            //        XX
            //        YY 
            //
            // where X and Y are in:
            //     00 0x03 -> pre lut
            //     00 0x02 -> 3x3 matrix
            //     00 0x01 -> post lut
            //     00 0x80 -> Torino Register
            //     00 0x81 -> Safe-Lite Register
            //     00 0x82 -> Backlight Register
            //
            // NOTE: The pre-lut and post-lut have reversed ID's from 
            //       what is currently specified. The flipped version
            //       is correct (pre-lut = 3).
            //
            // The response appears to be up to 36 bytes of data, plus 
            // 3 bytes of header;
            //
            //     0xe4
            //     0x00  -> Register index, from above??
            //     0x01  -> ""      ""           ""
            //     0xXX  -> Number of channels
            //     0xXX  -> Number of entries, high byte
            //     0xXX  -> Number of entries, low byte
            //     0xXX  -> Bits per entry
            //     ...   -> 0x0 terminated ascii string
            //       

            msg[0] = 0xe2;
            msg[1] = 0xe0;
            msg[2] = 0;
            msg[3] = pmbIdx[i];
            if (ddc->sendI2cPayload(msg, 4, devId) == false) {
                if (debug) {
                    fprintf(stderr, "Problem send to get PMB:\n");
                }

                setErrorString(std::string("Send of Table Read cmd failed:\n") + 
                                 ddc->errorString());
                return false;
            }

            bufSize = 36;
            if (ddc->recvI2cPayload(msg, bufSize, false, devId) == false) {
                if (debug) {
                    fprintf(stderr, "Problem recv to get PMB:\n");
                    fprintf(stderr, "\t%s\n", ddc->errorString().c_str());
                    continue;
                }
                setErrorString( std::string("Recv of Table Read cmd failed:\n") + 
                                    ddc->errorString());
                return false;
            }

            pmbInfo.numChannels =  msg[3];
            pmbInfo.tableLength = (msg[4]<<8) | msg[5];
            pmbInfo.bitDepth    = msg[6];
            pmbInfo.name = "";

            for (uint32_t j=7; j<bufSize; ++j) {
                pmbInfo.name.append(1, msg[j]);
            }
            
            if (debug) {
                fprintf(stderr, "\nProgrammable Block %d:\n", i);
                fprintf(stderr, "\tIndex:        0x%x\n", pmbInfo.pmbIndex);
                fprintf(stderr, "\tname:         %s\n",   pmbInfo.name.c_str());
                fprintf(stderr, "\t# Channels:   %d\n",   pmbInfo.numChannels);
                fprintf(stderr, "\tLength:       %d\n",   pmbInfo.tableLength);
                fprintf(stderr, "\tBit Depth:    %d\n",   pmbInfo.bitDepth);
                fprintf(stderr, "\tMemory Blk:   %d\n",   pmbInfo.isMemoryBlock);
                fprintf(stderr, "\tRegister Blk: %d\n",   pmbInfo.isRegisterBlock);
            }

            (*conn).second.pmbInfoHash[pmbInfo.pmbIndex] = pmbInfo;

        }
    }

    return true;
}

// -------------------------------------
//
// virtual 
uint32_t
Ookala::DreamColorCtrl::patternGeneratorEnableTime(
                                uint32_t connId /* = 0 */,
                                PluginChain *chain /* = NULL */)
{
    return 5;
}


// -------------------------------------
//
// virtual 
uint32_t
Ookala::DreamColorCtrl::patternGeneratorDisableTime(
                                uint32_t connId /* = 0 */,
                                PluginChain *chain /* = NULL */)
{
    return 5;
}

// -------------------------------------
//
// virtual 
uint32_t
Ookala::DreamColorCtrl::patternGeneratorSettleTime(
                                uint32_t connId /* = 0 */,
                                PluginChain *chain /* = NULL */)
{
    return 2;
}

// -------------------------------------
//
// virtual 
uint32_t
Ookala::DreamColorCtrl::backlightBrightnessSettleTime(
                                uint32_t connId /* = 0 */,
                                PluginChain *chain /* = NULL */)
{
    return 20;
}

// -------------------------------------
//
// virtual 
uint32_t
Ookala::DreamColorCtrl::backlightGenericSettleTime(
                                uint32_t connId /* = 0 */,
                                PluginChain *chain /* = NULL */)
{
    return 5;
}


// -------------------------------------
//
// Test if the OSD is locked
//
// virtual
bool
Ookala::DreamColorCtrl::getOsdLocked(bool &locked, uint32_t connId /* = 0 */,
                                           PluginChain *chain /* = NULL */)
{
    uint16_t curr, max;

    setErrorString("");

    if (!retryGetVcpFeature(0xCA, curr, max, connId, chain)) {
        setErrorString( errorString() +
                         std::string( " Error in retryGetVcpFeature()."));
        return false;
    }

    if (curr == 1) {
        locked = true;
    } else {
        locked = false;
    }

    return true;
}

// -------------------------------------
//
// Lock or unlock the OSD
//
// virtual
bool
Ookala::DreamColorCtrl::setOsdLocked(bool locked, uint32_t connId /* = 0 */,
                                          PluginChain *chain /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;
    uint16_t curr = 2;

    setErrorString("");

    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (locked) {
        curr = 1;
    } else {
        curr = 2;
    }

    if (!ddc->setVcpFeature(0xCA, curr, ddcId)) {
        setErrorString("Error in setVcpFeature()\n");
        return false;
    }

    return true;
}

// -------------------------------------
//
// Returns the number of hours on the backlight
//
// virtual
bool
Ookala::DreamColorCtrl::getBacklightHours(
                            uint32_t    &hours, uint32_t connId /* = 0 */,
                            PluginChain *chain /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;
    uint16_t curr, max;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!retryGetVcpFeature(0xC0, curr, max, connId, chain)) {
        setErrorString( errorString() + 
                        std::string("Error in retryGetVcpFeature()\n"));
        return false;
    }

    hours = curr;

    return true;
}

// -------------------------------------
//
// Returns the backlight temprature in C.
//
// virtual
bool
Ookala::DreamColorCtrl::getBacklightTemp(uint32_t    &temp,
                                         uint32_t     connId /* = 0 */,
                                         PluginChain *chain /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;
    uint16_t curr, max;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!retryGetVcpFeature(0xE4, curr, max, connId, chain)) {
        setErrorString(errorString() + 
                        std::string("Error in retryGetVcpFeature()."));
        return false;
    }

    temp = curr;

    return true;
}

// -------------------------------------
//
// virtual 
bool
Ookala::DreamColorCtrl::getFirmwareVersion(uint32_t    &version, 
                                           uint32_t     connId /* = 0 */,
                                           PluginChain *chain /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;
    uint16_t curr, max;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!retryGetVcpFeature(0xC9, curr, max, connId, chain)) {
        setErrorString( errorString() + 
                            std::string("Error in retryGetVcpFeature()."));
        return false;
    }

    version = static_cast<uint32_t>(curr);

    return true;
}

// -------------------------------------
//
// virtual
uint32_t
Ookala::DreamColorCtrl::numColorSpace(uint32_t     connId /* = 0 */,
                                      PluginChain *chain /* = NULL */)
{
    DisplayConn conn;

    if (!getConnFromKey(connId, &conn, chain)) return 0;

    return conn.numColorspaces;
}

// -------------------------------------
//
//virtual
bool
Ookala::DreamColorCtrl::getColorSpace(uint32_t &csIdx, uint32_t connId /* = 0 */,
                                      PluginChain *chain /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;
    uint16_t curr, max;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!retryGetVcpFeature(0xE1, curr, max, connId, chain)) {
        setErrorString( errorString() +
                            std::string("Error in retryGetVcpFeature()."));
        return false;
    }

    // The current color space is encoded in the lower byte
    // of the 'curr' value
    csIdx = curr & 0xff;

    return true;
}

// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::setColorSpace(uint32_t     csIdx, 
                                      uint32_t     connId /* = 0 */,
                                      PluginChain *chain /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, NULL);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (csIdx >= numColorSpace(connId, NULL)) {
        setErrorString("Color space index out of range.");
        return false;
    }

    // To select a color gamut, we set 0xe1 using the usual VCP 
    // set mechanism. The high byte shuold be 0x1, and the low
    // byte the desired gamut. 
    if (!ddc->setVcpFeature(0xe1, (1<<8) | (csIdx & 0xff), ddcId)) {
                                    
        setErrorString( std::string("VCP Feature Set of 0xE1 failed. ") +
                                 ddc->errorString());
        return false;
    }


    return true;
}

// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::setColorProcessingEnabled(
                                      bool         enable, 
                                      uint32_t     connId /* = 0 */,
                                      PluginChain *chain /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    // Enable and disable are not symmetric. There is an explicit
    // disable command, but to enable, we have to re-selecte
    // the current color space.
    if (enable) {
        uint32_t csIdx;

        if (!getColorSpace(csIdx, connId, chain)) {
            return false;
        }

        if (!setColorSpace(csIdx, connId, chain)) {
            return false;
        }

        // This needs to sleep, even if we've been cancelled
        // already - so don't pass in the chain, just NULL.
        // Otherwise, we might not sleep if we're trying
        // to put the display back in the right state after
        // a cancel.
        if (!idle(3, NULL)) {
            return false;
        }

        return true;
    } else {
        return ddc->setVcpFeature(0xE1, (0x80 << 8), ddcId);
    }

    return false;

}

// -------------------------------------
//
// virtual
uint32_t
Ookala::DreamColorCtrl::patternGeneratorBitDepth(
                                         uint32_t     connId /* = 0 */,
                                         PluginChain *chain /* = NULL */)
{
    DisplayConn conn;

    if (!getConnFromKey(connId, &conn, chain)) return 0;

    return conn.patternGenBitDepth;
}

// -------------------------------------
//
// Enable or disable the pattern generator
// virtual
bool 
Ookala::DreamColorCtrl::setPatternGeneratorEnabled(
                                           bool         enable, 
                                           uint32_t     connId /* = 0 */,
                                           PluginChain *chain /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (enable) {
        return ddc->setVcpFeature(0xE1, (0xF1 << 8) | 0x01, ddcId);
    } else {
        return ddc->setVcpFeature(0xE1, (0xF1 << 8) | 0x00, ddcId);
    }

    return false;
}

// -------------------------------------
//
// Set the color displayed by the pattern generator
//
// virtual

bool
Ookala::DreamColorCtrl::setPatternGeneratorColor(
                                         uint32_t     red,  
                                         uint32_t     green,
                                         uint32_t     blue, 
                                         uint32_t     connId /* = 0 */,
                                         PluginChain *chain  /* = NULL */)
{
    Ddc     *ddc;
    uint32_t ddcId;
    uint8_t  msg[1024];

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    uint32_t mask = (1 << patternGeneratorBitDepth(connId, chain))-1;

    red   = red   & mask;
    green = green & mask;
    blue  = blue  & mask;

    msg[0] = 0xe7;
    msg[1] = 0xe1;
    msg[2] = 0;
    msg[3] = 0;

    msg[4] = red & 0xff;          // Red low byte
    msg[5] = (red >> 8) & 0xff; 

    msg[6] = green & 0xff;        // Green low byte
    msg[7] = (green >> 8) & 0xff; 

    msg[8] = blue & 0xff;         // Blue low byte
    msg[9] = (blue >> 8) & 0xff; 

    if (ddc->sendI2cPayload(msg, 10, ddcId) == false) {
        setErrorString( std::string("Send of table write cmd failed: ") +    
                    ddc->errorString());
        return false;
    }

    return true;
}

// -------------------------------------
//
// Should this have a default value that reads the
// currently set color space?
//
// virtual
bool
Ookala::DreamColorCtrl::getColorSpaceInfo(  
                                  DreamColorSpaceInfo &info, 
                                  uint32_t             connId /* = 0 */, 
                                  PluginChain         *chain /* = NULL */)
{
    return downloadColorSpaceInfo(info, 0xff, connId, chain);
}


// -------------------------------------
//
// Should this have a default value that reads the
// currently set color space?
//
// virtual
bool
Ookala::DreamColorCtrl::getColorSpaceInfoByIdx(
                                       DreamColorSpaceInfo &info, 
                                       uint32_t             csIdx,
                                       uint32_t             connId /* = 0 */, 
                                       PluginChain         *chain /* = NULL */)
{
    return downloadColorSpaceInfo(info, csIdx, connId, chain);
}

// -------------------------------------
//
// Return the number of bits in the pre-lut       
//
// virtual 
uint32_t
Ookala::DreamColorCtrl::getPreLutBitDepth(
                                  uint32_t      connId /* = 0 */,
                                  PluginChain  *chain /* = NULL */)
{
    DisplayConn conn;
    std::map<uint32_t, struct PMBInfo>::iterator theBlock;

    if (!getConnFromKey(connId, &conn, chain)) return 0;

    theBlock = conn.pmbInfoHash.find(MemoryBlockPreLut);
    if (theBlock == conn.pmbInfoHash.end()) {
        return 0;
    } 

    return (*theBlock).second.bitDepth;
}

// -------------------------------------
//
// Return the number of bits in the post-lut       
//
// virtual 
uint32_t
Ookala::DreamColorCtrl::getPostLutBitDepth(
                                   uint32_t      connId /* = 0 */,
                                   PluginChain  *chain /* = NULL */)
{
    DisplayConn conn;
    std::map<uint32_t, struct PMBInfo>::iterator theBlock;

    if (!getConnFromKey(connId, &conn, chain)) return 0;

    theBlock = conn.pmbInfoHash.find(MemoryBlockPostLut);
    if (theBlock == conn.pmbInfoHash.end()) {
        return 0;
    } 

    return (*theBlock).second.bitDepth;
}

// -------------------------------------
//
// Return the number of entries in the pre-lut
//
// virtual 
uint32_t 
Ookala::DreamColorCtrl::getPreLutLength(
                                uint32_t      connId /* = 0 */,
                                PluginChain  *chain /* = NULL */)
{
    DisplayConn conn;
    std::map<uint32_t, struct PMBInfo>::iterator theBlock;

    if (!getConnFromKey(connId, &conn, chain)) return 0;

    theBlock = conn.pmbInfoHash.find(MemoryBlockPreLut);
    if (theBlock == conn.pmbInfoHash.end()) {
        return 0;
    } 

    return (*theBlock).second.tableLength;
}

// -------------------------------------
//
// Return the number of entries in the post-lut
//
// virtual 
uint32_t 
Ookala::DreamColorCtrl::getPostLutLength(
                                 uint32_t      connId /* = 0 */,
                                 PluginChain  *chain /* = NULL */)
{
    DisplayConn conn;
    std::map<uint32_t, struct PMBInfo>::iterator theBlock;

    if (!getConnFromKey(connId, &conn, chain)) return 0;

    theBlock = conn.pmbInfoHash.find(MemoryBlockPostLut);
    if (theBlock == conn.pmbInfoHash.end()) {
        return 0;
    } 

    return (*theBlock).second.tableLength;
}

// -------------------------------------
//
// Retrieve the pre-lut for the current color space
//
// virtual
bool 
Ookala::DreamColorCtrl::getPreLut(
                          std::vector<uint32_t> &redLut,
                          std::vector<uint32_t> &greenLut,
                          std::vector<uint32_t> &blueLut,
                          uint32_t               connId /* = 0 */,
                          PluginChain           *chain /* = NULL */)
{
    Ddc        *ddc;
    uint32_t    ddcId;
    DisplayConn conn;


    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    redLut.clear();
    greenLut.clear();
    blueLut.clear();

    if (!getConnFromKey(connId, &conn, chain)) {
        return false;
    }

    // Make sure we really have a Pre-LUT block
    std::map<uint32_t, struct PMBInfo>::iterator theBlock;
    theBlock = conn.pmbInfoHash.find(MemoryBlockPreLut);
    if (theBlock == conn.pmbInfoHash.end()) {
        return false;
    } 

    return downloadLut16(MemoryBlockPreLut, 
                            getPreLutLength(connId, chain),
                            redLut, greenLut, blueLut, connId, chain);
}   

// -------------------------------------
//
// Retrieve the post-lut for the current color space
//
// virtual
bool 
Ookala::DreamColorCtrl::getPostLut(
                           std::vector<uint32_t> &redLut,
                           std::vector<uint32_t> &greenLut,
                           std::vector<uint32_t> &blueLut,
                           uint32_t               connId /* = 0 */,
                           PluginChain           *chain /* = NULL */)
{
    Ddc        *ddc;
    uint32_t    ddcId;
    DisplayConn conn;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    redLut.clear();
    greenLut.clear();
    blueLut.clear();

    if (!getConnFromKey(connId, &conn, chain)) {
        return false;
    }

    // Make sure we really have a Post-LUT block
    std::map<uint32_t, struct PMBInfo>::iterator theBlock;
    theBlock = conn.pmbInfoHash.find(MemoryBlockPostLut);
    if (theBlock == conn.pmbInfoHash.end()) {
        return false;
    } 

    return downloadLut16(MemoryBlockPostLut, 
                            getPostLutLength(connId, chain),
                            redLut, greenLut, blueLut, connId, chain);
}

// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::getMatrix(
                          Mat33       &matrix,
                          uint32_t     connId /* = 0 */,
                          PluginChain *chain  /* = NULL */)
{
    Ddc        *ddc;
    uint32_t    ddcId;
    DisplayConn conn;
    uint32_t    bufSize;
    uint8_t     msg[1024], *data;

    uint16_t    us;
    int16_t     s;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!getConnFromKey(connId, &conn, chain)) {
        return false;
    }

    if (!torinoIdle(connId, chain)) {
        return false;
    }

    // Make sure we really have a matrix memory block
    std::map<uint32_t, struct PMBInfo>::iterator theBlock;
    theBlock = conn.pmbInfoHash.find(MemoryBlockMatrix);
    if (theBlock == conn.pmbInfoHash.end()) {
        setErrorString("No Matrix memory block.");
        return false;
    } 
    uint32_t matrixDepth = (*theBlock).second.bitDepth;

    //    Send:   0xE2 0xE2     [BLK_ID] [OFFSET]
    //    Recv:   0xE4 [BLK_ID] [OFFSET] 
    //
    // Which should give us up to 32 bytes of goodness. But,
    // matrix values are stored as unsigned ints, normalized
    // to 1.0.

    msg[0] = 0xE2;
    msg[1] = 0xE2;
    msg[2] = MemoryBlockMatrix;  // 3x3 matrix is block 2
    msg[3] = 0;                  // Offset at 0.
    if (ddc->sendI2cPayload(msg, 4, ddcId) == false) {
        setErrorString( std::string("Send of Table Read cmd failed: ") +
                                ddc->errorString());
        return false;
    }

    // Response is 21 bytes for fixed point values,
    // 2 bytes * 9 matrix values, plus 3 bytes for
    // the BLK_ID + OFFSET in the response header
    bufSize = 21;
    if (ddc->recvI2cPayload(msg, bufSize, false, ddcId) == false) {                                    
        setErrorString( std::string("Recv of Table Read cmd failed: ") + 
                            ddc->errorString());
        return false;
    }


    // Sometimes, if there isn't a matrix associated with a
    // space, we get a response of 4-bytes long.
    if (bufSize == 4) {
        matrix.m00 = 
        matrix.m11 = 
        matrix.m22 = 1.0;

        matrix.m01 = 
        matrix.m02 = 
        matrix.m10 =
        matrix.m12 =
        matrix.m20 = 
        matrix.m21 = 0;

        return true;
    }

    if (bufSize != 21) {
        setErrorString("Read too little data for color space.");
        return false;
    }

    data = &msg[3];

    for (int i=0; i<18; ++i) {
        printf("msg[size++] =  %d;\n", data[i]);
    }

    // The following is correct; 1.0 is 1<<matrixDepth, and not
    // (1<<matrixDepth)-1.

    us = (data[1]<<8) | data[0];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m00 = (double)s / (double)((1<<matrixDepth));

    us = (data[3]<<8) | data[2];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m01 = (double)s / (double)((1<<matrixDepth));

    us = (data[5]<<8) | data[4];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m02 = (double)s / (double)((1<<matrixDepth));

    // // // 

    us = (data[7]<<8) | data[6];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m10 = (double)s / (double)((1<<matrixDepth));

    us = (data[9]<<8) | data[8];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m11 = (double)s / (double)((1<<matrixDepth));

    us = (data[11]<<8) | data[10];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m12 = (double)s / (double)((1<<matrixDepth));

    // // // 

    us = (data[13]<<8) | data[12];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m20 = (double)s / (double)((1<<matrixDepth));

    us = (data[15]<<8) | data[14];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m21 = (double)s / (double)((1<<matrixDepth));

    us = (data[17]<<8) | data[16];
    memcpy(&s, &us, sizeof(uint16_t));
    matrix.m22 = (double)s / (double)((1<<matrixDepth));

    return true;
}



// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::getBacklightRegRaw(
                                   uint32_t    &p0,    uint32_t &p1,
                                   uint32_t    &p2,    uint32_t &brightness,
                                   uint32_t     connId /* = 0 */,
                                   PluginChain *chain /* = NULL */)
{
    Ddc        *ddc;
    uint32_t    ddcId;
    uint32_t    bufSize;
    uint8_t     msg[1024], *data;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!torinoIdle(connId, chain)) {
        return false;
    }

    //    Send:   0xE2 0xE2     [BLK_ID] [OFFSET]
    //    Recv:   0xE4 [BLK_ID] [OFFSET] 
    //
    // Which should give us up to 32 bytes of goodness. But,
    // matrix values are stored as unsigned ints, normalized
    // to 1.0.

    msg[0] = 0xE2;
    msg[1] = 0xE2;
    msg[2] = 0x82;  // Backlight registers are block 0x82
    msg[3] = 0;     // Offset at 0.
    if (ddc->sendI2cPayload(msg, 4, ddcId) == false) {
        setErrorString( std::string("Send of Table Read cmd failed: ") +
                            ddc->errorString());

        return false;
    }

    // Response is 19 bytes, it seems
    bufSize = 19;
    if (ddc->recvI2cPayload(msg, bufSize, false, ddcId) == false) {
        setErrorString( std::string("Recv of Table Read cmd failed: ") + 
                            ddc->errorString());
        return false;
    }

    p0         = 
    p1         = 
    p2         = 
    brightness = 0;

    data = &msg[3];
    for (int32_t idx=0; idx<(int)bufSize-3; idx+=2) {
        switch (data[idx]) {

            // p0 [Almost Y] - low byte 
            case 0x16:
                p0 |= data[idx+1];
                break;

            // p0 [Almost Y] - high byte
            case 0x17:
                p0 |= data[idx+1] << 8;
                break;

            // p1 [Almost x] - low byte
            case 0x18:
                p1 |= data[idx+1];
                break;

            // p1 [Almost x] - high byte
            case 0x19:
                p1 |= data[idx+1] << 8;
                break;

            // p2 [Almost y] - low byte
            case 0x1a:
                p2 |= data[idx+1];
                break;

            // p2 [Almost y] - high byte
            case 0x1b:
                p2 |= data[idx+1] << 8;
                break;

            // Brightness
            case 0xc4:
                brightness = data[idx+1];
                break;

            default:
                break;
        }   
    }

    return true;
}

// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::setBacklightRegRaw(
                                   uint32_t     p0,    uint32_t  p1,
                                   uint32_t     p2,    uint32_t  brightness,
                                   bool         setP0, bool      setP1, 
                                   bool         setP2, bool      setBrightness,
                                   uint32_t     connId /* = 0 */,
                                   PluginChain *chain  /* = NULL */)
{
    if (!torinoIdle(connId, chain)) {
        return false;
    }

    printf("Sleeping on line %d - do I need to?\n", __LINE__);
    if (!idle(1, chain)) {
        return false;
    }

    return uploadBacklightRegRaw(p0, p1, p2, brightness,
                         setP0, setP1, setP2, setBrightness, 
                            false, connId, chain);
}

// -------------------------------------
//
// Compute the value of the brightness register that gives
// us a certain luminance. Note the brightness control
// only moves in increments of 1 cd/m^2, and ranges between
// 40 cd/m^2 [0x28] and 250 cd/m^2 [0xff].
//
// It's not entirely clear at this point what to do if
// you really want to exceed that range.
//
// virtual
uint32_t
Ookala::DreamColorCtrl::brightnessRegister(
                                    double luminance, 
                                    uint32_t connId /* = 0 */,
                                    PluginChain *chain /* = NULL */)
{
    uint32_t Yint;
    double   t;

    if (luminance < 40)  return 40;
    if (luminance > 250) return 255;

    Yint = static_cast<uint32_t>(luminance+.5);
    switch (Yint) {
        case    250:    return  255;
        case    249:    return  253;
        case    248:    return  252;
        case    247:    return  251;
        case    246:    return  250;
        case    245:    return  249;
        case    244:    return  248;
        case    243:    return  247;
        case    242:    return  246;
        case    241:    return  245;
        case    240:    return  244;
        case    239:    return  243;
        case    238:    return  242;
        case    237:    return  241;
        case    236:    return  240;
        case    235:    return  239;
        case    234:    return  238;
        case    233:    return  237;
        case    232:    return  236;
        case    231:    return  235;
        case    230:    return  234;
        case    229:    return  233;
        case    228:    return  232;
        case    227:    return  231;
        case    226:    return  230;
        case    225:    return  229;
        case    224:    return  228;
        case    223:    return  227;
        case    222:    return  226;
        case    221:    return  225;
        case    220:    return  224;
        case    219:    return  223;
        case    218:    return  222;
        case    217:    return  221;
        case    216:    return  220;
        case    215:    return  219;
        case    214:    return  218;
        case    213:    return  217;
        case    212:    return  216;
        case    211:    return  215;
        case    210:    return  214;
        case    209:    return  213;
        case    208:    return  212;
        case    207:    return  211;
        case    206:    return  210;
        case    205:    return  209;
        case    204:    return  208;
        case    203:    return  207;
        case    202:    return  206;
        case    201:    return  205;
        case    200:    return  204;
        case    199:    return  202;
        case    198:    return  201;
        case    197:    return  200;
        case    196:    return  199;
        case    195:    return  198;
        case    194:    return  197;
        case    193:    return  196;
        case    192:    return  195;
        case    191:    return  194;
        case    190:    return  193;
        case    189:    return  192;
        case    188:    return  191;
        case    187:    return  190;
        case    186:    return  189;
        case    185:    return  188;
        case    184:    return  187;
        case    183:    return  186;
        case    182:    return  185;
        case    181:    return  184;
        case    180:    return  183;
        case    179:    return  182;
        case    178:    return  181;
        case    177:    return  180;
        case    176:    return  179;
        case    175:    return  178;
        case    174:    return  177;
        case    173:    return  176;
        case    172:    return  175;
        case    171:    return  174;
        case    170:    return  173;
        case    169:    return  172;
        case    168:    return  171;
        case    167:    return  170;
        case    166:    return  169;
        case    165:    return  168;
        case    164:    return  167;
        case    163:    return  166;
        case    162:    return  165;
        case    161:    return  164;
        case    160:    return  163;
        case    159:    return  162;
        case    158:    return  161;
        case    157:    return  160;
        case    156:    return  159;
        case    155:    return  158;
        case    154:    return  157;
        case    153:    return  156;
        case    152:    return  155;
        case    151:    return  154;
        case    150:    return  153;
        case    149:    return  151;
        case    148:    return  150;
        case    147:    return  149;
        case    146:    return  148;
        case    145:    return  147;
        case    144:    return  146;
        case    143:    return  145;
        case    142:    return  144;
        case    141:    return  143;
        case    140:    return  142;
        case    139:    return  141;
        case    138:    return  140;
        case    137:    return  139;
        case    136:    return  138;
        case    135:    return  137;
        case    134:    return  136;
        case    133:    return  135;
        case    132:    return  134;
        case    131:    return  133;
        case    130:    return  132;
        case    129:    return  131;
        case    128:    return  130;
        case    127:    return  129;
        case    126:    return  128;
        case    125:    return  127;
        case    124:    return  126;
        case    123:    return  125;
        case    122:    return  124;
        case    121:    return  123;
        case    120:    return  122;
        case    119:    return  121;
        case    118:    return  120;
        case    117:    return  119;
        case    116:    return  118;
        case    115:    return  117;
        case    114:    return  116;
        case    113:    return  115;
        case    112:    return  114;
        case    111:    return  113;
        case    110:    return  112;
        case    109:    return  111;
        case    108:    return  110;
        case    107:    return  109;
        case    106:    return  108;
        case    105:    return  107;
        case    104:    return  106;
        case    103:    return  105;
        case    102:    return  104;
        case    101:    return  103;
        case    100:    return  102;
        case    99:     return  100;
        case    98:     return  99;
        case    97:     return  98;
        case    96:     return  97;
        case    95:     return  96;
        case    94:     return  95;
        case    93:     return  94;
        case    92:     return  93;
        case    91:     return  92;
        case    90:     return  91;
        case    89:     return  90;
        case    88:     return  89;
        case    87:     return  88;
        case    86:     return  87;
        case    85:     return  86;
        case    84:     return  85;
        case    83:     return  84;
        case    82:     return  83;
        case    81:     return  82;
        case    80:     return  81;
        case    79:     return  80;
        case    78:     return  79;
        case    77:     return  78;
        case    76:     return  77;
        case    75:     return  76;
        case    74:     return  75;
        case    73:     return  74;
        case    72:     return  73;
        case    71:     return  72;
        case    70:     return  71;
        case    69:     return  70;
        case    68:     return  69;
        case    67:     return  68;
        case    66:     return  67;
        case    65:     return  66;
        case    64:     return  65;
        case    63:     return  64;
        case    62:     return  63;
        case    61:     return  62;
        case    60:     return  61;
        case    59:     return  60;
        case    58:     return  59;
        case    57:     return  58;
        case    56:     return  57;
        case    55:     return  56;
        case    54:     return  55;
        case    53:     return  54;
        case    52:     return  53;
        case    51:     return  52;
        case    50:     return  51;
        case    49:     return  49;
        case    48:     return  48;
        case    47:     return  47;
        case    46:     return  46;
        case    45:     return  45;
        case    44:     return  44;
        case    43:     return  43;
        case    42:     return  42;
        case    41:     return  41;
        case    40:     return  40;

        default:
            t = (luminance - 40.0) / (250.0 - 40.0);
            if (t > 1) t = 1;
            if (t < 0) t = 0;
            return static_cast<uint32_t>(  t*255.0 + (1.-t)*40.0 + 0.5 );
    }

    return 255;
}


// -------------------------------------
//
// virtual
double
Ookala::DreamColorCtrl::evalTrcToLinear(
                                double              nonlinearX,
                                DreamColorSpaceInfo info,
                                uint32_t            connId /* = 0 */,
                                PluginChain        *chain /* = NULL */)
{
    double gamma, A0, A1, A2, A3;

    info.getTrc(gamma, A0, A1, A2, A3);

    return evalTrcToLinearParam(nonlinearX, A0, A1, A2, A3, gamma, 
                                connId, chain);
}

// -------------------------------------
//
// The current TRC takes the form of:
//
//    x_nonlin <= A0
//        x_lin = x_nonlin / A1
//    else
//        x_lin = [x_nonlin + A2]^gamma
//                [-------------]
//                [    1 + A3   ]
// virtual
double 
Ookala::DreamColorCtrl::evalTrcToLinearParam(
                                     double        nonlinearX,
                                     double        a0,         double a1, 
                                     double        a2,         double a3,
                                     double        gamma,
                                     uint32_t      connId /* = 0 */,
                                     PluginChain  *chain /* = NULL */)
{
    if (nonlinearX < 0) return 0;
    if (nonlinearX > 1) return 1;

    if (nonlinearX < a0) {
        if (fabs(a1) > 1e-10) {
            return nonlinearX / a1;
        } else {
            return pow(nonlinearX, gamma);
        }
    } 

    if (fabs(1 + a3) < 1e-6) return 0;

    double x = (nonlinearX + a2) / (1 + a3);
    if (x < 0) return 0;
    
    return pow(x, gamma);
}


// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::setCalibration(
                               uint32_t                         csIdx, 
                               const DreamColorCalibrationData &data,
                               uint32_t                         connId /* = 0 */,
                               PluginChain                     *chain /* = NULL */)
{

    Ddc         *ddc;
    uint32_t     ddcId;
    DisplayConn  conn;

    setErrorString("");

    if (!getConnFromKey(connId, &conn, chain)) {
        setErrorString("Invalid connection ID.");
        return false;
    }

    // Validate the LUT sizes
    if ((data.getPreLutRed().size()   != conn.pmbInfoHash[MemoryBlockPreLut].tableLength) ||
        (data.getPreLutGreen().size() != conn.pmbInfoHash[MemoryBlockPreLut].tableLength) ||
        (data.getPreLutBlue().size()  != conn.pmbInfoHash[MemoryBlockPreLut].tableLength)) {
        setErrorString("Invalid preLut length.");
        return false;
    }

    if ((data.getPostLutRed().size()   != conn.pmbInfoHash[MemoryBlockPostLut].tableLength) ||
        (data.getPostLutGreen().size() != conn.pmbInfoHash[MemoryBlockPostLut].tableLength) ||
        (data.getPostLutBlue().size()  != conn.pmbInfoHash[MemoryBlockPostLut].tableLength)) {
        setErrorString("Invalid preLut length.");
        return false;
    }

    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if ((chain) && (!chain->wasCancelled())) {
        chain->setUiString("Ui::status_string_minor",
                           "Uploading color space information.");
    }

    if (!setColorSpace(csIdx, connId, chain)) {
        return false;
    }
    if (!idle(1, chain)) {
        return false;
    }

    if (!uploadColorSpaceInfo(data.getColorSpaceInfo(), true, connId, chain)) {
        setColorSpace(csIdx, connId, chain);
        return false;
    }
    
    if (!idle(1, chain)) {
        setColorSpace(csIdx, connId, chain);
        return false;
    }
    if ((chain) && (!chain->wasCancelled())) {
        chain->setUiString("Ui::status_string_minor", "Uploading PostLUT.");
    }


    // Upload post-lut
    if (!uploadLut16(MemoryBlockPostLut, 
                        conn.pmbInfoHash[MemoryBlockPostLut].tableLength, 
                        data.getPostLutRed(), 
                        data.getPostLutGreen(), 
                        data.getPostLutBlue(),
                        true, 
                        connId, 
                        chain)) {
        setColorSpace(csIdx, connId, chain);
        return false;
    }

    if (!idle(1, chain)) {
        setColorSpace(csIdx, connId, chain);
        return false;
    }
    if ((chain) && (!chain->wasCancelled())) {
        chain->setUiString("Ui::status_string_minor", "Uploading Matrix.");
    }


    // Upload matrix
    printf("Sending matrix\n");
    if (!uploadMatrix(data.getMatrix(), true, connId, chain)) {
        return false;
    }

    if ((chain) && (!chain->wasCancelled())) {
        chain->setUiString("Ui::status_string_minor", "Uploading PreLUT.");
    }
    if (!idle(1, chain)) {
        setColorSpace(csIdx, connId, chain);
        return false;
    }

    // Upload pre-lut
    printf("Sending pre lut\n");
    if (!uploadLut16(MemoryBlockPreLut, 
                        conn.pmbInfoHash[MemoryBlockPreLut].tableLength, 
                        data.getPreLutRed(),
                        data.getPreLutGreen(),
                        data.getPreLutBlue(),
                        true, 
                        connId,
                        chain)) {
        return false;
    }
 
    // Upload registers
    if (!idle(1, chain)) {
        setColorSpace(csIdx, connId, chain);
        return false;
    }
    if ((chain) && (!chain->wasCancelled())) {
        chain->setUiString("Ui::status_string_minor",
                           "Uploading backlight controls.");
    }


    if (!uploadBacklightRegRaw(
                        data.getRegP0(),
                        data.getRegP1(),
                        data.getRegP2(),
                        data.getRegBrightness(),
                        true, 
                        true, 
                        true, 
                        true, 
                        true, 
                        connId,
                        chain)) {
        return false;
    }
    if (!idle(5, chain)) {
        setColorSpace(csIdx, connId, chain);
        return false;
    }

    // 0xE1 0x02 to display/commit. - then 0xE1 0x03 to save
    if (chain) {
        chain->setUiString("Ui::status_string_minor",
                           "Saving uploaded data.");
    }

    printf("commit\n");
    if (!ddc->setVcpFeature(0xE1, (2 << 8), ddcId)) {
        setErrorString( std::string("Commit failed. ") + 
                                 ddc->errorString());
        return false;
    }
    if (!idle(5, chain)) {
        return false;
    }

    printf("save\n");
    if (!ddc->setVcpFeature(0xE1, (3 << 8), ddcId)) {
        setErrorString( std::string("Commit failed. ") + ddc->errorString());
        return false;
    }
    if (!idle(5, chain)) {
        return false;
    }

    return true;
}


// -------------------------------------
//
// virtual
bool
Ookala::DreamColorCtrl::getCalibration(
                               DreamColorCalibrationData &data,
                               uint32_t                   connId /* = 0 */,
                               PluginChain               *chain /* = NULL */)
{
    DreamColorSpaceInfo   info;
    std::vector<uint32_t> preLut[3], postLut[3];
    Mat33                 matrix;
    uint32_t              reg[4];

    setErrorString("");

    if (!getColorSpaceInfo(info, connId, chain)) {
        return false;
    }
    
    if (!getPreLut(preLut[0], preLut[1], preLut[2], connId, chain)) {
        return false;
    }

    if (!getPostLut(postLut[0], postLut[1], postLut[2], connId, chain)) {                     
        return false;
    }

    if (!getMatrix(matrix, connId, chain)) {
        return false;
    }

    if (!getBacklightRegRaw(reg[0], reg[1], reg[2], reg[3], connId, chain)) {
        return false;
    }

    data.setColorSpaceInfo(info);

    data.setPreLutRed(preLut[0]);
    data.setPreLutGreen(preLut[1]);
    data.setPreLutBlue(preLut[2]);

    data.setMatrix(matrix);

    data.setPostLutRed(postLut[0]);
    data.setPostLutGreen(postLut[1]);
    data.setPostLutBlue(postLut[2]);

    data.setRegP0(reg[0]);
    data.setRegP1(reg[1]);
    data.setRegP2(reg[2]);
    data.setRegBrightness(reg[3]);

    return true;
}


// -------------------------------------
//
// protected
Ookala::Ddc *
Ookala::DreamColorCtrl::getPluginFromKey(
                                 uint32_t     connId, 
                                 uint32_t    &devId,
                                 PluginChain *chain)
{
    std::vector<Plugin *> plugins;

    if (mRegistry == NULL) return NULL;
    
    if (!mDreamColorCtrlData) return NULL;

    if (mDreamColorCtrlData->mConnections.empty()) return NULL;

    if (connId == 0) {
        devId   = (*(mDreamColorCtrlData->mConnections.begin())).second.pluginId;


        plugins = mRegistry->queryByName(
                    (*(mDreamColorCtrlData->mConnections.begin())).second.pluginName);

        if (plugins.empty()) return NULL;

        return (Ddc *)(plugins[0]);
    }

    std::map<uint32_t, DisplayConn>::iterator value =
                 mDreamColorCtrlData->mConnections.find(connId);
    if (value == mDreamColorCtrlData->mConnections.end()) {
        return NULL;
    }

    devId  = (*value).second.pluginId;

    plugins = mRegistry->queryByName(
                (*value).second.pluginName);

    if (plugins.empty()) return NULL;

    return (Ddc *)(plugins[0]);
}

// -------------------------------------
//
// protected
bool
Ookala::DreamColorCtrl::getConnFromKey(
                               uint32_t     connId, 
                               DisplayConn *conn,
                               PluginChain *chain)
{
    if ((!mDreamColorCtrlData) || (!conn)) return false;

    if (mDreamColorCtrlData->mConnections.empty()) return false;

    if (connId == 0) {
        *conn   = (*(mDreamColorCtrlData->mConnections.begin())).second;
        return true;
    }

    std::map<uint32_t, DisplayConn>::iterator value =
                 mDreamColorCtrlData->mConnections.find(connId);
    if (value == mDreamColorCtrlData->mConnections.end()) {
        return false;
    }

    *conn = (*value).second;
    return true;
} 


// -------------------------------------
//
// Send a message to retrieve a 16-bit LUT, and then read
// out the values. 
//
//   lutPMB    - The memory block ID for the LUT (1-post, 3-pre)
//   lutLength - Number of entries in the LUT
//   redLut    - Storage for the red channel
//   greenLut  - 
//   blueLut   -              
//
// protected
bool          
Ookala::DreamColorCtrl::downloadLut16(
                              uint32_t               lutPMB,
                              uint32_t               lutLength,
                              std::vector<uint32_t> &redLut,
                              std::vector<uint32_t> &greenLut,
                              std::vector<uint32_t> &blueLut,
                              uint32_t               connId,
                              PluginChain           *chain)
{
    Ddc        *ddc;
    uint32_t    ddcId;
    uint32_t    bufSize;
    uint8_t     msg[1024], *data;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    redLut.clear();
    greenLut.clear();
    blueLut.clear();

    //    Send:   0xE2 0xE2     [BLK_ID] [OFFSET]
    //    Recv:   0xE4 [BLK_ID] [OFFSET] 
    //
    // LUT data comes down in chunks of 96 bytes worth of data.
    // If our LUT has 1024 entries, that gives us 64 chunks.
    //  ( 2 bytes/entry * 3 channels * 1024 entries )

    uint32_t channel = 0;
    for (uint32_t off=0; off<(2*3*lutLength) / 96; ++off) {

        if (!torinoIdle(connId, chain)) {
            return false;
        }

        msg[0] = 0xE2;
        msg[1] = 0xE2;
        msg[2] = lutPMB;  // Pre-lut is block 3, post-lut is block 1
        msg[3] = off;     // Block offset
        if (ddc->sendI2cPayload(msg, 4, ddcId) == false) {
            setErrorString( std::string("Send of Table Read cmd failed: ") +
                                    ddc->errorString());

            return false;
        }

        bufSize = 100;
        if (ddc->recvI2cPayload(msg, bufSize, false, ddcId) == false) {
            setErrorString( std::string("Recv of Table Read cmd failed: ") +
                                    ddc->errorString());
            return false;
        }

        // Data comes back in 96-byte packets, plus 3 bytes 
        // at the head:
        //    0xE4 0x02 [OFFSET]
        // And one byte at the end:
        //    0x7e
        //
        // XXX: What does the byte at the tail mean?
        //
        // For fixed point unsigned int values, this gives us
        // 64 packets with 96 bytes each: 
        //       ( 2 bytes *  3 colors * 1024 entries )
        // 
        // Data is structured as N samples of red, then
        // N Samples of green, then finally the blue.
        //
        // If we read 4-bytes on the first packet, this means
        // something about the display not having any
        // data to give us.
        if (bufSize != 100) {
            setErrorString("Read too little data for color space.");
            return false;
        }

        printf("0x%x 0x%x 0x%x 0x%x\n", msg[0], msg[1], msg[2], msg[3]);

        // The 48 here comes from the chunk of data having 96 bytes,
        // but each LUT entry has 2 bytes. 96 = 48*2.
        data = &msg[3];
        for (int idx=0; idx<48; ++idx) {

            switch (channel) {
                case 0:
                    redLut.push_back((data[2*idx+1] << 8) | data[2*idx]);
                    if (redLut.size() == lutLength) {
                        channel++;
                    }
                    break;
                
                case 1:
                    greenLut.push_back((data[2*idx+1] << 8) | data[2*idx]);
                    if (greenLut.size() == lutLength) {
                        channel++;
                    }
                    break;

                case 2:
                    blueLut.push_back((data[2*idx+1] << 8) | data[2*idx]);
                    if (blueLut.size() == lutLength) {
                        channel++;
                    }
                    break;

                default:
                    setErrorString("Error decoding channels.");
                    return false;
            }
        }
    }

    return true;
}

// -------------------------------------
//
// Upload a 16-bit LUT to a particular PMB. 
//
//   lutPMB    - The memory block ID for the LUT (1-post, 3-pre)
//   lutLength - Number of entries in the LUT
//   redLut    - Storage for the red channel
//   greenLut  - 
//   blueLut   -              
//
// protected
bool          
Ookala::DreamColorCtrl::uploadLut16(
                            uint32_t                     lutPMB,
                            uint32_t                     lutLength,
                            const std::vector<uint32_t> &redLut,
                            const std::vector<uint32_t> &greenLut,
                            const std::vector<uint32_t> &blueLut,
                            bool                         uploadingAllData,
                            uint32_t                     connId,
                            PluginChain                 *chain)
{
    Ddc                  *ddc;
    uint32_t              ddcId;
    std::vector<uint32_t> packedLuts;
    uint32_t              numPackets, size;
    uint8_t               msg[1024];
    
    uint32_t              packetSize = 96;

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (redLut.size()   < lutLength) return false;
    if (greenLut.size() < lutLength) return false;
    if (blueLut.size()  < lutLength) return false;

    // No logic for writing partial packets yet.
    if ( (2*3*lutLength) % packetSize) return false;
    
    // Take our RGB luts and place them end to end
    for (uint32_t idx=0; idx<lutLength; ++idx) {
        packedLuts.push_back(redLut[idx]);
    }
    for (uint32_t idx=0; idx<lutLength; ++idx) {
        packedLuts.push_back(greenLut[idx]);
    }
    for (uint32_t idx=0; idx<lutLength; ++idx) {
        packedLuts.push_back(blueLut[idx]);
    }

    // 2 bytes per entry, 3 channels,  by packetSize bytes per packet.
    numPackets = 2 * 3 * lutLength / packetSize;

    // Now, build each packet with part of the LUT data.
    for (uint32_t packetIdx=0; packetIdx<numPackets; ++packetIdx) {

        if (!torinoInState(packetIdx*packetSize, lutPMB<<8, connId, chain)) {
            return false;
        }

        // If we were cancelled, reset the Torino state.
        if ((chain) && (chain->wasCancelled())) {
            uint32_t csIdx;

            getColorSpace(csIdx, connId, chain);
            setColorSpace(csIdx, connId, chain);
            return false;            
        }

        if (chain) {
            char buf[1024];
            
            if (lutPMB == MemoryBlockPreLut) {
#ifndef _WIN32                
                sprintf(buf, "Uploading PreLUT, %.0f %% complete.",                               
#else
                sprintf_s(buf, 1023, "Uploading PreLUT, %.0f %% complete.",
#endif
                               100.*(double)packetIdx / (double)(numPackets-1));

                chain->setUiString("Ui::status_string_minor", std::string(buf));
            } else if (lutPMB == MemoryBlockPostLut) {
#ifndef _WIN32
                sprintf(buf, "Uploading PostLUT, %.0f %% complete.",
#else
                sprintf_s(buf, 1023, "Uploading PostLUT, %.0f %% complete.",
#endif
                               100.*(double)packetIdx / (double)(numPackets-1));

                chain->setUiString("Ui::status_string_minor", std::string(buf));
            }
        }

        size = 0;
        msg[size++] = 0xE7;
        msg[size++] = 0xE2;
        msg[size++] = lutPMB;
        msg[size++] = 0;                // ??
        msg[size++] = lutPMB;
        msg[size++] = 0;                        // Format == uncompressed
        msg[size++] = packetSize & 0xff;
        msg[size++] = (packetSize >> 8) & 0xff;
        msg[size++] = (packetIdx * packetSize) & 0xff;
        msg[size++] = ((packetIdx * packetSize) >> 8) & 0xff;

        //printf("\t%x %x %x %x %x %x %x %x %x %x\n",
        //       msg[0], msg[1], msg[2], msg[3], msg[4], msg[5],
        //        msg[6], msg[7], msg[8], msg[9]);

        for (uint32_t dataIdx=0; dataIdx<packetSize/2; ++dataIdx) {
        
            uint32_t value = packedLuts[packetIdx*packetSize/2 + dataIdx];

            msg[size++] = value & 0xff;
            msg[size++] = (value >> 8) & 0xff;
        }
        
        if (ddc->sendI2cPayload(msg, size, ddcId) == false) {
            setErrorString( std::string("Send of Table Write cmd failed: ") +
                                ddc->errorString());
            return false;
        }
        //printf("%d of %d, %d bytes\n", packetIdx, numPackets-1, size);

        //sleep(1);
    }

    return true;
}

// -------------------------------------
//
// protected
bool
Ookala::DreamColorCtrl::downloadColorSpaceInfo(
                                       DreamColorSpaceInfo &info, 
                                       uint32_t             csIdx, 
                                       uint32_t             connId, 
                                       PluginChain          *chain)
{
    uint32_t bufSize;
    uint8_t  msg[1024], *data;
    Ddc     *ddc;
    uint32_t ddcId;
    Yxy      YxyValue;

    info.setConnId(connId);

    if (csIdx == 0xff) {
        uint32_t currIdx;
        if (!getColorSpace(currIdx, connId, chain)) {
            setErrorString("Unable to get current color space.");
            return false;
        }
        info.setPresetId(currIdx);
    } else {
        info.setPresetId(csIdx);
    }

printf("downloadColorSpaceInfo( %d ), preset is %d\n",
            csIdx, info.getPresetId());

    if ((csIdx != 0xff) && (csIdx >= numColorSpace(connId, chain))) {
        setErrorString("Invalid color space index.");
        return false;
    }

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!torinoIdle(connId, chain)) {
        return false;
    }


    // Table Read 0xE2 or Table Write 0xE7 a 78-byte block of
    // data describing color information.
    //    Read:   0xE2 0xE2 0x00 [CS] [checksum]
    //    Write:  0xE7 0xE2 0x00 0x00 b1 b2 ... b78 [checksum]
    //
    // I _think_ the 0x00/0x00 here is in reference to .. I think
    // the first one may be BlockID, the second should be 0?
    //
    // You should choose the gamut to work on first (0xE1 above).
    //
    // (note, the spec indicies start at 1, not 0, so bewarned
    //  but gamut numbering starts at 0)

    msg[0] = 0xE2;
    msg[1] = 0xE2;
    msg[2] = 0;
    msg[3] = csIdx & 0xff;
    if (ddc->sendI2cPayload(msg, 4, ddcId) == false) {
        setErrorString( std::string("Send of Table Read cmd failed: ") + 
                            ddc->errorString());
        return false;
    }
    bufSize = 81;
    if (ddc->recvI2cPayload(msg, bufSize, false, ddcId) == false) {
        setErrorString( std::string("Recv of Table Read cmd failed: ") + 
                            ddc->errorString());
        return false;
    }

    if (bufSize != 81) {
        setErrorString("Read too little data for color space.");
        return false;
    }
    
    // Byte 0:     Enabled/Disabled        [1=enabled, 2=disabled]
    // Byte 1:     Calibrated/Uncalibrated [1=calib,   2=non-calib]
    // Byte 2-3:   Backlight hours
    // Byte 4-20:  Name.         
    //                           
    // Byte 21-24: White Y         
    // Byte 25-28: White x
    // Byte 29-32: White y
    //
    // Byte 33-36: Red x
    // Byte 37-40: Red y
    //
    // Byte 41-44: Green x
    // Byte 45-48: Green y
    //
    // Byte 49-52: Blue x
    // Byte 53-56: Blue y
    //
    // Byte 57-60: Gamma
    // Byte 61-64: Gamma A0
    // Byte 65-68: Gamma A1
    // Byte 69-72: Gamma A2 
    // Byte 73-76: Gamma A3

    data = &msg[3];

    if (data[0] == 1) {
        info.setEnabled(true);
    } else {
        info.setEnabled(false);
    }

    if (data[1] == 1) {
        info.setCalibrated(true);
    } else {
        info.setCalibrated(false);
    }

    info.setCalibratedTimeRemaining((data[3]<<8) | data[2]);

    {
        std::string name = "";
        for (int i=4; i<=20; ++i) {
            if (data[i] == 0) break;

            name += data[i];;
        }
        info.setName(name);
    }

    YxyValue.Y = fourBytesToFloat(&data[21]);
    YxyValue.x = fourBytesToFloat(&data[25]);
    YxyValue.y = fourBytesToFloat(&data[29]);
    info.setWhite(YxyValue);

printf("dcc white: %f %f %f\n",
        YxyValue.Y, YxyValue.x, YxyValue.y);

    YxyValue.Y        = 0;
    YxyValue.x        = fourBytesToFloat(&data[33]);
    YxyValue.y        = fourBytesToFloat(&data[37]);
    info.setRed(YxyValue);


    YxyValue.Y      = 0;
    YxyValue.x      = fourBytesToFloat(&data[41]);
    YxyValue.y      = fourBytesToFloat(&data[45]);
    info.setGreen(YxyValue);


    YxyValue.Y       = 0;
    YxyValue.x       = fourBytesToFloat(&data[49]);
    YxyValue.y       = fourBytesToFloat(&data[53]);
    info.setBlue(YxyValue);


    info.setTrc(fourBytesToFloat(&data[57]),
                fourBytesToFloat(&data[61]),
                fourBytesToFloat(&data[65]),
                fourBytesToFloat(&data[69]),
                fourBytesToFloat(&data[73]));

    return true;
}



// -------------------------------------
//
// protected 
bool
Ookala::DreamColorCtrl::uploadColorSpaceInfo(
                                     DreamColorSpaceInfo info,
                                     bool                uploadingAllData,
                                     uint32_t            connId, 
                                     PluginChain        *chain)
{
    Ddc     *ddc;
    uint32_t ddcId;
    uint8_t  msg[1024];
    double   trcGamma, trcA[4];

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!torinoIdle(connId, chain)) {
        return false;
    }   

    msg[0] = 0xE7;
    msg[1] = 0xE2;
    msg[2] = 0;
    msg[3] = 0;

    if (uploadingAllData) {
        msg[4] = 1;     // Mode, write all data
    } else {
        msg[4] = 2;     // Mode, write colorspace only
    }
    
    if (info.getEnabled()) {
        msg[5] = 1;     // Colorspace enabled
    } else {
        msg[5] = 2;
    }

    if (info.getCalibrated()) {
        msg[6] = 1;     // Calibrated;
    } else {
        msg[6] = 2;      
    }

    // XXX: Currently, this seems buggy - it looks like the
    //       low byte value ends up as both bytes
    //

    // Countdown of calibrated # hrs, low byte
    msg[7] = info.getCalibratedTimeRemaining() & 0xff; //0xe8;  

    // Hi-byte
    msg[8] = (info.getCalibratedTimeRemaining() >> 8) & 0xff; //3;    

    // Name, 17 bytes long
    for (int idx=9; idx<=25; ++idx) {
        msg[idx] = 0;
    }
    std::string subName = info.getName().substr(0, 17);
    for (uint32_t idx=0; idx<subName.length(); ++idx) {
        msg[9+idx] = subName[idx];
    }

    floatToFourBytes(info.getWhite().Y, &msg[26]); // White Y 
    floatToFourBytes(info.getWhite().x, &msg[30]); // White x 
    floatToFourBytes(info.getWhite().y, &msg[34]); // White y 

    floatToFourBytes(info.getRed().x,    &msg[38]); // Red x
    floatToFourBytes(info.getRed().y,    &msg[42]); // Red y

    floatToFourBytes(info.getGreen().x,  &msg[46]); // Green x
    floatToFourBytes(info.getGreen().y,  &msg[50]); // Green y

    floatToFourBytes(info.getBlue().x,   &msg[54]); // Blue x
    floatToFourBytes(info.getBlue().y,   &msg[58]); // Blue y


    info.getTrc(trcGamma, trcA[0], trcA[1], trcA[2], trcA[3]);
    floatToFourBytes(trcGamma, &msg[62]); // Gamma

    floatToFourBytes(trcA[0],    &msg[66]); // TRC A0
    floatToFourBytes(trcA[1],    &msg[70]); // TRC A1
    floatToFourBytes(trcA[2],    &msg[74]); // TRC A2
    floatToFourBytes(trcA[3],    &msg[78]); // TRC A3

    if (ddc->sendI2cPayload(msg, 82, ddcId) == false) {
        setErrorString( std::string("Send of Table Read cmd failed: ") +
                            ddc->errorString());
        return false;
    }

    return true;
}


// -------------------------------------
//
// protected
bool
Ookala::DreamColorCtrl::uploadMatrix(
                             const Mat33  &matrix,
                             bool          uploadingAllData,
                             uint32_t      connId,
                             PluginChain  *chain)
{
    Ddc        *ddc;
    uint32_t    ddcId;
    DisplayConn conn;
    uint32_t    size = 0;
    uint8_t     msg[1024];
    double      matrixDat[9];

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    if (!getConnFromKey(connId, &conn, chain)) {
        return false;
    }

    if (!torinoInState(0, 0x0200, connId, chain)) {
        return false;
    }

    size = 0;
    msg[size++] = 0xE7;              // Table Write
    msg[size++] = 0xE2;              // VCP command opcode
    msg[size++] = MemoryBlockMatrix;
    msg[size++] = 0;
    msg[size++] = MemoryBlockMatrix;
    msg[size++] = 0;                 // Uncompressed data
   
    msg[size++] = 18;                // Low byte of data length. 
                                     // 2 bytes/entry * 9 entries

    msg[size++] = 0;                 // Hi byte of data lenght.

    msg[size++] = 0;
    msg[size++] = 0;

    matrixDat[0] = matrix.m00;
    matrixDat[1] = matrix.m01;
    matrixDat[2] = matrix.m02;

    matrixDat[3] = matrix.m10;
    matrixDat[4] = matrix.m11;
    matrixDat[5] = matrix.m12;

    matrixDat[6] = matrix.m20;
    matrixDat[7] = matrix.m21;
    matrixDat[8] = matrix.m22;

    // Look at the memoery block for bit depth instead of hard-coding.
    for (int matrixIdx=0; matrixIdx<9; ++matrixIdx) {

        // This really is supposed to be 4096, and not 4095.
        int16_t value = static_cast<int16_t>(matrixDat[matrixIdx] * 4096.0 + 0.5);

        printf("%d: %f -> %d [%d]\n", matrixIdx, matrixDat[matrixIdx],
                                 value, value & 0xffff);

        msg[size++] = value & 0xff;
        msg[size++] = (value >> 8) & 0xff;
    }
    
    if (ddc->sendI2cPayload(msg, size, ddcId) == false) {
        setErrorString( std::string("Send of Table Write cmd failed: ") +
                            ddc->errorString());
        return false;
    }

    return true;
}


// -------------------------------------
//
// protected
bool
Ookala::DreamColorCtrl::uploadBacklightRegRaw(
                                      uint32_t      p0,    uint32_t  p1,
                                      uint32_t      p2,    uint32_t  brightness,
                                      bool          setP0, bool      setP1, 
                                      bool          setP2, bool      setBrightness,
                                      bool          uploadingAllData,
                                      uint32_t      connId,
                                      PluginChain  *chain)
{
    Ddc        *ddc;
    uint32_t    ddcId;
    uint32_t    size;
    uint8_t     msg[1024];

    setErrorString("");
    ddc = getPluginFromKey(connId, ddcId, chain);
    if (ddc == NULL) {
        setErrorString("No DDC connection available.");
        return false;
    }

    // We need different torino tests based on what we're doing
    if (uploadingAllData) {
        if (!torinoInState(0, 0x8200, connId, chain)) {
            return false;
        }
    } else {
        if (!torinoIdle(connId, chain)) {
            return false;
        }
    }

    // Send 0xE7 0xE2 [BlkID] [Offset] [BlkID] [Format]
    msg[0] = 0xE7;
    msg[1] = 0xE2;
    msg[2] = 0x82; // Register Block ID
    msg[3] = 0;     
    msg[4] = 0x82; // Register Block ID
    msg[5] = 0x80; // Data format - address/value pairs

    msg[6] = 4;   // # of bytes of address/value pairs - inc later
    msg[7] = 0;  

    msg[8] = 0x04; // Use xyY?
    msg[9] = 0x02; // ???

    size = 10;

    if (setP0) {
        msg[size++] = 0x16;
        msg[size++] = p0 & 0xff;         // p0 low 

        msg[size++] = 0x17;
        msg[size++] = (p0 >> 8)  & 0xff; // p0 high 

        msg[6]     += 4;
    }

    if (setP1) {
        msg[size++] = 0x18;
        msg[size++] = p1 & 0xff;         // p1 low 

        msg[size++] = 0x19;
        msg[size++] = (p1 >> 8)  & 0xff; // p1 high 

        msg[6]     += 4;
    }

    if (setP2) {
        msg[size++] = 0x1a;
        msg[size++] = p2 & 0xff;         // p2 low

        msg[size++] = 0x1b;
        msg[size++] = (p2 >> 8) & 0xff;  // p2 high 

        msg[6]     += 4;
    }

    if (setBrightness) {
        msg[size++] = 0xc4;
        msg[size++] = brightness & 0xff;   // Brightness, more-or-less in cd/m^2

        msg[6]     += 2;
    }

    msg[size++] = 0x1;
    msg[size++] = 0x12;

    if (ddc->sendI2cPayload(msg, size, ddcId) == false) {
        setErrorString( std::string("Send of Table Write cmd failed: ") +
                            ddc->errorString());
        return false;
    }

    return true;
}

// -------------------------------------
//
// Occasionally, we'll read too fast for things to handle. In
// that case, we should back-off for a slight bit and try
// again. This unfortunatly happens more often that we would
// like...
//
// protected
bool
Ookala::DreamColorCtrl::retryGetVcpFeature(
                                   uint8_t       opcode,  uint16_t &currVal, 
                                   uint16_t     &maxVal,  uint32_t connId,
                                   PluginChain  *chain)
{
    uint32_t ddcId;
    Ddc     *ddc;

    ddc = getPluginFromKey(connId, ddcId, chain);
    if (!ddc) {
        setErrorString("Unknown connection.");
        return false;
    }

    if (!ddc->getVcpFeature(opcode, currVal, maxVal, ddcId)) {
        uint32_t sleepUs = 10000;
        bool     success = false;
        int      iter    = 0;

        while ((iter < 5) && (!success)) {
#ifndef _WIN32
            usleep(sleepUs);
#else
            printf("sleeping on DreamColorCtrl::retryGetVcpFeature()\n");
            Sleep(sleepUs / 1000);
            printf("done\n");
#endif

            if (ddc->getVcpFeature(opcode, currVal, maxVal, ddcId)) {
                success = true;
                break;
            }

            iter++;
            sleepUs *= 2;
        }

        if (!success) {
            setErrorString( std::string("VCP Feature Get failed: ") +
                                 ddc->errorString());
            return false;   
        }
    } 

    return true;
}


// -------------------------------------
// This tests the status of the Torino unit. This should
// be true before sending any data to the PMBs. 
//
// If the return value was false, semething went horribly
// wrong, beyone the fact that the unit was busy, or
// if the unit is not idle.
//
// protected
bool
Ookala::DreamColorCtrl::torinoIdle(uint32_t      connId,
                                   PluginChain  *chain)
{
    return torinoInState(0, 0xffff, connId, chain);
}

// -------------------------------------
//
// Tornio can have various states other than 'idle', which
// you're free to specify as the 'currState' and 'maxState'.
//
// Returns false if communications failed or if we're
// not in the specified state.
//
// protected
bool
Ookala::DreamColorCtrl::torinoInState( 
                              uint16_t      currState, 
                              uint16_t      maxState,
                              uint32_t      connId,
                              PluginChain  *chain)
{
    uint32_t ddcId;
    Ddc     *ddc;
    uint16_t curr, max;
    bool     isInState = false;
    int32_t  numFailures     = 0;
    int32_t  failuresAllowed = 5;

    ddc = getPluginFromKey(connId, ddcId, chain);
    if (!ddc) {
        setErrorString("No DDC connection available.");
        return false;
    }


    setErrorString("");
    for (uint32_t iter=0; iter<100; ++iter) {
        if (!retryGetVcpFeature(0xe2, curr, max, connId, chain)) {
            printf("Get failed\n");
            numFailures++;
            setErrorString( std::string("VCP get failed: ") +
                                        ddc->errorString());

            if (numFailures >= failuresAllowed) {
                break;
            }
        } else {
            numFailures = 0;
            printf("iter %d: 0x%x 0x%x\n", iter, max, curr);
            if ((max == maxState) && (curr == currState)) {
                isInState = true;
                setErrorString("");
                break;
            }
        }

#ifndef _WIN32
        usleep(100000);
#else
        Sleep(100);
#endif

        // If we're trying to wait for Torino to idle (curr = 0, max=0xffff)
        // and the unit is expecting color space data to be uploaded, 
        // then we're never going to hit the idle state. In this case,
        // we should re-send the current color space to reset the
        // color space upload.
        if ((currState == 0) && (maxState = 0xffff) && 
            (curr != 0xffff) && (max != 0xffff)) {
            uint32_t csIdx;

            getColorSpace(csIdx, connId, chain);

            setColorSpace(csIdx, connId, chain);
        }
    }

    if (!isInState)  {
        setErrorString( errorString() + 
                            std::string("Torino is not in requested state."));
    }

    return isInState;
}

// -------------------------------------
//
// protected
double
Ookala::DreamColorCtrl::fourBytesToFloat(uint8_t *buf)
{
    float    floatVal;
    uint8_t *floatBuf;

    // XXX: This needs a host endian check

    floatBuf = (uint8_t *)(&floatVal);
    floatBuf[0] = buf[0];
    floatBuf[1] = buf[1];
    floatBuf[2] = buf[2];
    floatBuf[3] = buf[3];

    return static_cast<double>(floatVal);
}

// -------------------------------------
//
// protected
void
Ookala::DreamColorCtrl::floatToFourBytes(double in, uint8_t *out)
{
    float    floatVal = static_cast<float>(in);
    uint8_t *floatBuf = (uint8_t *)(&floatVal);

    out[0] = floatBuf[0];
    out[1] = floatBuf[1];
    out[2] = floatBuf[2];
    out[3] = floatBuf[3];
}

// -------------------------------------
//
// Periodically test if we've been cancelled or not. If so,
// return false.
//
// protected
bool
Ookala::DreamColorCtrl::idle(double seconds, PluginChain *chain)
{
    uint32_t sleepUs = static_cast<uint32_t>( (seconds - floor(seconds)) * 1000000.0);

#ifndef _WIN32
    usleep(sleepUs);
#else
    Sleep(sleepUs / 1000.0);
#endif

    if (chain) {
        if (chain->wasCancelled()) {
            setErrorString("Execution cancelled.");
            return false;
        }
    } 

    for (uint32_t i=0; i<static_cast<uint32_t>(floor(seconds)); ++i) {
#ifndef _WIN32
        sleep(1);
#else
        Sleep(1000);
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


// -------------------------------------
//
// Enumerate displays. 
//
// virtual protected
bool
Ookala::DreamColorCtrl::_preRun(PluginChain *chain)
{
    if (!enumerate(chain)) {
        return false;
    }

    return true;
}


// -------------------------------------
//
// Respond to a few commands:
//
//    DreamColorCtrl::setColorSpace [INT] --> Change to a particular preset.
//
// virtual protected
bool
Ookala::DreamColorCtrl::_run(PluginChain *chain)
{
    Dict        *chainDict = chain->getDict();
    DictItem    *item      = NULL;
    IntDictItem *intItem   = NULL;

    if (!chainDict) {
        setErrorString("No chain dict found in DreamColorCtrl::_run().");
        return false;
    }

    item = chainDict->get("DreamColorCtrl::setColorSpace");
    if (item) {
        intItem = dynamic_cast<IntDictItem *>(item);
        if (intItem) {
            if (!setColorSpace(intItem->get())) {
                return false;
            }
        }
    } 



    return true;
}




