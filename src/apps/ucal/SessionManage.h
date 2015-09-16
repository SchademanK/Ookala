// --------------------------------------------------------------------------
// $Id: SessionManage.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef SESSIONMANAGE_H_HAS_BEEN_INCLUDED
#define SESSIONMANAGE_H_HAS_BEEN_INCLUDED

#ifdef __linux__


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>

#include <X11/SM/SMlib.h>
#include <X11/ICE/ICElib.h>

// Trying to encapsulate all the XSMP bits we'll need to be
// restored when a user logs back in.

class wxApp;
class SessionManage
{
    public:
        SessionManage(wxApp *app);
        virtual ~SessionManage();

        // Check if we can even find a session manager.
        bool hasSms();

        // Enter here after checking for an existing session
        // manager. A new worker thread will be spawned, which
        // will setup and select() for events.
        //
        // SessionId should be the id from a previous session. 
        // If we don't have one of those, just pass in NULL.
        void mainloop(const char *sessionId);

        // If we need to stop things, call this. Ok to call 
        // from the main thread (actually, you should probably
        // _only_ call this from the main thread).
        void shutdown();

    protected:
        // Hold a pointer back to the app, so we can get things
        // like argv/argc, as well as posting events into the
        // main loop.
        wxApp          *mApp;

        // Our worker thread, and a flag set by the main thread that
        // we've previously called pthread_create() or shutdown().
        pthread_t       mWorkerThread;
        bool            mStartedWorkerThread;
        bool            mShutdownWorkerThread;

        // Should the worker thread break and exit.
        bool            mThreadMainCancel;

        // mutex around mThreadMainCancel
        pthread_mutex_t mThreadMainCancelMutex;

        // ext/SM related data
        SmcConn         mSmcConn;    
        SmcCallbacks    mSmcCallbacks;

        std::string     mSessionId;
        bool            mHasSessionId;
        // SM setup
        bool smcSetupProperties();

        // The worker thread. 
        static void * smcThreadMain(void *arg);
        
        // SM callbacks
        static void smcSaveCallback(SmcConn connection,
                                    SmPointer clientData,
                                    int       saveType,
                                    Bool      shutdown,
                                    int       interactStyle,
                                    Bool      fast);

        static void smcSaveCompleteCallback(SmcConn connection,
                                                          SmPointer clientData);
        static void smcCancelledCallback(SmcConn connection,
                                                          SmPointer clientData);
        static void smcDieCallback(SmcConn connection,    SmPointer clientData);

    private:
        SessionManage() {}
};

#endif
#endif

