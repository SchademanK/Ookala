// --------------------------------------------------------------------------
// $Id: SessionManage.cpp 135 2008-12-19 00:49:58Z omcf $
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
#include <pwd.h>

#include <vector>
#include <string>

#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
    #include <wx/cmdline.h>
#endif

#include "SessionManage.h"
#include "Events.h"

// =========================================
//
//  SessionManage
//
// -----------------------------------------

SessionManage::SessionManage(wxApp *app)
{
    mApp                  = app;
    mStartedWorkerThread  = false;
    mShutdownWorkerThread = false;
    mThreadMainCancel     = false;
    mHasSessionId         = false;

    pthread_mutex_init(&mThreadMainCancelMutex, NULL);
}

// -----------------------------------------
//
// This is assuming we're destructing from the main-thread
//
// virtual
SessionManage::~SessionManage()
{
    // If we havn't killed the thread already,
    // we should do that..
    if ((mStartedWorkerThread) && (!mShutdownWorkerThread)) {
        shutdown();
    }
}

// -----------------------------------------
// 
// XSMP claims that SESSION_MANAGER should be holding the
// connection string to the session manager. If we can't
// at least find that, there's no way we can go on.
//
bool
SessionManage::hasSms()
{
    if (!getenv("SESSION_MANAGER")) return false;

    return true;
}


// -----------------------------------------
//
void
SessionManage::mainloop(const char *sessionId)
{
    int errVal;

    if (!hasSms()) return;

    if (sessionId) {
        mHasSessionId = true;
        mSessionId    = sessionId;
    }

    errVal = pthread_create(&mWorkerThread, NULL, smcThreadMain, (void *)this);
    if (errVal) {
        perror("pthread_create: ");
        return;
    }
    
    mStartedWorkerThread = true;
}

// -----------------------------------------
//
// This should only be called from the main thread.
//
void
SessionManage::shutdown()
{
printf("SessionManage::shutdown()\n");

    if (!mStartedWorkerThread) return;
    if (mShutdownWorkerThread) return;

    if (pthread_equal(pthread_self(), mWorkerThread)) {
        fprintf(stderr, 
            "ERROR: Calling SessionManage::shutdown() from worker thread.\n");
        return;
    }

    pthread_mutex_lock(&mThreadMainCancelMutex);
    mThreadMainCancel = true;
    pthread_mutex_unlock(&mThreadMainCancelMutex);

    pthread_join(mWorkerThread, NULL);

    mShutdownWorkerThread = true;
}

// -----------------------------------------
//
// protected
bool
SessionManage::smcSetupProperties()
{
    SmProp *props[7];

    // SmCloneCommand
    props[0]           = new SmProp;
    props[0]->name     = (char *) SmCloneCommand;
    props[0]->type     = (char *) SmLISTofARRAY8;
    props[0]->num_vals = 1;
    props[0]->vals     = new SmPropValue[1];
    for (int i=0; i<1; ++i) {
        props[0]->vals[i].length = strlen((const char*)(mApp->argv[i])) + 1;
        props[0]->vals[i].value  = strdup((const char*)(mApp->argv[i]));
    }

    // SmRestartCommand     
    props[1]           = new SmProp;
    props[1]->name     = (char *) SmRestartCommand;
    props[1]->type     = (char *) SmLISTofARRAY8;
    props[1]->num_vals = 2;
    props[1]->vals     = new SmPropValue[2];
    for (int i=0; i<1; ++i) {
        props[1]->vals[i].length = strlen((const char*)(mApp->argv[i])) + 1;
        props[1]->vals[i].value  = strdup((const char*)(mApp->argv[i]));
    }
    char buf[4096];
    sprintf(buf, "--sm-client-id=%s", mSessionId.c_str());
    props[1]->vals[1].length   = strlen(buf) + 1;
    props[1]->vals[1].value    = strdup(buf);


    // SmProgram
    props[2]                 = new SmProp;
    props[2]->name           = (char *) SmProgram;
    props[2]->type           = (char *) SmARRAY8;
    props[2]->num_vals       = 1;
    props[2]->vals           = new SmPropValue[1];
    props[2]->vals[0].length = strlen((const char*)(mApp->argv[0])) + 1;
    props[2]->vals[0].value  = strdup((const char*)(mApp->argv[0]));

    // SmUserID
    struct passwd *pwd = getpwuid(getuid());

    props[3]           = new SmProp;
    props[3]->name     = (char *) SmUserID;
    props[3]->type     = (char *) SmARRAY8;
    props[3]->num_vals = 1;
    props[3]->vals     = new SmPropValue[1];
    props[3]->vals[0].value = strdup(pwd->pw_name);
    props[3]->vals[0].length = strlen(pwd->pw_name) + 1;
   

    // SmRestartStyleHint    
    props[4]                   = new SmProp;
    props[4]->name             = (char *) SmRestartStyleHint;
    props[4]->type             = (char *) SmCARD8;
    props[4]->num_vals         = 1;
    props[4]->vals             = new SmPropValue[1];
    props[4]->vals[0].length   = 1;
    props[4]->vals[0].value    = malloc(1);
    ((char *)props[4]->vals[0].value)[0] = SmRestartIfRunning;       



    // SmCurrentDirectory
    char dir[4096];
    if(getcwd(dir, 4095) == 0) {
        return false;
    }

    props[5]                 = new SmProp;
    props[5]->name           = (char *) SmCurrentDirectory;
    props[5]->type           = (char *) SmARRAY8;
    props[5]->num_vals       = 1;
    props[5]->vals           = new SmPropValue[1];
    props[5]->vals[0].value  = strdup(dir);
    props[5]->vals[0].length = strlen(dir) + 1;

    // SmEnvironment

    int                      envIdx = 0;
    std::vector<std::string> env;

    while (environ[envIdx]) {
        std::string varAndValue(environ[envIdx]);

        std::string::size_type breakPnt = varAndValue.find('=');

        if (breakPnt != std::string::npos) {
            env.push_back(varAndValue.substr(0, breakPnt));
            env.push_back(varAndValue.substr(breakPnt+1));

        } else {
            // No =, just push the whole thing plus an empty value
            env.push_back(varAndValue);
            env.push_back(std::string(""));
        }
        envIdx++;
    }

    props[6] = new SmProp;
    props[6]->name = (char *) SmEnvironment;
    props[6]->type = (char *) SmLISTofARRAY8;
    props[6]->num_vals = env.size();
    props[6]->vals     = new SmPropValue[env.size()];
    for (uint32_t i=0; i<env.size(); ++i) {
        props[6]->vals[i].length = env[i].size() + 1;
        props[6]->vals[i].value  = strdup(env[i].c_str());
    }

    // Set values
    SmcSetProperties(mSmcConn, 7, props);

    // free
    for (int i=0; i<7; ++i) {
        for (int j=0; j<props[i]->num_vals; ++j) {
            free(props[i]->vals[j].value); 
        }
        delete[] props[i]->vals;
        delete props[i];

        props[i] = NULL;
    }

    return true;    
}

// -----------------------------------------
//
// static protected
void *
SessionManage::smcThreadMain(void *arg)
{
    int       iceFd;
    char     *newId, *prevId;
    char      errorBuffer[1024];
    bool      cancelled = false;
    unsigned  eventMask = 
                SmcSaveYourselfProcMask |
                SmcDieProcMask |
                SmcSaveCompleteProcMask |
                SmcShutdownCancelledProcMask;

    SessionManage *us = (SessionManage *)arg;

    fprintf(stderr, "\t!!! smcThreadMain()\n");

    us->mSmcCallbacks.die.callback                   = smcDieCallback;
    us->mSmcCallbacks.die.client_data                = arg; 
    us->mSmcCallbacks.save_yourself.callback         = smcSaveCallback;
    us->mSmcCallbacks.save_yourself.client_data      = arg;
    us->mSmcCallbacks.save_complete.callback         = smcSaveCompleteCallback;
    us->mSmcCallbacks.save_complete.client_data      = arg;
    us->mSmcCallbacks.shutdown_cancelled.callback    = smcCancelledCallback;
    us->mSmcCallbacks.shutdown_cancelled.client_data = arg;

    prevId = NULL;
    if (us->mHasSessionId) {
        prevId = strdup(us->mSessionId.c_str());
        fprintf(stderr, "!!!!\tprevId: %s\n", prevId);
    }

    us->mSmcConn = SmcOpenConnection(
                    NULL, 
                    NULL,
                    SmProtoMajor, 
                    SmProtoMinor,
                    eventMask,
                    &us->mSmcCallbacks,
                    prevId,
                    &newId,
                    1024,
                    errorBuffer);


    if (prevId) {
        free(prevId);
        prevId = NULL;
    }

    fprintf(stderr, "!!!!\tnewId: %s\n", newId);
    us->mSessionId = newId;

    if (!us->mSmcConn) {
        fprintf(stderr, "Unable to connect to SM: %s\n", errorBuffer);
        pthread_exit(NULL);
        return NULL;
    }

    if (!us->smcSetupProperties()) {
        SmcCloseConnection(us->mSmcConn, 0, NULL);
        pthread_exit(NULL);
        return NULL;
    }

    iceFd = IceConnectionNumber(SmcGetIceConnection(us->mSmcConn));

    while (!cancelled) { 
        fd_set         fds;
        struct timeval timeout;
        
        timeout.tv_sec  = 0;
        timeout.tv_usec = 100000;

        FD_ZERO(&fds);
        FD_SET(iceFd, &fds);
        
        if (!select(iceFd+1, &fds, NULL, NULL, &timeout) < 0) {
            fprintf(stderr, "Error in SM main loop select()\n");
            break;
        }

        if (FD_ISSET(iceFd, &fds)) {
            if (IceProcessMessages(SmcGetIceConnection(us->mSmcConn), NULL, NULL) ==
                    IceProcessMessagesIOError) {

                fprintf(stderr, "Error processing ICE message in SM main loop\n");
                break;
            }
        }

        pthread_mutex_lock(&us->mThreadMainCancelMutex);
        cancelled = us->mThreadMainCancel;
        pthread_mutex_unlock(&us->mThreadMainCancelMutex);
    }

printf("Smc main loop stopped\n");

    // Close connection + teardown
    SmcCloseConnection(us->mSmcConn, 0, NULL);

    pthread_exit(NULL);

    return NULL;
}

// -----------------------------------------
//
// static protected
void
SessionManage::smcSaveCallback(SmcConn connection,
                               SmPointer clientData,
                               int       saveType,
                               Bool      shutdown,
                               int       interactStyle,
                               Bool      fast)
{
    //SessionManage * us = (SessionManage *)clientData;

    printf("smcSaveCallback()\n");
    printf("\tsaveType:      %d\n", saveType);
    printf("\tshutDown:      %d\n", shutdown);
    printf("\tinteractStyle: %d\n", interactStyle);
    printf("\tfast:          %d\n", fast);

    // XXX: Here, we should save parameters for restarting
    //us->smcSetupProperties();

    printf("Set properties\n");

    SmcSaveYourselfDone(connection, True);

    printf("Save Done\n");
}

// -----------------------------------------
//
// static protected
void
SessionManage::smcSaveCompleteCallback(SmcConn connection, SmPointer clientData)
{
    printf("smcSaveCompleteCallback()\n");
}


// -----------------------------------------
//
// static protected
void
SessionManage::smcCancelledCallback(SmcConn connection, SmPointer clientData)
{
    printf("smcCancelledCallback()\n");
}


// -----------------------------------------
//
// static protected
void
SessionManage::smcDieCallback(SmcConn connection, SmPointer clientData)
{
    SessionManage *us = (SessionManage *)clientData;

    printf("smcDieCallback()\n");

    // Post a message back to the main loop that the session is going down
    // quickly!
    wxCommandEvent signalEvent(sessionKilledEvent, wxID_ANY);
    wxPostEvent(us->mApp, signalEvent);

}

#endif
