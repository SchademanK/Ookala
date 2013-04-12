// --------------------------------------------------------------------------
// $Id: SignalHandler.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef SIGNALHANDLER_H_HAS_BEEN_INCLUDED
#define SIGNALHANDLER_H_HAS_BEEN_INCLUDED

//
// A little class for dealing with multi-threaded
// signal handling.
//
// In order to setup masking correctly, this should
// be started up before anything else that might spawn
// a thread
//

#ifdef __linux__

#include <pthread.h>

class wxApp;
class SignalHandler
{
    public:
        SignalHandler(wxApp *app);
        virtual ~SignalHandler();

        // To be called from the main thread to startup the
        // signal handler thread.
        bool mainloop();

        // Also to be called from the main thread, to shutdown
        // the signal handler thread.
        bool shutdown();

    protected:
        wxApp     *mApp;

        pthread_t  mSignalHandlerThread;
        bool       mHasSignalHandlerThread; 

        // The main loop that we spawn off a thread into. Will
        // catch signals and post events back to the wxApp 
        //
        // args should be a pointer to the SignalHandler we care about.
        static void * signalHandlerThreadMain(void *args);

    private:
        SignalHandler() {}    
        
};

#endif


#endif


