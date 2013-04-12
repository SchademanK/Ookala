// --------------------------------------------------------------------------
// $Id: SignalHandler.cpp 135 2008-12-19 00:49:58Z omcf $
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

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>


#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"   
#endif

#include "Events.h"
#include "SignalHandler.h"

// --------------------------------------
//
SignalHandler::SignalHandler(wxApp *app)
{
    mApp                    = app;
    mHasSignalHandlerThread = false;
}

// --------------------------------------
//
SignalHandler::~SignalHandler()
{
}


// --------------------------------------
//
bool
SignalHandler::mainloop()
{
    sigset_t signals;

    sigfillset(&signals);
    pthread_sigmask(SIG_BLOCK, &signals, NULL);

    if (pthread_create(&mSignalHandlerThread, NULL,
                    SignalHandler::signalHandlerThreadMain,
                    (void *)this)) {
        perror("pthread_create(): ");
        return false;
    }
    mHasSignalHandlerThread = true;

    return true;
}

// --------------------------------------
//
bool
SignalHandler::shutdown()
{
printf("SignalHandler::shutdown()\n");

    // This could be hit in one of two ways. Either the signal handling
    // thread has caused the app to exit, or the user has. We can't
    // really tell the difference at this point (well, we could, but
    // it just adds uglyness). So, first, signal the signal thread
    // to quit, and then wait for it to finish.

#ifdef __linux__
    if (mHasSignalHandlerThread) {
printf("SignalHandler::shutdown() sending SIGINT\n");

        pthread_kill(mSignalHandlerThread, SIGINT);

printf("SignalHandler::shutdown() joining\n");

        pthread_join(mSignalHandlerThread, NULL);
    }
#endif

printf("SignalHandler::shutdown() done\n");

    return true;

}

// --------------------------------------
//
// static protected
void * 
SignalHandler::signalHandlerThreadMain(void *arg)
{
    int             signal;
    sigset_t        signals;
    SignalHandler  *handler = (SignalHandler *)arg;

    printf("SignalHandler::signalHandlerThreadMain()\n");

    while (1) {
        signal = 0;

        if (sigfillset(&signals)) {
            fprintf(stderr, "Error setting signal set\n");
            pthread_exit(NULL);
        }

        if (sigwait(&signals, &signal)) {
            fprintf(stderr, "Error on sigwait()\n");
            continue;
        }
        
        printf("SignalHandler::signalHandlerThreadMain() got signal %d\n", signal);

        switch (signal) {            
            case SIGINT:
            case SIGQUIT:
            case SIGILL:
            case SIGABRT:
            case SIGBUS:
            case SIGKILL:
                {
                    // Signal back to the app to quit
                    wxCommandEvent signalEvent(signalProcessedEvent, wxID_ANY);
                    wxPostEvent(handler->mApp, signalEvent);
                }

                // And we're done.
                printf("Signal thread exiting\n");
                pthread_exit(NULL);
                break;
            
            default:
                break;
        }
    }

    printf("Signal thread exiting\n");
    return NULL;
}



#endif /* #ifdef __linux */
