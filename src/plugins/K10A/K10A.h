// --------------------------------------------------------------------------
// $Id: K10A.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef K10A_H_HAS_BEEN_INCLUDED
#define K10A_H_HAS_BEEN_INCLUDED

#ifdef EXIMPORT
    #undef EXIMPORT
#endif

#ifdef _WIN32
#ifdef K10A_EXPORTS
#define EXIMPORT _declspec(dllexport)
#else
#define EXIMPORT _declspec(dllimport)
#endif

#else
#define EXIMPORT
#endif

#include "Types.h"
#include <string.h>
#include "Plugin.h"
#include "Dict.h"
#include "Sensor.h"

namespace Ookala {

class K10A: public Sensor
{
    // PUBLIC ---------------------------------------------------------------
    public:
        // Constructors -----------------------------------------------------
        K10A();

        // Copy Constructors ------------------------------------------------
        K10A(const K10A &);

        // Destructor -------------------------------------------------------  
        virtual ~K10A();

        // Operator: Assignment ---------------------------------------------
        K10A & operator=(const K10A &src);

        // ------------------------------------------------------------------
        PLUGIN_ALLOC_FUNCS(K10A)

        // Inquiry ----------------------------------------------------------
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

    // PROTECTED ------------------------------------------------------------
    protected:
        // ------------------------------------------------------------------
        // Info that we'll pull out of the chain dict
        struct DictOptions 
        {
			std::string       port;
            int32_t           calFileID;
            int32_t           measurements;
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
        //	  K10A::port             [STRING]  Serial port name
        //	  K10A::calFileID        [INT]     Which of the Cal files stored on
        //	                                   the device to use, can be 0-96.
        //	  K10A::measurements     [INT]     Number of measurements to take.
        //	                                   The result will be an average of
        //	                                   all measurements taken.
        bool     getDictOptions(DictOptions &opts, PluginChain *chain);

    // PRIVATE --------------------------------------------------------------
    
};

}; // namespace Ookala

#endif



