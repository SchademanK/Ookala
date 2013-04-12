// --------------------------------------------------------------------------
// $Id: WxDreamColorCalib.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <stdio.h>
#include <stdlib.h>
#include <string>

#ifdef __linux__
#include <unistd.h>
#endif

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"    
#endif

#include "wx/thread.h"
#include "wx/evtloop.h"

#include "PluginChain.h"
#include "PluginRegistry.h"
#include "WxDreamColorCalib.h"


// ----------------------------------

BEGIN_PLUGIN_REGISTER(1)
    PLUGIN_REGISTER(0, Ookala::WxDreamColorCalib)
END_PLUGIN_REGISTER

// ----------------------------------

namespace Ookala {

// A little class for passing back and forth return
// values from the worker thread to the main thread.
class WxDreamColorCalibReturn: public wxTimer
{
    public:
        WxDreamColorCalibReturn();
        ~WxDreamColorCalibReturn() {}

        // Has the return value been set?
        bool         hasFinished();

        bool         finish(const bool         retVal, 
                           const std::string &errorString);

        bool         getReturnValue();
        std::string  getErrorString();

        // Notify should fire from the main thread
        virtual void Notify();

    protected:
        wxMutex     mLock;
        bool        mReturnValue;
        bool        mHasReturnValue;
        std::string mErrorString;
};

// ----------------------------------

class WxDreamColorCalibRun: public wxThread
{
    public:
        WxDreamColorCalibRun(PluginRegistry          *registry, 
                             PluginChain             *chain,
                             WxDreamColorCalibReturn *ret);
        ~WxDreamColorCalibRun() {};

        virtual void * Entry();        

    protected:
        PluginRegistry          *mRegistry;
        PluginChain             *mChain;
        WxDreamColorCalibReturn *mReturn;
};

}; // namespace Ookala


// ==================================
//
// WxDreamColorCalibReturn
//
// ----------------------------------

Ookala::WxDreamColorCalibReturn::WxDreamColorCalibReturn():
    wxTimer(),
    mLock(wxMUTEX_DEFAULT)
{
    mReturnValue    = false;
    mHasReturnValue = false;
}

// ----------------------------------
//
bool
Ookala::WxDreamColorCalibReturn::hasFinished()
{
    bool retval = false;

    mLock.Lock();
    retval = mHasReturnValue;
    mLock.Unlock();

    return retval;
}


// ----------------------------------
//
bool
Ookala::WxDreamColorCalibReturn::finish(
                                const bool         retVal, 
                                const std::string &errorString)
{
    mLock.Lock();

    mReturnValue    = retVal;
    mErrorString    = errorString;
    mHasReturnValue = true;

    mLock.Unlock();

    return true;
}


// ----------------------------------
//
bool
Ookala::WxDreamColorCalibReturn::getReturnValue()
{
    bool retval = false;

    mLock.Lock();
    retval = mReturnValue;
    mLock.Unlock();

    return retval;
}


// ----------------------------------
//
std::string
Ookala::WxDreamColorCalibReturn::getErrorString()
{
    std::string errStr;

    mLock.Lock();

    errStr = mErrorString;

    mLock.Unlock();

    return errStr;
}

// ----------------------------------
//
// virtual
void  
Ookala::WxDreamColorCalibReturn::Notify()
{
    Stop();

    if (hasFinished()) {
        wxEventLoop::GetActive()->Exit(0);
    } else {

        // If we can't restart the timer, fall-back to just
        // spinning on the finished flag; 
        if (!Start(-1, wxTIMER_ONE_SHOT)) {
            while (!hasFinished()) {
#ifdef _WIN32
                Sleep(1000);
#else
                sleep(1);
#endif
            }
            wxEventLoop::GetActive()->Exit(0);
        }
    }
}




// ==================================
//
// WxDreamColorCalibRun
//
// ----------------------------------

Ookala::WxDreamColorCalibRun::WxDreamColorCalibRun(
                            PluginRegistry          *registry,
                            PluginChain             *chain,
                            WxDreamColorCalibReturn *ret):
    wxThread()
{
    mRegistry = registry;
    mChain    = chain;
    mReturn   = ret;
}

// ----------------------------------
//
// virtual
void *
Ookala::WxDreamColorCalibRun::Entry()
{
    if (!mRegistry) {
        mReturn->finish(false, "No registry found!");
        return NULL;
    }

    std::vector<Plugin *> plugins = mRegistry->queryByName("DreamColorCalib");
    if (plugins.empty()) {
        mReturn->finish(false, "DreamColorCalib not loaded.");
        return NULL;
    }

    printf("Running %s\n", plugins[0]->name().c_str());
    bool ret = (plugins[0])->run(mChain);

    mReturn->finish(ret, (plugins[0])->errorString());

    return NULL;
}


// ==================================
//
// WxDreamColorCalib
//
// ----------------------------------

Ookala::WxDreamColorCalib::WxDreamColorCalib():
    Plugin()
{
    setName("WxDreamColorCalib");
}

// ----------------------------------
//
Ookala::WxDreamColorCalib::WxDreamColorCalib(const WxDreamColorCalib &src):
    Plugin(src)
{
}

// ----------------------------------
//
// virtual
Ookala::WxDreamColorCalib::~WxDreamColorCalib()
{

}

// ----------------------------
//
Ookala::WxDreamColorCalib &
Ookala::WxDreamColorCalib::operator=(const WxDreamColorCalib &src)
{
    if (this != &src) {
        Plugin::operator=(src);
    }

    return *this;
}

// ----------------------------------
//
// virtual protected
bool
Ookala::WxDreamColorCalib::_checkDeps()
{
    if (!mRegistry) {
        setErrorString("No PluginRegistry for WxDreamColorCalib.");
        return false;
    }

    std::vector<Plugin *> plugins = mRegistry->queryByName("DreamColorCalib");
    if (plugins.empty()) {
        setErrorString("No DreamColorCalib plugin found for WxDreamColorCalib.");
        return false;
    }

    return true;
}


// ----------------------------------
//
// virtual protected
bool
Ookala::WxDreamColorCalib::_run(PluginChain *chain)
{
    WxDreamColorCalibReturn retVal;
    WxDreamColorCalibRun   *runner = 
                    new WxDreamColorCalibRun(mRegistry, chain, &retVal);

    // Start the timer 
    if (!retVal.Start(1000, wxTIMER_ONE_SHOT)) {
        setErrorString("Unable to start watcher timer.");
        return false;
    }

    // Start the thread
    runner->Create();
    runner->Run();

    // Start the main-loop - this is just like displaying
    // modal dialogs.
    printf("Going into event loop...\n");
    wxEventLoop().Run();
    printf("Back from event loop...\n");

    retVal.Stop();

    // Retrieve the return values.
    printf("Got ret val: %d %s\n", 
            retVal.getReturnValue(), retVal.getErrorString().c_str());

    setErrorString(retVal.getErrorString());
    return retVal.getReturnValue(); 

}

