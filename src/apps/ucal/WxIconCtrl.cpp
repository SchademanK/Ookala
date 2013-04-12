// --------------------------------------------------------------------------
// $Id: WxIconCtrl.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include "Dict.h"
#include "DictHash.h"
#include "PluginRegistry.h"
#include "PluginChain.h"

#include "WxIconCtrl.h"

#include "wxUtils.h"

#include "default_icon.xpm"

// ----------------------------------
WxIconCtrl::WxIconCtrl():
    Ookala::Plugin()
{
    setName("WxIconCtrl");
    mTaskbarIcon = NULL;
}

// ----------------------------------
//
WxIconCtrl::WxIconCtrl(const WxIconCtrl &src):
    Ookala::Plugin(src)
{
    mTaskbarIcon = src.mTaskbarIcon;
    mIcons       = src.mIcons;
}

// ----------------------------------
//
// virtual
WxIconCtrl::~WxIconCtrl()
{
}

// ----------------------------
//
WxIconCtrl &
WxIconCtrl::operator=(const WxIconCtrl &src)
{
    if (this != &src) {
        Ookala::Plugin::operator=(src);
    }

    return *this;
}

// ----------------------------------

bool
WxIconCtrl::setTaskbarIcon(wxTaskBarIcon *taskbarIcon)
{
    mTaskbarIcon = taskbarIcon;

    return true;
}

// ----------------------------------
bool
WxIconCtrl::setIcon(const std::string &iconKey)
{
    if (mTaskbarIcon == NULL) {
        setErrorString("No taskbar icon control found.");
        return false;
    }


    std::map<std::string, wxIcon>::iterator theIcon = mIcons.find(iconKey);

    if (theIcon == mIcons.end()) {
        wxIcon defaultIcon(default_icon_xpm);

        for (int i=0; i<5; ++i) {
            mTaskbarIcon->SetIcon(defaultIcon, wxT(""));
        }

        setErrorString(std::string("No icon found named ") + iconKey);        
        return false;
    }


    // Try installing the icon multiple times, becase sometimes 
    // it won't catch on the first time?
    for (int i=0; i<5; ++i) {
        mTaskbarIcon->SetIcon((*theIcon).second, wxT(""));
    }

    return true;
}


// ----------------------------------
//
// This is looking for:
//
//    WxIconCtrl::set  -> Hash key of icon to set [STRING]
//    WxIconCtrl::load -> keys to filename entries in the same 
//                        dict [STRING ARRAY].
//
//
// For loading, we might setup something like:
//       Chain Dict["WxIconCtrl::load"] = {"Icon0_good", "Icon0_bad"}
//       Chain Dict["Icon0_good"]       = "/usr/share/icons/foo_good.xpm" 
//       Chain Dict["Icon0_bad"]        = "/usr/share/icons/foo_bad.xpm" 
// 
// virtual protected
bool
WxIconCtrl::_run(Ookala::PluginChain *chain)
{
    Ookala::Dict                *dict       = NULL;
    Ookala::DictHash            *hash       = NULL;
    Ookala::DictItem            *item       = NULL;
    Ookala::StringDictItem      *stringItem = NULL;
    bool                         debug      = true;

    setErrorString("");

    if (mRegistry == NULL) {
        setErrorString("No PluginRegistry for WxIconCtrl.");
        return false;
    }   

    if (chain == NULL) {
        setErrorString("No chain for WxIconCtrl.");
        return false;
    }

    // Retrieve the chain dictionary
    std::vector<Ookala::Plugin *> plugins = mRegistry->queryByName("DictHash");
    if (plugins.empty()) {
        if (debug) {
            fprintf(stderr, "No DictHash found for WxIconCtrl.");
        }
        return false;
    }
    hash = dynamic_cast<Ookala::DictHash *>(plugins[0]);
    if (!hash) {
        if (debug) {
            fprintf(stderr, "No DictHash found for WxIconCtrl.");
        }       
        return false;
    }

    dict = hash->getDict(chain->getDictName().c_str());
    if (!dict) {
        if (debug) {
            fprintf(stderr, "No chain Dict found for WxIconCtrl.");
        }
        return false;
    }


    if (!loadIcons(dict)) {
        return false;
    }

    // Search the chain dict for WxIconCtrl::set
    item = dict->get("WxIconCtrl::set");
    if (!item) {
        if (debug) {
            fprintf(stderr, "NOTE: No 'WxIconCtrl::set' [STRING] in Dict %s\n",
                    chain->getDictName().c_str());
        } 
        return true;
    }

    stringItem = dynamic_cast<Ookala::StringDictItem *>(item);
    if (!stringItem) {
        if (debug) {
            fprintf(stderr, "No 'WxIconCtrl::set' [STRING] in Dict %s\n",
                    chain->getDictName().c_str());
        } 
        return false;
    }

    return setIcon(stringItem->get());
}


// ----------------------------------
//
// This should only return false if something _really_ 
// bad happeend which should cause the running of the
// plugin to fail. Not being able to load a file is
// not reason enough to return false (though you might
// want to spew out a warning...)
//
// virtual
bool
WxIconCtrl::loadIcons(Ookala::Dict *chainDict)
{   
    Ookala::DictItem                *item            = NULL;
    Ookala::StringArrayDictItem     *stringArrayItem = NULL;
    Ookala::StringDictItem          *stringItem      = NULL;
    bool                             debug           = true;
    std::vector<std::string>         keys;

    if (chainDict == NULL) {
        return true;
    }

    // Look for the list of keys to icon files, "WxIconCtrl::load"
    item = chainDict->get("WxIconCtrl::load");
    if (!item) {
        if (debug) {
            fprintf(stderr, "NOTE: No WxIconCtrl::load. Nothing to load.\n");
        }
        return true;
    }
    stringArrayItem = dynamic_cast<Ookala::StringArrayDictItem *>(item);
    if (!stringArrayItem) {
        if (debug) {
            fprintf(stderr, "NOTE: WxIconCtrl::load is not of type [String Array]\n");
        }
        return true;
    }
    keys = stringArrayItem->get();

    // Walk over all the keys, and look for filenames to load
    for (std::vector<std::string>::iterator theKey = keys.begin();
                theKey != keys.end(); ++theKey) {

        item = chainDict->get((*theKey).c_str());
        if (!item) {
            if (debug) {
                fprintf(stderr, "NOTE: WxIconCtrl::load couldn't find key %s\n",
                                            (*theKey).c_str());
            }
            continue;
        }
        stringItem = dynamic_cast<Ookala::StringDictItem *>(item);
        if (!stringItem) {
            if (debug) {
                fprintf(stderr, "NOTE: %s is not of type [String]\n",
                                             (*theKey).c_str());
            }
            continue;
        }

        // See if we can open the file; Wx can give some spectacular
        // issues if it can't find a file..
        FILE *fid = fopen(stringItem->get().c_str(), "r");
        if (!fid) {
            fprintf(stderr, "NOTE: Error opening %s\n",
                            stringItem->get().c_str());
            continue;
        }
        fclose(fid);

        wxIcon icon;

        if (!icon.LoadFile(_S(stringItem->get()), wxBITMAP_TYPE_XPM)) {
            if (debug) {
                fprintf(stderr, "NOTE: Error reading %s as .xpm\n",
                            stringItem->get().c_str());
            }
            continue;
        }

        mIcons[*theKey] = icon;
    }
    

    return true;
}




