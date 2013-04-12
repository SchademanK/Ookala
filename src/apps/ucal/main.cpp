// --------------------------------------------------------------------------
// $Id: main.cpp 135 2008-12-19 00:49:58Z omcf $
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


#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include <vector>
#include <algorithm>

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"   
    #include "wx/taskbar.h"   
#endif

#include "wxUtils.h"

#include "Dict.h"
#include "DictHash.h"
#include "DataSavior.h"
#include "PluginChain.h"
#include "PluginRegistry.h"


#include "wx/taskbar.h"
#include "wx/evtloop.h"
#include <wx/cmdline.h>

#include "WxIconCtrl.h"
#include "CalibChecker.h"


#ifdef __linux__
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "SessionManage.h"
#include "SignalHandler.h"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN     
#include <windows.h>
#endif

#include "default_icon.xpm"

// ---------------------------------------

enum {
    ID_QUIT                = wxID_HIGHEST+1,
    ID_Taskbar_Quit,        
    ID_Taskbar_Run,        

    // Put these last..
    ID_Taskbar_Run_Chain_0 = wxID_HIGHEST+1000,
};


// ---------------------------------------

class UcalTaskbar: public wxTaskBarIcon
{
    public:
        UcalTaskbar(wxApp                              *parent,        
                    Ookala::PluginRegistry             *reg,
                    std::vector<Ookala::PluginChain *> &chains);


        void setChains(std::vector<Ookala::PluginChain *> &chains);

        virtual wxMenu* CreatePopupMenu();

        void onQuitButton(wxCommandEvent &event);
        void onRun(wxCommandEvent &event);
        void onTimer(wxTimerEvent& event);

        void runPeriodicChains();

        void cancelRunningChainAndQuit();
        
    protected:
        wxApp                              *mParent;
        Ookala::PluginRegistry             *mReg;
        std::vector<Ookala::PluginChain *>  mChains;

        // Keep track of which chain is currently running, in case
        // we need to cancel it from a signal handler.
        Ookala::PluginChain                *mRunningChain;
        Ookala::Mutex                       mRunningChainMutex;

        // Quit after we run the current or chain
        bool                        mQuitAfterRunning;

        // Don't allow menu creating while we're running something
        // already; Having multiple things in flight is bad.
        bool                        mAllowMenu;

        wxTimer                     mTimer;
        DECLARE_EVENT_TABLE()
};

// ---------------------------------------

// Custom event for sending a 'stop because of a signal' action
// from the signal processing thread back to the main thread

wxEventType signalProcessedEvent = wxNewEventType();
wxEventType sessionKilledEvent   = wxNewEventType();


// ---------------------------------------

class UcalApp: public wxApp
{
    public:
        UcalApp();               

        virtual bool OnInit();  
        virtual int  OnExit();

        void         onSignalProcessed(wxEvent &event);
        void         onSessionKilled(wxEvent &event);

    protected:
        Ookala::PluginRegistry              mReg;
        std::vector<Ookala::PluginChain *>  mChains;

        UcalTaskbar                        *mTaskbar;        
        
        bool                        mListChains;
        bool                        mManualChainRun;
        std::string                 mManualChainName;
        Ookala::PluginChain        *mManualChain;

#ifdef __linux__
        SignalHandler               mSignalHandler;

        // Under linux, we need to keep tabs on the session manager in
        // order to restart if we're cancelled.
        SessionManage               mSession;

        // The session id, passed in as a command line option.
        // Should be NULL if we're just starting up.
        char                       *mSessionId;                 
#endif
       
        bool     parseArgs();

        // Locate where our config happens to be hiding, if we can find anything.       
        bool     findConfigFileXdg(std::string &configFile);
        bool     findConfigFile(std::string &configFile);

        void     handleDictNode(xmlDocPtr doc, xmlNodePtr root, 
                        Ookala::DictHash *dhash, std::string &dictName);

        Ookala::Plugin * handlePluginNode(xmlDocPtr doc, xmlNodePtr root, 
                        Ookala::PluginRegistry &reg);

        bool     handleLoadPluginNode(xmlDocPtr doc, xmlNodePtr root,
                        Ookala::PluginRegistry &reg);

        void     handleXmlNode(xmlDocPtr                           doc,    
                               xmlNodePtr                          root,      
                               Ookala::DictHash                   *dhash,                       
                               Ookala::PluginRegistry             &reg,  
                               std::vector<Ookala::PluginChain *> &chains);

        bool     loadConfig(std::string filename, Ookala::PluginRegistry &reg,
                      std::vector<Ookala::PluginChain *> &chains);

        DECLARE_EVENT_TABLE()
};

// =======================================
//
// UcalTaskbar 
//
// ---------------------------------------

UcalTaskbar::UcalTaskbar(wxApp                              *parent,
                         Ookala::PluginRegistry             *reg,
                         std::vector<Ookala::PluginChain *> &chains):
    wxTaskBarIcon(),
    mTimer(this)
{
    mParent           = parent;
    mReg              = reg;
    mChains           = chains;
    mAllowMenu        = true;
    mRunningChain     = NULL;
    mQuitAfterRunning = false;

    if (wxTaskBarIcon::IsOk()) {
        wxIcon icon(default_icon_xpm);

        // Try installing the icon multiple times, becase sometimes 
        // it won't catch on the first time?
        for (int i=0; i<5; ++i) {
            SetIcon(icon, wxT("uCal"));
        }
    }

    mTimer.Start(1000, wxTIMER_ONE_SHOT);
}


// --------------------------------------
//
void 
UcalTaskbar::setChains(std::vector<Ookala::PluginChain *> &chains)
{
    mChains = chains;
}

// --------------------------------------
//
// virtual
wxMenu *
UcalTaskbar::CreatePopupMenu()
{
    if (mAllowMenu == false) {
        return NULL;
    }

    printf("Create Popup Menu!\n");

    wxMenu *menu = new wxMenu;

    for (uint32_t i=0; i<mChains.size(); ++i) {

        if (!mChains[i]->getHidden()) {
            menu->Append(ID_Taskbar_Run_Chain_0 + i, _S(mChains[i]->name()));

            Connect(ID_Taskbar_Run_Chain_0 + i,
                    wxEVT_COMMAND_MENU_SELECTED, 
                    wxCommandEventHandler(UcalTaskbar::onRun));
        }
    }

    menu->AppendSeparator();

    menu->Append(ID_Taskbar_Quit, _T("&Quit"));    

    return menu;
}

// --------------------------------------
//
void 
UcalTaskbar::onQuitButton(wxCommandEvent &event)
{
    if (mParent) {
        mParent->ExitMainLoop();
    }
}

// --------------------------------------
//
void 
UcalTaskbar::onRun(wxCommandEvent &event)
{
    int32_t chainIdx = event.GetId() - ID_Taskbar_Run_Chain_0;
    bool    ret, needsQuit;

    printf("onTestCalibrate(), run chain %d\n", chainIdx);
            
    if (chainIdx < 0) return;
    if (chainIdx >= (int)mChains.size()) return;
    
    mAllowMenu = false;

    mTimer.Stop();

    mRunningChainMutex.lock();
    mRunningChain = mChains[chainIdx];
    mRunningChainMutex.unlock();

    ret = mChains[chainIdx]->run();

    mRunningChainMutex.lock();
    mRunningChain = NULL;
    needsQuit = mQuitAfterRunning;
    mRunningChainMutex.unlock();

    if (needsQuit) {
        if (mParent) {
            mParent->ExitMainLoop();
        }
        return;
    }

    mTimer.Start(-1, wxTIMER_ONE_SHOT);

    if (ret == false) {
        if (mChains[chainIdx]->wasCancelled() == false) {

            wxMessageBox(_S("Execution failed: " + 
                         mChains[chainIdx]->errorString()),
                         _("uCal Error"),
                         wxOK | wxICON_ERROR);

            printf("Chain run FAILED!!!\n");
            printf("\t%s\n", mChains[chainIdx]->errorString().c_str());
        } else {
            printf("Chain run Cancelled\n");
        } 
    } else {
        printf("Chain run success!!\n");
    }

    mAllowMenu = true;

}

// --------------------------------------
//
void 
UcalTaskbar::onTimer(wxTimerEvent& event)
{
    std::vector<int32_t> periods;

    mTimer.Stop();

    runPeriodicChains();

    for (std::vector<Ookala::PluginChain *>::iterator theChain = mChains.begin();
                theChain != mChains.end(); ++theChain) {
        if ((*theChain)->getPeriod() > 0) {
            periods.push_back((*theChain)->getPeriod());
        }
    }

    std::sort(periods.begin(), periods.end());

    if (!periods.empty()) {
        mTimer.Start(periods[0]*1000, wxTIMER_ONE_SHOT);
    } else {
        mTimer.Start(1000, wxTIMER_ONE_SHOT);
    }
}

// --------------------------------------
//
void 
UcalTaskbar::runPeriodicChains()
{
    bool needsQuit;

    for (std::vector<Ookala::PluginChain *>::iterator theChain = mChains.begin();
                theChain != mChains.end(); ++theChain) {
        if ((*theChain)->needsPeriodicExecution()) {

            printf("Periodically running %s\n", (*theChain)->name().c_str());

            mRunningChainMutex.lock();
            mRunningChain = (*theChain);
            mRunningChainMutex.unlock();

            (*theChain)->run();

            mRunningChainMutex.lock();
            mRunningChain = NULL;
            needsQuit = mQuitAfterRunning;
            mRunningChainMutex.unlock();

            if (needsQuit) {
                if (mParent) {
                    mParent->ExitMainLoop();
                }
                return;
            }

            printf("run() finished\n");
        }
    }
}

// --------------------------------------
//
void 
UcalTaskbar::cancelRunningChainAndQuit()
{
    mRunningChainMutex.lock();

    if (!mRunningChain) {

        mRunningChainMutex.unlock();

        printf("No running chain, just quit now.\n");
        if (mParent) {
            mParent->ExitMainLoop();
        }

    } else {
        mRunningChain->cancel();
        mQuitAfterRunning = true;
        mRunningChainMutex.unlock();

        printf("Chain running, posting a cancel + quit msg\n");
    }
}


 

BEGIN_EVENT_TABLE(UcalTaskbar, wxTaskBarIcon)
    EVT_MENU(ID_Taskbar_Quit, UcalTaskbar::onQuitButton)
    EVT_MENU(ID_Taskbar_Run,  UcalTaskbar::onRun)
    EVT_TIMER(wxID_ANY,       UcalTaskbar::onTimer)
END_EVENT_TABLE()


// =======================================
//
// UcalApp
//
// ---------------------------------------

UcalApp::UcalApp():
#ifdef __linux__
    wxApp(),
    mSignalHandler(this),
    mSession(this)
#else
    wxApp()
#endif
{

#ifdef __linux__
    mSessionId = NULL;
#endif

}

// --------------------------------------
//
// virtual
bool
UcalApp::OnInit()
{
    std::string configFile;

    mTaskbar        = NULL;
    mListChains     = false;
    mManualChainRun = false;
    mManualChain    = NULL;

    if (!parseArgs()) return false;

    fprintf(stdout, "UcalApp::OnInit()\n");

    WxIconCtrl *iconCtrl = new WxIconCtrl();

    mReg.loadPlugin(iconCtrl, NULL, NULL);

    mReg.loadPlugin(new CalibChecker(), NULL, NULL);

    if (!findConfigFile(configFile)) {
        fprintf(stderr, "Unable to locate config file ucal.xml\n");
        return false;
    }
 
    if (!loadConfig(configFile.c_str(), mReg, mChains)) {
        fprintf(stderr, "Error loading file.\n");
        return false;
    }

    printf("Loaded %d chains\n", (int)mChains.size());

    // If we need to list chains, do so and bail out.
    if (mListChains) {
        uint32_t idx = 0;
        printf("%d chains loaded:\n", (int)mChains.size());

        for (std::vector<Ookala::PluginChain *>::iterator theChain = mChains.begin();
                    theChain != mChains.end(); ++theChain) {
            fprintf(stderr, "\t%d: %s\n", idx, (*theChain)->name().c_str());
            idx++;
        }
        return false;
    }

    // Before we load the taskbar, we need to make sure that
    // we actually have a notification area to load into.
    // Otherwise, badness will ensue. Try spinning on this
    // for a little while, and then yell at the user.
    //
    // If we're just going to run a chain and quit, don't bother
    // setting up the tray icon (or session management).
#ifdef __linux__
    if (!mManualChainRun) {
        Display *dpy  = XOpenDisplay(NULL);
        if (dpy == NULL) {
            fprintf(stderr, "Error opening display");
            return false;
        }

        char atomName[1024];
        sprintf(atomName, "_NET_SYSTEM_TRAY_S%d", DefaultScreen(dpy));

        Atom trayAtom = XInternAtom(dpy, atomName, False);
        if (trayAtom == None) {
            fprintf(stderr, "No XAtom: %s\n", atomName);
            return false;
        }

        Window trayOwner = XGetSelectionOwner(dpy, trayAtom);

        if (trayOwner == None) {
            for (int32_t retry=0; retry<3; retry++) {
                int32_t iter=0;
                while ((trayOwner == None) && (iter < 20)) {

                    sleep(1);

                    trayOwner = XGetSelectionOwner(dpy, trayAtom);
                    iter++;
                }

                if (trayOwner == None) {
					const char* msg = ("Unable to locate system tray!\n\n"
                                     "If running Gnome, this usually means you "
                                     "need a notification area applet in your dock.\n\n"
                                     "If running KDE, this usually means you need"
                                     "a system tray applet in your dock.\n\n"
                                     "Please load the necessary component or cancel.");
                    int retVal = 
                        wxMessageBox(_U(msg), _("uCal: No System Tray!"),
                                      wxCANCEL | wxOK | wxICON_HAND);
                    if (retVal == wxCANCEL) {
                        XCloseDisplay(dpy);
                        return false;
                    }
                }
            }
        }

        XCloseDisplay(dpy);
    }

    // Startup signal handling
    mSignalHandler.mainloop();

    // While we're at it, see if we can fire up a connection
    // to the session manager
    if (!mManualChainRun) {
        if (mSession.hasSms()) {
            mSession.mainloop(mSessionId);
        }
    }
#endif

    // If we're just running a chain, do so and be done.
    if (mManualChainRun) {
               
        for (std::vector<Ookala::PluginChain *>::iterator theChain = mChains.begin();
                    theChain != mChains.end(); ++theChain) {
            if ((*theChain)->name() == mManualChainName) {
                mManualChain = (*theChain);
                (*theChain)->run();        
                mManualChain = NULL;

#ifdef __linux__
                mSignalHandler.shutdown();
#endif
                return false;
            }
        }

        fprintf(stderr, "ERROR: Chain \"%s\" not found\n",
                                             mManualChainName.c_str());
#ifdef __linux__
        mSignalHandler.shutdown();
#endif
        return false;
    }

    mTaskbar = new UcalTaskbar(this, &mReg, mChains);
    
    if (!mTaskbar->IsOk()) {
        printf("Taskbar is not ok!!\n");
        exit(1);
    }
    iconCtrl->setTaskbarIcon(mTaskbar);

    // Run any chains which need to be run initially
    mTaskbar->runPeriodicChains();

    return true;
}

// --------------------------------------
//
// virtual
int
UcalApp::OnExit()
{
    printf("UcalApp::OnExit!\n");

    if (mTaskbar) {
        printf("Deleting taskbar\n");
        delete mTaskbar;
        mTaskbar = NULL;
    }

#ifdef __linux__
    if (mSession.hasSms()) {
        mSession.shutdown();
    }

    mSignalHandler.shutdown();
#endif

    return 0;
}

// --------------------------------------
//

void         
UcalApp::onSignalProcessed(wxEvent &event)
{
    printf("onSignalProcessed()!\n");

    if (mTaskbar) {
printf("calling canelRunningChainAndQuit()\n");
        mTaskbar->cancelRunningChainAndQuit();
    } else {

        if (mManualChain) {
printf("Cancelling manual chain\n");
            mManualChain->cancel();
        } else {
printf("Just quitting\n");
            ExitMainLoop();
        }
    }
}

// --------------------------------------
//

void
UcalApp::onSessionKilled(wxEvent &event)
{
    printf("onSessionKilled()\n");

#ifdef __linux__
    if (mSession.hasSms()) {
        mSession.shutdown();
    }

    // This will signal to the signal thread, which will pass us 
    // back a message to take down the taskbar and exit.
    mSignalHandler.shutdown();
#endif

}




// --------------------------------------
//
// protected
bool
UcalApp::parseArgs()
{
    wxString        stringVal;
    wxCmdLineParser parser(argc, argv);

    parser.EnableLongOptions(true);

#ifdef __linux__
    parser.AddOption(wxT("s"), wxT("sm-client-id"), _("Session Management ID"),
                        wxCMD_LINE_VAL_STRING, wxCMD_LINE_NEEDS_SEPARATOR);
#endif
    
    // -l, --list -> list all chains and exit
    parser.AddSwitch(wxT("l"), wxT("list"), _("List all chains found in the config"));

    // -r "chainName, --run "chainName" --> execute a chain and exit
    parser.AddOption(wxT("r"), wxT("run"), _("Execute a chain and exit"),
                         wxCMD_LINE_VAL_STRING);

    // Return and display usage if things go wrong.
    if (parser.Parse(true)) {
        return false;
    }

#ifdef __linux__
    if (parser.Found(wxT("s"), &stringVal)) {
        mSessionId = strdup(stringVal.mb_str());
    }
#endif

    mListChains = parser.Found(wxT("l"));

    mManualChainRun = parser.Found(wxT("r"), &stringVal);
    if (mManualChainRun) {
        mManualChainName = stringVal.mb_str();

        // Trim any head/tail space
        size_t start = mManualChainName.find_first_not_of(" \t\n");
        size_t end   = mManualChainName.find_last_not_of(" \t\n");

        if (start == std::string::npos) {
            mManualChainName = "";
        } else {
            mManualChainName = mManualChainName.substr(start, end-start+1);
        }
    }

    return true;
}

// --------------------------------------
//
// protected

bool     
UcalApp::findConfigFileXdg(std::string &configFile)
{
    // Search in this order:
    //   $XDG_CONFIG_HOME  = $home/.config by default
    //   $XDG_CONFIG_DIRS  = /etc/xdg by default, can be : delimited
    
    std::vector<std::string> searchPaths;
    std::string fileName = "/ucal/ucal.xml";

    // Add $XDG_CONFIG_HOME to the mix first
    if ( getenv("XDG_CONFIG_HOME") ) {
            searchPaths.push_back( std::string( getenv("XDG_CONFIG_HOME") ) + 
                                   std::string( "/.config"));
    } else {
        char *homeDir = getenv("HOME");

        if (homeDir) {
            searchPaths.push_back( std::string(homeDir) + std::string("/.config") );
        }
    }
    
    // Then parse our all the paths in $XDG_CONFIG_DIRS and add them
    // to the search in order that they appear
    char *xdg_config_dirs = getenv("XDG_CONFIG_DIRS");
    if (!xdg_config_dirs) {
        searchPaths.push_back("/etc/xdg");
    } else {
        std::string            xdgConfigDirs(xdg_config_dirs);
        std::string            subStr;
        std::string::size_type start = 0;
        std::string::size_type end;

        printf("XDG_CONFIG_DIRS: %s\n", xdg_config_dirs);

        end = xdgConfigDirs.find(':', start);
        while (end != std::string::npos) {
            
            subStr = xdgConfigDirs.substr(start, end-start);
            if (!subStr.empty()) {
                printf("pushing %s\n", subStr.c_str());
                searchPaths.push_back(subStr);
            }

            start = end + 1;
            end = xdgConfigDirs.find(':', start);
        }

        // Push the last bit
        subStr = xdgConfigDirs.substr(start, std::string::npos);
        if (!subStr.empty()) {
            printf("Pushing %s\n", subStr.c_str());
            searchPaths.push_back(subStr);
        }
    }
    
    printf("Search paths:\n");
    for (std::vector<std::string>::iterator thePath = searchPaths.begin();
            thePath != searchPaths.end(); ++thePath) {
        std::string fullPath = (*thePath) + fileName;
        FILE       *fid;

        printf("%s\n", fullPath.c_str());

        fid = fopen(fullPath.c_str(), "r");
        if (fid) {
            fclose(fid);
            configFile = fullPath;
            return true;
        }
    }


    return false;
}

// --------------------------------------
//
// protected

bool     
UcalApp::findConfigFile(std::string &configFile)
{
#ifdef __linux__

    return findConfigFileXdg(configFile);


#elif defined _WIN32
    FILE       *fid = NULL;
    TCHAR       appPath[MAX_PATH];

#ifdef UNICODE
    std::wstring appPathStr;
    std::wstring fileNameStr = _T("\\ucal.xml");
#else
    std::string  appPathStr;
    std::string  fileNameStr = _T("\\ucal.xml");
#endif

    for (int idx=0; idx<MAX_PATH; ++idx) {
        appPath[idx] = 0;
    }

    GetModuleFileName(NULL, appPath, MAX_PATH-1);
    appPathStr = appPath;
    appPathStr = appPathStr.substr(0, appPathStr.rfind("\\"));

    printf("App Directroy: %s\n", appPathStr.c_str());

    appPathStr += fileNameStr;

#ifdef UNICODE
    _wfopen_s(&fid, appPathStr.c_str(), "r");
#else
    fopen_s(&fid, appPathStr.c_str(), "r");
#endif

    if (fid) {
        fclose(fid);
        configFile = appPathStr;
        printf("Found %s\n", appPathStr.c_str());
        return true;
    }

    return false;

#else

    configFile = "";
    fprintf(stderr, "No way to find config files - giving up?\n");
    return false;


#endif
}


// --------------------------------------
//
// Deal with <dict>...</dict> nodes. 
// protected
void
UcalApp::handleDictNode(xmlDocPtr doc, xmlNodePtr root, 
                Ookala::DictHash *dhash, std::string &dictName)
{
    if (root == NULL) return;

    std::string rootName((char *)root->name);
    
    // If node is a dict, grab it's name and unserialize it
    if (rootName == "dict") {
        dictName = "";

        printf("\tLoading Dict...\n");
        
        xmlChar *attrName = xmlGetProp(root, (const xmlChar *)("name"));
        if (attrName != NULL) {
            dictName = (char *)(attrName);
            xmlFree(attrName);
        }

        printf("\t   Name: %s\n", dictName.c_str());

        Ookala::Dict *dict = dhash->newDict(dictName.c_str());
        if (!dict) {
            fprintf(stderr, "WARNING: Unable to create dict \"%s\"\n",
                                         dictName.c_str());
            return;
        }

        dict->unserialize(doc, root);
        return;
    }
}

// --------------------------------------
//
// handle <plugin>pluginName</plugin> -> look up the
// plugin in the registry and return ptr if found, 
// or NULL otherwise.
//
// protected

Ookala::Plugin *
UcalApp::handlePluginNode(xmlDocPtr doc, xmlNodePtr root, 
                                    Ookala::PluginRegistry &registry)
{
    if (root == NULL) return NULL;

    std::string rootName((char *)root->name);
    
    if (rootName == "plugin") {
        std::string pluginName;
        std::vector<Ookala::Plugin *>  plugins;
    
        xmlChar *value = xmlNodeListGetString(doc, root->xmlChildrenNode, 1);
        if (value != NULL) {
            pluginName = (char *)value;            
            xmlFree(value);
        } else {
            return NULL;
        }

        printf("Found plugin; %s\n", pluginName.c_str());


        plugins = registry.queryByName(pluginName);
        if (plugins.empty()) {
            fprintf(stderr, "ERROR: Plugin %s not loaded..\n", pluginName.c_str());
            return NULL;
        }

        return plugins[0];
    }

    return NULL;
}

// --------------------------------------
//
// Deal with <loadplugin>filename</loadplugin>. Try to
// load the file and return if we succeeded/failed.
//
// protected

bool
UcalApp::handleLoadPluginNode(xmlDocPtr doc, xmlNodePtr root,
                           Ookala::PluginRegistry &registry)
{
    if (root == NULL) return false;

    std::string rootName((char *)root->name);
    
    if (rootName == "loadplugin") {
        std::string pluginName;
    
        xmlChar *value = xmlNodeListGetString(doc, root->xmlChildrenNode, 1);
        if (value != NULL) {
            pluginName = (char *)value;            
            xmlFree(value);
        } else {
            return false;
        }

        printf("Found plugin; %s\n", pluginName.c_str());


        if (!registry.loadPlugin(pluginName.c_str())) {
            fprintf(stderr, "ERROR: Can't load plugin %s\n",
                                    pluginName.c_str());
            return false;
        }

        return true;
    }

    return false;
}

// --------------------------------------
//
// This is a big switch statement based on the 
// tag types we care about. 
//
// protected
void
UcalApp::handleXmlNode(xmlDocPtr doc,    
                       xmlNodePtr root, 
                       Ookala::DictHash *dhash, 
                       Ookala::PluginRegistry &registry,
                       std::vector<Ookala::PluginChain *> &chains)
{
    if (root == NULL) return;

    //xmlNodePtr  child = NULL;
    std::string dictName;
    std::string rootName((char *)root->name);
    
    printf("got element: [%s]\n", root->name);

    // If node is a dict, grab it's name and unserialize it
    if (rootName == "dict") {
        handleDictNode(doc, root, dhash, dictName);
    }

    // If we're supposed to load a plugin, do that.
    if (rootName == "loadplugin") {
        handleLoadPluginNode(doc, root, registry);
    }

    // If the node is a chain, grab that.
    if (rootName == "chain") {        
        Ookala::PluginChain   *chain;

        chain = new Ookala::PluginChain(&registry);
        if (!chain->unserialize(doc, root)) {
            delete chain;
            chain = NULL;
        } else {
            chains.push_back(chain);
        }
    }

}

// ---------------------------------
// 
// Open up our config file, walk through the
// tree, and handle the various tag types
// that we run into.
//
// protected
bool
UcalApp::loadConfig(std::string filename, Ookala::PluginRegistry &reg,
                    std::vector<Ookala::PluginChain *> &chains)
{
    xmlDocPtr                      doc;
    xmlNodePtr                     root, child;
    Ookala::DictHash              *hash = NULL;
    std::vector<Ookala::Plugin *>  plugins;

    plugins = reg.queryByName("DictHash");
    if (plugins.empty()) {
        fprintf(stderr, "ERROR: No DictHash found\n");
        return false;
    }

    hash = (Ookala::DictHash *)(plugins[0]);
 
    doc = xmlParseFile(filename.c_str());
    if (doc == NULL) {
        printf("Can't parse file\n");
        return false;
    }

    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        printf("no root!\n");
        return false;
    }

    printf("root element: %s\n", root->name);

    handleXmlNode(doc, root, hash, reg, chains);
    child = root->xmlChildrenNode;
    while (child != NULL) {
        handleXmlNode(doc, child, hash, reg, chains); 

        child = child->next;
    }

    xmlFreeDoc(doc);

    return true;
}


BEGIN_EVENT_TABLE(UcalApp, wxApp)
    EVT_CUSTOM(signalProcessedEvent,wxID_ANY, UcalApp::onSignalProcessed)
    EVT_CUSTOM(sessionKilledEvent,  wxID_ANY, UcalApp::onSessionKilled)
END_EVENT_TABLE()


IMPLEMENT_APP_CONSOLE(UcalApp)
