// --------------------------------------------------------------------------
// $Id: Chroma5.h 135 2008-12-19 00:49:58Z omcf $
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

// This module requires a direct-licensed library interface from X-Rite, Inc.
// API and symbols are copyright (C) 2004 X-Rite, Inc.
//
// It is provided only for X-Rite licensed users.  All other applications 
// are strictly prohibited.
//
// Published with permission of X-Rite, Inc., October 2008

#ifndef CHROMA5_H_HAS_BEEN_INCLUDED
#define CHROMA5_H_HAS_BEEN_INCLUDED

#ifdef EXIMPORT
    #undef EXIMPORT
#endif

#ifdef _WIN32
#ifdef CHROMA5_EXPORTS
#define EXIMPORT _declspec(dllexport)
#else
#define EXIMPORT _declspec(dllimport)
#endif

#else
#define EXIMPORT
#endif

// X-Rite Sequel Imaging SipCal ----------------------------------------------
#include "Sipcal.h"

#include "Types.h"
#include "Plugin.h"
#include "Dict.h"
#include "Sensor.h"

namespace Ookala {

class Chroma5: public Sensor
{
    public:
        Chroma5();
        Chroma5(const Chroma5 &);
        virtual ~Chroma5();
        Chroma5 & operator=(const Chroma5 &src);

        PLUGIN_ALLOC_FUNCS(Chroma5)

        // Returns the number of sensors detected
        virtual std::vector<uint32_t> sensors();

        // Values are returned in units of cd/m^2.
        virtual bool measureYxy(Yxy &value,
                                PluginChain *chain = NULL,
                                uint32_t sensorIdx = 0);

        // Need to signal back to the user that a re-enumeration
        // might be needed, especially if a disconnect occurred.
        virtual std::vector<uint32_t> actionNeeded(
                                PluginChain *chain    = NULL,
                                uint32_t     sensorId = 0);

        // Allow the user to force a re-enumeration of devices.
        virtual bool actionTaken(uint32_t     actionKey, 
                                 PluginChain *chain = NULL,
                                 uint32_t     sensorId = 0);

    protected:

        // Info that we'll pull out of the chain dict
        struct DictOptions
        {
            int32_t           displayType;        // XXX: 
            double            integrationTime;
            int32_t           calibration;        // XXX: 
        };

        // Running count of the next key to map to a device. The ID
        // of '0' is reserved for a default device (the first one that
        // we happen to find - in other words, the user doesn't care).
        uint32_t mNextSensorKey;

        // Is the device open, and if so, what is its key?
        bool     mDeviceOpen;
        uint32_t mDeviceKey;

        // Have we read the screen refresh since we opened the device?
        bool     mReadScreenRefresh;

        // Given a chain, lookup it's dict and pull out things that
        // we care about:
        //
        //      Chroma5::displayType         [STRING]    {lcd, crt}
        //      Chroma5::integrationTime     [DOUBLE]    Number of seconds 
        //                                               to measure
        //      Chroma5::calibrationIdx      [INT]       Override the 
        //                                               calibration matrix
        //                                               choice based on 
        //                                               displayType.
        //                                               Can be 0-3.
        bool     getDictOptions(DictOptions &opts, PluginChain *chain);

    private:
        // Sequel Imaging Library Error Codes
        long     mSIPerror;
};

}; // namespace Ookala

#endif



