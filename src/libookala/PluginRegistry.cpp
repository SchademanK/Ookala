// --------------------------------------------------------------------------
// $Id: PluginRegistry.cpp 135 2008-12-19 00:49:58Z omcf $
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

#ifdef __linux__
#include <dlfcn.h>
#endif

#include "Plugin.h"
#include "PluginRegistry.h"
#include "DictHash.h"
#include "Color.h"
#include "DataSavior.h"
#include "Interpolate.h"


// ===================================
//
// PluginRegistry
//
// -----------------------------------

Ookala::PluginRegistry::PluginRegistry()
{
    mPluginRegistryData = new _PluginRegistry;    

    loadPlugin(static_cast<Plugin *>(new DictHash),    NULL, NULL);
    loadPlugin(static_cast<Plugin *>(new Color),       NULL, NULL);
    loadPlugin(static_cast<Plugin *>(new Interpolate), NULL, NULL);

    loadPlugin(static_cast<Plugin *>(new DataSavior), 
                    DataSavior::dictitem_create,
                    DataSavior::dictitem_destroy);
}

// -----------------------------------
//
Ookala::PluginRegistry::PluginRegistry(const PluginRegistry &src)
{
    mPluginRegistryData = new _PluginRegistry;    
    
    if (src.mPluginRegistryData) {
        mPluginRegistryData->mPluginInfo = src.mPluginRegistryData->mPluginInfo;
        mPluginRegistryData->mLibHandles = src.mPluginRegistryData->mLibHandles;
    }    
}

// -----------------------------------
//
Ookala::PluginRegistry::~PluginRegistry()
{
    if (mPluginRegistryData) {

        // Walk the list of plugins and destroy them all
        for (std::vector<struct PluginInfo>::iterator i = 
                    mPluginRegistryData->mPluginInfo.begin();
                i != mPluginRegistryData->mPluginInfo.end(); i++) {
            if ((*i).deleteFunc) {
                (*(*i).deleteFunc)((*i).plugin);
            } else if (((*i).plugin) && ((*i).builtinPlugin)) {
                delete (*i).plugin;
            }
        }

        // Then walk the list of lib handles and close them all
        for (std::vector<LibHandle>::iterator j = 
                    mPluginRegistryData->mLibHandles.begin();
                j != mPluginRegistryData->mLibHandles.end(); j++) {
#ifdef __linux__
            if (dlclose(*j) != 0) {
                fprintf(stderr, "dlclose(): %s\n", dlerror());
            }
#elif defined _WIN32
            FreeLibrary(*j);
#endif
        }
   
        delete mPluginRegistryData;
        mPluginRegistryData = NULL;
    }

}

// ----------------------------
//
Ookala::PluginRegistry &
Ookala::PluginRegistry::operator=(const PluginRegistry &src)
{
    if (this != &src) {
        if (mPluginRegistryData) {
            delete mPluginRegistryData;
            mPluginRegistryData = NULL;
        }

        if (src.mPluginRegistryData) {
            mPluginRegistryData  = new _PluginRegistry();
            *mPluginRegistryData = *(src.mPluginRegistryData);
        }
    }

    return *this;
}

// -----------------------------------
//
// Load in a DSO from a file and pull in all the 
// plugins that it contains.
//
// virtual

bool
Ookala::PluginRegistry::loadPlugin(const char *filename)
{
    int numPlugins;
    LibHandle  hand;
    RegisterFunc registerFunc;
    struct PluginInfo pi;
    struct PluginAllocInfo *allocInfo;

    if (!mPluginRegistryData) return false;

    pi.registered    = false;
    pi.builtinPlugin = false;

    pi.dsoName.assign(filename);

    hand = NULL;

#ifdef __linux__
    hand = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);
#elif defined _WIN32
    hand = LoadLibrary(filename);
#endif


    if (hand == NULL) {
        fprintf(stderr, "Error opening %s\n", filename);
#ifdef __linux__
        fprintf(stderr, "%s\n", dlerror());
#elif defined _WIN32
        char errMsg[1024];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)errMsg, 1023, NULL);
        fprintf(stderr, "\t%s\n", errMsg);
#endif
        return false;
    }    

    registerFunc = NULL;

#ifdef __linux__
    registerFunc = (RegisterFunc)dlsym(hand, "pluginRegister");
#elif defined _WIN32
    registerFunc = (RegisterFunc)GetProcAddress(hand, "pluginRegister");
#endif

    if (registerFunc == NULL) {
        fprintf(stderr, "Can't find pluginRegister() in %s\n", filename);
#ifdef __linux__
        fprintf(stderr, "%s\n", dlerror());
        dlclose(hand);
#elif defined _WIN32
        FreeLibrary(hand);
#endif
        return false;
    }    

    allocInfo = (*registerFunc)(&numPlugins);
    if ((allocInfo == NULL) || (numPlugins == 0)) {
        fprintf(stderr, "Can't find any plugin information in %s\n", filename);
#ifdef __linux__
        dlclose(hand);
#elif defined _WIN32
        FreeLibrary(hand);
#endif
        return false;
    }

    bool retVal = false;
    for (int i=0; i<numPlugins; ++i) {

        bool nameFound;

        pi.createFunc         = allocInfo[i].createFunc;
        pi.deleteFunc         = allocInfo[i].deleteFunc;
        pi.dictItemCreateFunc = allocInfo[i].dictItemCreateFunc;
        pi.dictItemDeleteFunc = allocInfo[i].dictItemDeleteFunc;
        pi.pluginName         = allocInfo[i].name;

        pi.handle             = hand;

        // Test if we already have a plugin of this name loaded. If so, skip it
        // for now.
        nameFound = false;
        for (std::vector<PluginInfo>::iterator theInfo = 
                               mPluginRegistryData->mPluginInfo.begin();
                    theInfo != mPluginRegistryData->mPluginInfo.end(); 
                                                            ++theInfo) {
            if (pi.pluginName == (*theInfo).pluginName) {
                nameFound = true;
            }
        }
        if (nameFound) {
            continue;
        }

        pi.plugin = NULL;
        pi.plugin = (*pi.createFunc)();
        if (pi.plugin == NULL) {
            fprintf(stderr, "Problem creating plugin object from %s\n", filename);
#ifdef __linux__
            fprintf(stderr, "%s\n", dlerror());
#endif

            continue;
        }

        // After creating an object, run postLoad() to see if it fails
        // immediately. If so, don't bother keeping track of it.
        if (pi.plugin->postLoad() == false) {
            (*pi.deleteFunc)(pi.plugin);
            fprintf(stderr, "WARNING: %s failed postLoad()\n", pi.pluginName.c_str());
            continue;
        }

        // Loaded successfully and passed postLoad, so its safe to
        // add it into the list of what we care about. But first, 
        // we need to remember to assigne a pointer back to the 
        // plugin registry in each plugin.

        pi.plugin->setPluginRegistry(this);

        mPluginRegistryData->mPluginInfo.push_back(pi);

        printf("Loaded %s from %s\n",
            pi.pluginName.c_str(),
            pi.dsoName.c_str());

        retVal = true;
    }

    if (retVal) {
        mPluginRegistryData->mLibHandles.push_back(hand);
        registerPlugins();
    } else {
#ifdef __linux__
        dlclose(hand);
#elif defined _WIN32
        FreeLibrary(hand);
#endif
    }

    return retVal;
}

// -----------------------------------
//
// virtual
bool
Ookala::PluginRegistry::loadPlugin(
                           Plugin             *plugin,
                           DictItemCreateFunc  dictItemCreate,
                           DictItemDeleteFunc  dictItemDelete)
{
    if (!mPluginRegistryData) return false;

    // Load in any builtin plugins
    PluginInfo pi;

    pi.pluginName = plugin->name();
    pi.plugin     = plugin;
    
    pi.registered         = false;
    pi.builtinPlugin      = true;
    pi.handle             = NULL;
    pi.createFunc         = NULL;
    pi.deleteFunc         = NULL;
    pi.dictItemCreateFunc = dictItemCreate;
    pi.dictItemDeleteFunc = dictItemDelete;

    if (!plugin->postLoad()) {
        return false;
    }

    if (!plugin->setPluginRegistry(this)) {
        return false;
    }

    mPluginRegistryData->mPluginInfo.push_back(pi);
   
    return registerPlugins();
}


// -----------------------------------
// Look through various places and load in plugins that
// seem to be valid
//
// virtual
bool
Ookala::PluginRegistry::registerPlugins()
{
    if (!mPluginRegistryData) return false;

    // Once everything is loaded, run checkDeps() on all the plugins
    // so they can check if everything they need has been loaded.
    // If they fail, take them out of the list.

    std::vector<struct PluginInfo>::iterator i = 
                        mPluginRegistryData->mPluginInfo.begin();
    while (i != mPluginRegistryData->mPluginInfo.end()) {
        if ((*i).registered == false) {
            // If we fail, don't advance the itertor. erase() will
            // give us the next item in the vector
            if ((*i).plugin->checkDeps() == false) {
                fprintf(stderr, 
                    "WARNING: %s failed checkDeps()\n",
                                     (*i).pluginName.c_str());
                if ((*i).deleteFunc) {
                    (*(*i).deleteFunc)((*i).plugin);
                } else if (((*i).plugin) && ((*i).builtinPlugin)) {
                    delete (*i).plugin;
                }
                i = mPluginRegistryData->mPluginInfo.erase(i);
            } else {
                ++i;
            }
        } else {
            ++i;
        }
    }
    
    // Then, make a pass post-checkDeps. Values of checkDeps() should be
    // cached at this point and safe to query w/o fear of circular 
    // dependancies.
    i = mPluginRegistryData->mPluginInfo.begin();
    while (i != mPluginRegistryData->mPluginInfo.end()) {
        if ((*i).registered == false) {
            // If we fail, don't advance the itertor. erase() will
            // give us the next item in the vector
            if ((*i).plugin->postCheckDeps() == false) {
                fprintf(stderr, "WARNING: %s failed postCheckDeps()\n", 
                                            (*i).pluginName.c_str());
                if ((*i).deleteFunc) {
                    (*(*i).deleteFunc)((*i).plugin);
                } else if (((*i).plugin) && ((*i).builtinPlugin)) {
                    delete (*i).plugin;
                }
                i = mPluginRegistryData->mPluginInfo.erase(i);
            } else {
                ++i;
            }
        } else {
            ++i;
        }
    }

    return true;
}

// -----------------------------------
//
// Search for a plugin with an exact match to the given
// name. If nothing is found, return an empty vector.

std::vector<Ookala::Plugin *>
Ookala::PluginRegistry::queryByName(const std::string name)
{
    std::vector<Plugin *> matches;
    std::string goal(name);

    if (!mPluginRegistryData) return matches;

    for (std::vector<struct PluginInfo>::iterator i = 
                 mPluginRegistryData->mPluginInfo.begin();
            i != mPluginRegistryData->mPluginInfo.end(); ++i) {

        if ((*i).plugin->matchName(goal)) {
            matches.push_back((*i).plugin);
        }        
    }
    return matches;
}

// -----------------------------------
//
// Search for a plugin that has a given set of attributes.
// If a plugin matches all specified attributes, it's counted
// as a hit and returned. If nothing matches, we return
// an empty vector.

std::vector<Ookala::Plugin *>
Ookala::PluginRegistry::queryByAttribute(const std::string attrib)
{
    std::vector<Plugin *> matches;
    std::string goal(attrib);

    if (!mPluginRegistryData) return matches;

    for (std::vector<struct PluginInfo>::iterator i = 
                 mPluginRegistryData->mPluginInfo.begin();
            i != mPluginRegistryData->mPluginInfo.end(); ++i) {

        if ((*i).plugin->matchAttribute(goal)) {
            matches.push_back((*i).plugin);
        }        
    }

    return matches;
}

// -----------------------------------
//
// Search for a plugin that has a given set of attributes.
// If a plugin matches all specified attributes, it's counted
// as a hit and returned. If nothing matches, we return
// an empty vector.

std::vector<Ookala::Plugin *>
Ookala::PluginRegistry::queryByAttributes(std::vector<std::string> attribs)
{
    std::vector<Plugin *> matches;

    if (!mPluginRegistryData) return matches;

    for (std::vector<struct PluginInfo>::iterator i = 
                 mPluginRegistryData->mPluginInfo.begin();
            i != mPluginRegistryData->mPluginInfo.end(); ++i) {

        if ((*i).plugin->matchAttributes(attribs)) {
            matches.push_back((*i).plugin);
        }        
    }

    return matches;
}



// -----------------------------------
//

std::vector<Ookala::Plugin *>
Ookala::PluginRegistry::queryByIndex(const int idx)
{
    std::vector<Plugin *> matches;

    if (!mPluginRegistryData)           return matches;
    if (idx <  0)                       return matches;

    if (idx >= (int)mPluginRegistryData->mPluginInfo.size()) {
        return matches;
    }

    matches.push_back(mPluginRegistryData->mPluginInfo[idx].plugin);
    
    return matches;
}


// -----------------------------------
//

const int
Ookala::PluginRegistry::numPlugins()
{
    if (!mPluginRegistryData) return 0;

    return (int)mPluginRegistryData->mPluginInfo.size();
}


// -----------------------------------
//

Ookala::DictItem *
Ookala::PluginRegistry::createDictItem(const std::string typeName) 
{
    if (!mPluginRegistryData) return NULL;

    // First, check out all our built-in types
    if (typeName == "bool") {
        return new BoolDictItem;
    }

    if (typeName == "int") {
        return new IntDictItem;
    }

    if (typeName == "double") {
        return new DoubleDictItem;
    }

    if (typeName == "string") {
        return new StringDictItem;
    }

    if (typeName == "blob") {
        return new BlobDictItem;
    }
        
    if (typeName == "intArray") {
        return new IntArrayDictItem;
    }

    if (typeName == "doubleArray") {
        return new DoubleArrayDictItem;
    }

    if (typeName == "stringArray") {
        return new StringArrayDictItem;
    }

    // If we're not requesting one of those, walk the plugin list
    // and see if anyone recognizes this type
    for (std::vector<PluginInfo>::iterator plugin = 
                          mPluginRegistryData->mPluginInfo.begin();
                plugin != mPluginRegistryData->mPluginInfo.end(); 
                                                        ++plugin) {

        DictItem *item = NULL;

        if ((*plugin).dictItemCreateFunc) {
            item = ((*plugin).dictItemCreateFunc)(typeName.c_str());
            if (item) {
                return item;
            }
        }
    }
    
    return NULL;
}

// -----------------------------------
//

bool
Ookala::PluginRegistry::deleteDictItem(DictItem *item) 
{
    if (!mPluginRegistryData) return false;

    if ((item->itemType() == "bool") ||
        (item->itemType() == "int") ||
        (item->itemType() == "double") ||
        (item->itemType() == "string") ||
        (item->itemType() == "blob") ||
        (item->itemType() == "intArray") ||
        (item->itemType() == "doubleArray") ||
        (item->itemType() == "stringArray")) {
        delete item;
        return true;
    }

    // If we're not requesting one of those, walk the plugin list
    // and see if anyone recognizes this type
    for (std::vector<PluginInfo>::iterator plugin = 
                          mPluginRegistryData->mPluginInfo.begin();
                plugin != mPluginRegistryData->mPluginInfo.end(); 
                                                        ++plugin) {

        if ((*plugin).dictItemDeleteFunc) {
            if (((*plugin).dictItemDeleteFunc)(item) == 0) {
                return true;
            }
        }
    }
    
    return false;
}
