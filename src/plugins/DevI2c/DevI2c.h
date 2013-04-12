// --------------------------------------------------------------------------
// $Id: DevI2c.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DEVI2C_H_HAS_BEEN_INCLUDED
#define DEVI2C_H_HAS_BEEN_INCLUDED

#include <map>

#include "Ddc.h"

namespace Ookala {

//
// A little class for making sense of /dev/i2c under linux
//

class DevI2c: public Ddc
{
    public:
        DevI2c();
        DevI2c(const DevI2c &);
        virtual ~DevI2c();
        DevI2c & operator=(const DevI2c &src);

        PLUGIN_ALLOC_FUNCS(DevI2c)

        virtual bool enumerate();

        // Return keys to the devices that we've found. Keys should be
        // persistent across re-enumeration of devices. The device
        // key of 0 is reserved as the 'default' device - that is,
        // we don't much care which device gets hit with the command.
        virtual std::vector<uint32_t> devices();

        // These deal with the 'payload' of a message. They will add (or strip)
        // the header of the message and the checksum. For our write example
        // above, we would pass in the 4-byte payload to sendI2cPayload() to
        // change the brightness:
        //
        //    uint8_t buf[] = {0x3, 0x10, 0, 0x80};
        //
        //    sendI2cPayload(0, buf, 4);
        virtual bool sendI2cPayload(uint8_t *buf, uint32_t  bufLen,
                                                  uint32_t  devId = 0);
        virtual bool recvI2cPayload(uint8_t *buf, uint32_t &bufLen,
                                    bool strictSizeChecking = true,
                                    uint32_t devId = 0);

    protected:
        // File-descriptors - and the device file names that were opened
        // to hold them.
        std::map<uint32_t, int>         mFds;
        std::map<uint32_t, std::string> mFilenames;

        // These deal with full messages - that is, payload, header, and checksum.
        // Translate devId to a fd and send or recv the required data.
        virtual bool sendI2cMsg(uint8_t *buf, uint32_t bufLen, uint32_t devId = 0);
        virtual bool recvI2cMsg(uint8_t *buf, uint32_t bufLen, uint32_t devId = 0);

        // Enumerate displays. Perhaps this should happen on preRun?
        virtual bool  _run(PluginChain *chain);
};

}; // namespace Ookala

#endif


