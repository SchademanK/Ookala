// --------------------------------------------------------------------------
// $Id: Ddc.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <time.h>

#include "Types.h"
#include "Ddc.h"


// ------------------------------------

Ookala::Ddc::Ddc():
    Plugin(),
    mDebug(false),
    mNextKey(1)
{
    setName("Ddc");

    addAttribute("DDC");
    addAttribute("DDC/CI");
    addAttribute("DDCCI");

    addAttribute("ddc");
    addAttribute("ddc/ci");
    addAttribute("ddcci");

    mDdcData = new _Ddc;
}

// ------------------------------------
//
Ookala::Ddc::Ddc(const Ddc &src):
    Plugin(src)
{
    mDebug     = src.mDebug;
    mDebugFile = src.mDebugFile;

    mNextKey   = src.mNextKey;

    mDdcData = new _Ddc;

    if (src.mDdcData) {
        mDdcData->edidData = src.mDdcData->edidData;
    }
}

// ------------------------------------
//
// virtual 
Ookala::Ddc::~Ddc()
{
    if (mDdcData) {
        delete mDdcData;
        mDdcData = NULL;
    }
}

// ----------------------------
//
Ookala::Ddc &
Ookala::Ddc::operator=(const Ddc &src)
{
    if (this != &src) {
        Plugin::operator=(src);

        if (mDdcData) {
            delete mDdcData;
            mDdcData = NULL;
        }

        if (src.mDdcData) {
            mDdcData = new _Ddc();
            *mDdcData = *(src.mDdcData);
        }

        mDebug     = src.mDebug;
        mDebugFile = src.mDebugFile;
        mNextKey   = src.mNextKey;
    }

    return *this;
}


// ------------------------------------
//
// virtual 
std::vector<uint32_t>
Ookala::Ddc::devices()
{
    std::vector<uint32_t> devs;
    return devs;
}

// ------------------------------------
//
// virtual 
bool
Ookala::Ddc::enumerate()
{
    return true;
}

// ------------------------------------
//
// virtual 
bool 
Ookala::Ddc::getEdid13(struct Edid1_3 &edid, uint32_t devId /* = 0 */)
{
    uint32_t realId;

    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID.");
        return false;
    }

    if (!getEdid(realId, edid)) {
        setErrorString("Invalid device ID.");
        return false;
    }

    return true;
}

// ------------------------------------
//
// Set a VCP feature, message opcode 0x03
//   0 - 0x03
//   1 - Key of value to write
//   2 - New value, high byte
//   3 - New value, low byte
//
// virtual 
bool 
Ookala::Ddc::setVcpFeature(uint8_t opcode, uint16_t value, uint32_t devId /* = 0 */)
{
    uint8_t msg[4];
    uint32_t realId;

    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID.");
        return false;
    }

    // And start building the message
    msg[0] = 3;
    msg[1] = opcode;
    msg[2] = static_cast<uint8_t>((value >> 8) & 0xff);
    msg[3] = static_cast<uint8_t>(value & 0xff);

    return sendI2cPayload(msg, 4, realId);
}

// ------------------------------------
//
// Retrieve the current and max values for a VCP feature
//
// Send the message, 2 bytes:
//   0 - 0x01
//   1 - opcode for value to retrieve
//
// Read the message, 8 bytes:
//   0 - 0x02
//   1 - Error code. 0 -> success, 
//                   1 -> unsupported value
//   2 - Key of value to read
//   3 - Type code.  0 -> set parameter, 
//                   1 -> momentary (like degauss)
//   4 - Maximum Value, high byte
//   5 - Maximum Value, low byte
//   6 - Current value, high byte
//   7 - Current value, low byte
//
// virtual
bool 
Ookala::Ddc::getVcpFeature(uint8_t  opcode,    uint16_t &currValue, 
                           uint16_t &maxValue, uint32_t devId /* = 0 */)
{
    uint32_t bufSize;
    uint8_t  msg[8];
    uint32_t realId;

    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID.");
        return false;
    }

    // Build the 2-byte query message.
    msg[0] = 1;
    msg[1] = opcode;

    if (sendI2cPayload(msg, 2, realId) == false) {
        return false;
    }

#ifdef _WIN32
    Sleep(40);
#else
    usleep(40000);
#endif

    // Then grab the 8-byte response
    bufSize = 8;
    if (recvI2cPayload(msg, bufSize, false, realId) == false) {
        return false;
    }

    // Check the 1st byte to make sure its a response message
    if (msg[0] != 2) {
        setErrorString(
            "VCP Feature Reply expected but message type differs.");
        return false;
    }
    
    // Check the 2nd byte to make sure the feature is supported
    if (msg[1] != 0) {
        setErrorString("Unsupported VCP feature.");
        return false;
    }

    // Check the 3rd byte to see if the code was echoed back to us
    if (msg[2] != opcode) {
        setErrorString("VCP Feature opcode not echoed properly.");
        return false;
    }

    currValue = (msg[6]<<8) + msg[7];
    maxValue  = (msg[4]<<8) + msg[5];
 
    return true;
}

// ------------------------------------
//
// Send a 2-byte message
//   Byte 0 - 0x09
//        1 - VCP Feature opcode
//
// Then recv an 8-byte response, just like when 
// reading values:
//
//   0 - 0x02
//   1 - Error code. 0 -> success, 
//                   1 -> unsupported value
//   2 - Key of value to read
//   3 - Type code.  0 -> set parameter, 
//                   1 -> momentary (like degauss)
//   4 - Maximum Value, high byte
//   5 - Maximum Value, low byte
//   6 - Current value, high byte
//   7 - Current value, low byte
//
// virtual
bool 
Ookala::Ddc::resetVcpFeature(uint8_t   opcode,   uint16_t &currValue,
                             uint16_t &maxValue, uint32_t  devId /* = 0 */)
{ 
    uint32_t bufSize;
    uint8_t  msg[8];
    uint32_t realId;

    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID.");
        return false;
    }

    msg[0] = 0x09;
    msg[1] = opcode;

    if (sendI2cPayload(msg, 2, realId) == false) {
        return false;
    }

#ifdef _WIN32
    Sleep(40);
#else
    usleep(40000);
#endif

    // Then grab the 8-byte response
    bufSize = 8;
    if (recvI2cPayload(msg, bufSize, false, realId) == false) {
        return false;
    }

    // Check the 1st byte to make sure its a response message
    if (msg[0] != 2) {
        setErrorString("VCP Feature Reply expected but message type differs.");
        return false;
    }
    
    // Check the 2nd byte to make sure the feature is supported
    if (msg[1] != 0) {
        setErrorString("Unsupported VCP feature.");
        return false;
    }

    // Check the 3rd byte to see if the code was echoed back to us
    if (msg[2] != opcode) {
        setErrorString("VCP Feature opcode not echoed properly.");
        return false;
    }

    currValue = (msg[6]<<8) + msg[7];
    maxValue  = (msg[4]<<8) + msg[5];
 
    return true;
}

// ------------------------------------
//
// Send a message to save current settings in memory
//
//   Byte 0 - 0x0c
//
//
// virtual
bool 
Ookala::Ddc::saveSettings(uint32_t devId /* = 0 */)
{
    uint8_t msg[2];
    uint32_t realId;

    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID.");
        return false;
    }

    msg[0] = 0x0c;

    return sendI2cPayload(msg, 1, realId);
}

// ------------------------------------
//
// virtual
bool
Ookala::Ddc::sendI2cPayload(uint8_t *buf,  
                            uint32_t bufLen, 
                            uint32_t devId /* = 0 */)
{
    return false;
}
                                                  
// ------------------------------------
//
// virtual
bool 
Ookala::Ddc::recvI2cPayload(
                    uint8_t *buf, uint32_t &bufLen,  
                    bool     strictSizeChecking /* = true */, 
                    uint32_t devId              /* = 0 */)
{
    return false;
}


// ------------------------------------
//
// virtual
void
Ookala::Ddc::setDebug(bool debug, std::string debugFile /* = "" */)
{
    mDebug     = debug;
    mDebugFile = debugFile;
}

// ------------------------------------
// 
// virtual protected
bool
Ookala::Ddc::addEdid(uint32_t key, const Edid1_3 &edid)
{
    if (!mDdcData) return false;

    mDdcData->edidData[key] = edid;

    return true;
}


// ------------------------------------
// 
// virtual protected
bool
Ookala::Ddc::getEdid(uint32_t key, Edid1_3 &edid)
{
    std::map<uint32_t, Edid1_3>::iterator foundEdid;

    if (!mDdcData) return false;

    foundEdid = mDdcData->edidData.find(key);

    if (foundEdid == mDdcData->edidData.end()) {
        return false;
    }

    edid = (*foundEdid).second;

    return true;
}

// ------------------------------------
// 
// virtual protected
std::vector<uint32_t> 
Ookala::Ddc::getEdidKeys()
{
    std::vector<uint32_t> keys;
    std::map<uint32_t, Edid1_3>::iterator foundEdid;

    if (!mDdcData) return keys;

    for (foundEdid = mDdcData->edidData.begin(); 
                foundEdid != mDdcData->edidData.end();
                                           ++foundEdid) {
        keys.push_back((*foundEdid).first);
    }


    return keys;
}

// ------------------------------------
// 
// virtual protected
bool
Ookala::Ddc::clearEdid(uint32_t key)
{
    std::map<uint32_t, Edid1_3>::iterator foundEdid;

    if (!mDdcData) return false;

    foundEdid = mDdcData->edidData.find(key);

    if (foundEdid == mDdcData->edidData.end()) {
        return false;
    }

    mDdcData->edidData.erase(foundEdid);

    return true;
}


// ------------------------------------
// 
// virtual protected
bool
Ookala::Ddc::validDevId(uint32_t queryDevId, uint32_t &realDevId)
{
    if (!mDdcData) return false;

    if (mDdcData->edidData.empty()) {
        return false;
    }

    if (queryDevId == 0) {
        realDevId = (*(mDdcData->edidData.begin())).first;
        return true;
    }

    realDevId = queryDevId;
    std::map<uint32_t, struct Edid1_3>::iterator theEdid = 
                            mDdcData->edidData.find(queryDevId);
    if (theEdid == mDdcData->edidData.end()) {
        return false;
    }

    return true;
}


// ------------------------------------
//
// virtual
bool
Ookala::Ddc::parseEdid1_3(uint8_t *buf, int bufSize, struct Edid1_3 *edid)
{
    bool debug = false;
    int  i;

    if (debug) {
        for (int i=0; i<16; i++) {
            printf("%x %x %x %x  %x %x %x %x\n",
                buf[i*8],
                buf[i*8+1],
                buf[i*8+2],
                buf[i*8+3],
                buf[i*8+4],
                buf[i*8+5],
                buf[i*8+6],
                buf[i*8+7]);
        }
    }

    if (bufSize < 128) {
        setErrorString("EDID data size too small.");
        return false;
    }

    if (buf[0] != 0) {
        setErrorString("EDID header magic number is wrong.");
        return false;
    }
    for (i=1; i<7; i++) {
        if (buf[i] != 0xff) {
            setErrorString("EDID header magic number is wrong.");
            return false;   
        }
    }
    if (buf[7] != 0) {
        setErrorString("EDID header magic number is wrong.");
        return false;
    }

    // Test version + revision
    if (buf[0x12] != 1) {
        setErrorString("Invalid EDID version.");
        return false;
    }
    if ((buf[0x12] == 1) && (buf[0x13] == 0)) {
        setErrorString("Invalid EDID version.");
        return false;
    }

    // Test year
    if (buf[0x11] <= 3) {
        setErrorString("EDID year predates EDID spec.");
        return false;
    }

    edid->manufacturer    = *(uint16_t *)(buf+0x08);
    edid->productCode     = *(uint16_t *)(buf+0x0a);
    edid->serialNumber    = *(uint32_t *)(buf+0x0c);
    edid->manufactureWeek = buf[0x10];
    edid->manufactureYear = buf[0x11];

    if (edid->manufactureYear > 3) {
        edid->manufactureYear += 1990;
    }

    memset(edid->monitorName, 0, 14);
    memset(edid->identifier,  0, 14);

    memcpy(edid->rawEdidBlock, buf, 128);

    // Check the detailed timing sections for goodies
    for (int off=0x36; off<=0x6c; off+=18) {

        if (debug) {
            printf("%x %x %x %x %x\n",
                buf[off], buf[off+1], buf[off+2], buf[off+3], buf[off+4]);
        }

        // Info is stored in blocks who have the first 5 bytes
        // of the form:  0 0 0 <tag> 0
        if ((buf[off] == 0)   && (buf[off+1] == 0) &&
            (buf[off+2] == 0) && (buf[off+4] == 0)) {
        
            switch (buf[off+3]) {

                // Monitor name
                case 0xfc: 
                    if (debug) printf("Got name tag\n");
                    for (i=0; i<13; i++) {
                        if (buf[off+5+i] == 0x0a) break;

                        edid->monitorName[i] = buf[off+5+i];
                        if (debug) printf("%c", buf[off+5+i]);
                    }
                    if (debug) printf("\n");
                    break;

                // Serial number
                case 0xff: 
                    for (i=0; i<13; i++) {
                        if (buf[off+5+i] == 0x0a) break;

                        edid->identifier[i] = buf[off+5+i];
                        if (debug) printf("%c", buf[off+5+i]);
                    }
                    if (debug) printf("\n");
                    break;

            }
        }
    }

    return true;
}






