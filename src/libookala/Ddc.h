// --------------------------------------------------------------------------
// $Id: Ddc.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DDC_H_HAS_BEEN_INCLUDED
#define DDC_H_HAS_BEEN_INCLUDED

#include <map>

//
// Base class for ddc/ci access
//

//
// Derived classes should read the EDID and count up the
// number of valid devices they see in _postLoad().
//
// Formatting of a basic access.bus commands:
//
// A simple 'VCP feature write' instruction has an opcode of 0x3.
// Also needed is the opcode of the VCP feature (e.g. brightness
// at 0x10) and a value (2 bytes, but generally 0-255). 
//
// The write instruction is then:
//    byte 0: 0x03   (write)
//         1: 0x10   (brightness)
//         2: 0x00   (high byte of value)
//         3: 0x80   (low byte of value, 128)
//
// However, to send this message over the wire, it needs a little 
// more. Basically, it needs a header at the front and a checksum
// at the tail. The entire message would consist of:
//
//    byte 0: 0xXX   (monitor address)
//         1: 0xXX   (host address)
//         2: 0x84   (message length, 0x80 | 4
//         3: 0x03   (write)
//         4: 0x10   (brightness)
//         5: 0x00   (high byte of value)
//         6: 0x80   (low byte of value, 128)
//         7: 0xXX   (checksum)
//    
// Then, fire off this 8-byte message to the monitor to change things.
//

// Reset a given VCP Feature to its factory default.
// virtual bool resetVcpFeature(uint32_t devId, uint8_t opcode);

// Save current settings on the display.
// virtual bool saveSettings(uint32_t devId);


// XXX: Once properly motivated, we should handle caps-strings as well.
//      See ACCESS.bus section 2 for details.


#include <string>
#include <vector>

#include "Types.h"
#include "Plugin.h"

namespace Ookala {

struct EXIMPORT Edid1_3 {
    uint16_t manufacturer;      // offset 0x08
    uint16_t productCode;       // offset 0x0a
    uint32_t serialNumber;      // offset 0x0c
    uint8_t  manufactureWeek;   // offset 0x10
    uint16_t manufactureYear;   // offset 0x11, biased by 1990

    char     monitorName[14];   // Detailed timing block with 
                                // tag: 0 0 0 0xfc 0.

    char     identifier[14];    // Detailed timing block with
                                // tag: 0 0 0 0xff 0. Usually
                                // is the serial number.

    uint8_t  rawEdidBlock[128]; // The first EDID block, undecoded.
};

class EXIMPORT Ddc: public Plugin
{
    public:
        Ddc();
        Ddc(const Ddc &src);
        virtual ~Ddc();
        Ddc & operator=(const Ddc &src);

        // Return keys to the devices that we've found. Keys should be
        // persistant across re-enumeration of devices. The device
        // key of 0 is reserved as the 'default' device - that is,
        // we don't much care which device gets hit with the command.
        // 
        // This is done for consistancy with the sensor interface.
        virtual std::vector<uint32_t> devices();

        // Force a scan for devices. Any devices that we already 
        // know about should _not_ change device Id. Otherwise, we
        // should prune out all non-responding devices. For devices
        // that exist, we should store their EDIDs - which is probably
        // the way to know if they're still around.
        //
        // (This might be too hard to enforce in reality, we might
        // want to consider relaxing this..).
        virtual bool enumerate();

        // NOTE: the base class implementation doesn't actually query
        // the EDID from the device, it uses the cached version in the
        // map from dev id -> edid block below. Derived classes should
        // read EDIDs at enumerate() time.
        virtual bool getEdid13(struct Edid1_3 &edid, uint32_t devId = 0);

        // For manipulating VCP Features, e.g. brighteness or contrast. For
        // a list of potential opcodes, see XXX.
        virtual bool setVcpFeature(uint8_t opcode, uint16_t value, 
                                                   uint32_t devId = 0);
        virtual bool getVcpFeature(uint8_t   opcode,   uint16_t &currValue,
                                   uint16_t &maxValue, uint32_t  devId = 0);

        virtual bool resetVcpFeature(uint8_t   opcode,   uint16_t &currValue,
                                     uint16_t &maxValue, uint32_t  devId = 0);

        virtual bool saveSettings(uint32_t devId = 0);

        // These deal with the 'payload' of a message. They will add (or strip)
        // the header of the message and the checksum. For our write example
        // above, we would pass in the 4-byte payload to sendI2cPayload() to
        // change the brightness:
        //
        //    uint8_t buf[] = {0x3, 0x10, 0, 0x80};
        //
        //    sendI2cPayload(0, buf, 4);
        virtual bool sendI2cPayload(uint8_t *buf, uint32_t bufLen,
                                                  uint32_t devId = 0);
        virtual bool recvI2cPayload(uint8_t *buf, uint32_t &bufLen,  
                                    bool     strictSizeChecking = true, 
                                    uint32_t devId = 0 );

        virtual void setDebug(bool debug, std::string debugFile = "");

    protected:

        bool        mDebug;
        std::string mDebugFile;

        // Running count of the next key to map to a device. The ID
        // of '0' is reserved for a default device (the first one that
        // we happen to find - in other words, the user doesn't care).
        uint32_t    mNextKey;

        // Associate an EDID with a given key.
        virtual bool    addEdid(uint32_t key, const Edid1_3 &edid);

        // Return the EDID associated with a given key
        virtual bool    getEdid(uint32_t key, Edid1_3 &edid);

        // Return all the EDID keys that we know about
        virtual std::vector<uint32_t> getEdidKeys();

        // Remove an association of an EDID and a key.
        virtual bool    clearEdid(uint32_t key); 

        // Test a device ID to see if it's one that we recognize. 
        // If so, return true. On success, realDevId will hold
        // the device id that we should use - this will get 
        // around aliasing id 0 to any device.
        virtual bool validDevId(uint32_t queryDevId, uint32_t &realDevId);

        // Parse a chunk of data into an edid 1.3 structure.
        // Returns false if the edid does not appear to be set
        // properly.
        virtual bool parseEdid1_3(uint8_t *buf, int bufSize, struct Edid1_3 *edid);

    private:
        
        struct _Ddc {
            // Hold edid data gathered from each device. Map is from
            // device ID -> edid block.
            std::map<uint32_t, struct Edid1_3> edidData;
        };

        _Ddc   *mDdcData;
};

}; // namespace Ookala

#endif

