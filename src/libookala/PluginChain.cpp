// --------------------------------------------------------------------------
// $Id: PluginChain.cpp 135 2008-12-19 00:49:58Z omcf $
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
#include <time.h>

#include <algorithm>

#ifdef __linux__
#include <unistd.h>
#endif

#include "Types.h"
#include "Dict.h"
#include "DictHash.h"
#include "Plugin.h"
#include "PluginRegistry.h"
#include "PluginChain.h"
#include "Ui.h"


// =========================================
//
// PluginChain
//
// -----------------------------------------

Ookala::PluginChain::PluginChain(PluginRegistry *registry)
{
    mRegistry          = registry;
    mCancelFlag        = false;
    mLastExecutionTime = 0;
    mPeriod            = 0;
    mHidden            = false;

    mPluginChainData   = new _PluginChain;
    mPluginChainData->mDictName    = "";
    mPluginChainData->mErrorString = "";
    mPluginChainData->mName        = "";
}

// -----------------------------------------
//
Ookala::PluginChain::PluginChain(const PluginChain &src)
{
    mRegistry          = src.mRegistry;
    mCancelFlag        = src.mCancelFlag;
    mLastExecutionTime = src.mLastExecutionTime;
    mPeriod            = src.mPeriod;
    mHidden            = src.mHidden;

    mPluginChainData   = new _PluginChain;

    mPluginChainData->mDictName    = "";
    mPluginChainData->mErrorString = "";
    mPluginChainData->mName        = "";

    if (src.mPluginChainData) {
        mPluginChainData->mDictName = src.mPluginChainData->mDictName;
        mPluginChainData->mErrorString = src.mPluginChainData->mErrorString;
        mPluginChainData->mName = src.mPluginChainData->mName;
        mPluginChainData->mPluginChain = src.mPluginChainData->mPluginChain;        
    }
}

// -----------------------------------------
//
// virtual
Ookala::PluginChain::~PluginChain()
{
    if (mPluginChainData) {
        delete mPluginChainData;
        mPluginChainData = NULL;
    }
}

// ----------------------------
//
Ookala::PluginChain &
Ookala::PluginChain::operator=(const PluginChain &src)
{
    if (this != &src) {
        if (mPluginChainData) {
            delete mPluginChainData;
            mPluginChainData = NULL;
        }

        if (src.mPluginChainData) {
            mPluginChainData  = new _PluginChain();
            *mPluginChainData = *(src.mPluginChainData);
        }

        mRegistry          = src.mRegistry;
        mCancelFlag        = src.mCancelFlag;
        mLastExecutionTime = src.mLastExecutionTime;
        mPeriod            = src.mPeriod;
        mHidden            = src.mHidden;
    }

    return *this;
}



// -----------------------------------------
//
void 
Ookala::PluginChain::setErrorString(const std::string &str)
{
    if (mPluginChainData) {
        mPluginChainData->mErrorString = str;
    }
}

// -----------------------------------------
//
std::string
Ookala::PluginChain::errorString()
{
    if (mPluginChainData) {
        return mPluginChainData->mErrorString;
    }

    return std::string("");
}

// -----------------------------------------
//
void
Ookala::PluginChain::setName(const std::string name)
{
    if (mPluginChainData) {
        mPluginChainData->mName = name;
    }
}

// -----------------------------------------
//
const std::string
Ookala::PluginChain::name()
{
    if (mPluginChainData) {
        return mPluginChainData->mName;
    }

    return std::string("");
}

// -----------------------------------------
//
void
Ookala::PluginChain::setDictName(const std::string dictName)
{
    if (mPluginChainData) {
        mPluginChainData->mDictName = dictName;        
    }
}

// -----------------------------------------
//
std::string
Ookala::PluginChain::getDictName()
{
    if (mPluginChainData) {
        std::string myName = mPluginChainData->mDictName;
        return myName;
    } 

    return std::string("");
}

// -----------------------------------------
//
Ookala::Dict *
Ookala::PluginChain::getDict()
{    
    DictHash *hash = NULL;

    if (!mPluginChainData) return NULL;
    if (!mRegistry)        return NULL;
        

    std::vector<Plugin *>plugins = mRegistry->queryByName("DictHash");
    if (plugins.empty()) {
        return NULL;
    }
    hash = dynamic_cast<DictHash *>(plugins[0]);
    if (!hash) {
        return NULL;
    }

    return hash->getDict(getDictName().c_str());
}


// -----------------------------------------
//
// The execution of all the plugins in the chain consists 
// of first running preRun(), followed by run() and postRun().
//
// If a plugin was cancelled, it should return false on run(),
// however, preRun() and postRun() should ignore the cancel 
// test and cleanup. 
//
// If execution was cancelled, this should return false. Also,
// if something went horribly wrong, this will return false.
// A pox upon boolean return values!! If something goes wrong,
// however, we should still execute _postRun() so people
// have a chance to clean-up
//
// virtual
bool
Ookala::PluginChain::run()
{
    bool       okToRun = true;
    bool       ret     = true;
    std::vector<Plugin *>::iterator thePlugin;

    mCancelFlag        = false;

    setErrorString("");

    if (!mPluginChainData) return false;

    // Record the current time, to say that we've executid
    mChainMutex.lock();
    mLastExecutionTime = time(NULL);
    mChainMutex.unlock();


    // And, if we're a one-shot execution, we don't have
    // to fire any more.
    if (mPeriod < 0) mPeriod = 0;
    

    // Grab a copy of the chain, to protect from access funk.
    // We can't just lock access to mPluginChain, because when
    // we're run()'ing, we would need to lock against changes
    // and also need to lock while query()'ing. However, chances
    // are that the running plugins will be querying.
    //
    // Making a copy probably isn't quite the right behavior,
    // but it doesn't break things badly and makes life much
    // easier.
    mChainMutex.lock();

    std::vector<Plugin *>theChain = mPluginChainData->mPluginChain;

    mChainMutex.unlock();

    

    if (theChain.empty()) {
        setErrorString("Empty plugin chain.");
        return false;
    }


    for (thePlugin = theChain.begin();
            thePlugin != theChain.end(); ++thePlugin) {

        printf("preRun %s\n", (*thePlugin)->name().c_str());
        if (((*thePlugin)->preRun(this) == false) && (wasCancelled() == false)) {
            okToRun      = false;
            ret          = false;
            setErrorString((*thePlugin)->errorString());
            break;
        }
    }

    if (wasCancelled()) {
        okToRun = false;
    }

    if (okToRun) {
        for (thePlugin = theChain.begin();
                thePlugin != theChain.end(); ++thePlugin) {
            printf("run %s\n", (*thePlugin)->name().c_str());

            if (((*thePlugin)->run(this) == false) && (wasCancelled() == false)) {
                ret          = false;
                setErrorString((*thePlugin)->errorString());
                break;
            }

            if (wasCancelled()) break;
        }
    }
    
    for (thePlugin = theChain.begin();
            thePlugin != theChain.end(); ++thePlugin) {
        printf("postRun %s\n", (*thePlugin)->name().c_str());
        if (((*thePlugin)->postRun(this) == false) && (wasCancelled() == false)) {
            ret          = false;
            setErrorString((*thePlugin)->errorString());
            break;
        }
    }
    
    if (wasCancelled()) {
        setErrorString("Plugin chain cancelled.");
        return false;
    }

    return ret;
}


// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::clear()
{
    if (!mPluginChainData) return false;

    mChainMutex.lock();

    mPluginChainData->mPluginChain.clear();

    mChainMutex.unlock();

    return true;
}

// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::append(Plugin *plugin)
{    
    if (!mPluginChainData) return false;

    if (plugin == NULL) {
        return false;
    }

    mChainMutex.lock();

    mPluginChainData->mPluginChain.push_back(plugin);

    mChainMutex.unlock();

    return true;
}

// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::cancel()
{
    mCancelMutex.lock();

    mCancelFlag = true;

    mCancelMutex.unlock();

    setUiString("Ui::status_string_minor", "Cancelling execution, please wait.");
    return true;
}

// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::wasCancelled()
{
    bool ret = false;

    mCancelMutex.lock();

    ret = mCancelFlag;

    mCancelMutex.unlock();

    return ret;
}

// -----------------------------------------
//
// Search for a plugin based on its name. If we don't find
// anything, return an empty vector.

std::vector<Ookala::Plugin *>
Ookala::PluginChain::queryByName(const std::string name)
{
    std::vector<Plugin *>matches;
    std::string goal(name);

    if (!mPluginChainData) return matches;

    mChainMutex.lock();

    for (std::vector<Plugin *>::iterator i = 
                 mPluginChainData->mPluginChain.begin();
            i != mPluginChainData->mPluginChain.end(); ++i) {

        if ((*i)->matchName(goal)) {
            matches.push_back(*i);
        }        
    }

    mChainMutex.unlock();

    return matches;
}


// -----------------------------------------
//
// Search for a plugin that has a given set of attributes.
// In order for a plugin to qualify, it should match against
// all supplied attributes.
// If nothing is found, an empty vector is returned.

std::vector<Ookala::Plugin *>
Ookala::PluginChain::queryByAttribute(const std::string attrib)
{
    std::vector<Plugin *>matches;
    std::string goal(attrib);

    if (!mPluginChainData) return matches;

    mChainMutex.lock();

    for (std::vector<Plugin *>::iterator i = 
                 mPluginChainData->mPluginChain.begin();
            i != mPluginChainData->mPluginChain.end(); ++i) {

        if ((*i)->matchAttribute(goal)) {
            matches.push_back(*i);
        }        
    }

    mChainMutex.unlock();

    return matches;
}

// -----------------------------------------
//
// Search for a plugin that has a given set of attributes.
// In order for a plugin to qualify, it should match against
// all supplied attributes.
// If nothing is found, an empty vector is returned.

std::vector<Ookala::Plugin *>
Ookala::PluginChain::queryByAttributes(std::vector<std::string> attribs)
{
    std::vector<Plugin *>matches;

    if (!mPluginChainData) return matches;

    mChainMutex.lock();

    for (std::vector<Plugin *>::iterator i = 
                 mPluginChainData->mPluginChain.begin();
            i != mPluginChainData->mPluginChain.end(); ++i) {

        if ((*i)->matchAttributes(attribs)) {
            matches.push_back(*i);
        }        
    }

    mChainMutex.unlock();

    return matches;
}

// -----------------------------------------
//
// virtual
int32_t
Ookala::PluginChain::getPeriod()
{    
    return mPeriod;
}

// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::setPeriod(int32_t period)
{
    mPeriod = period;

    return true;
}

// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::needsPeriodicExecution()
{
    // If we're a one-shot, go for it.
    if (mPeriod < 0) return true;

    // Otherwise, if we're a 0-shot, no go
    if (mPeriod == 0) return false;

    mChainMutex.lock();

    int32_t elapsedSec = time(NULL) - mLastExecutionTime;

    mChainMutex.unlock();

    if (elapsedSec> mPeriod) {
        return true;
    }

    return false;
}


// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::getHidden()
{
    return mHidden;
}

// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::setHidden(bool hidden)
{
    mHidden = hidden;
    return true;
}

// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    // XXX: TBD.

    return false;
}

// -----------------------------------------
//
// virtual
bool
Ookala::PluginChain::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    xmlNodePtr            child = NULL;
    std::string           chainName;
    std::string           dictName;
    int32_t               period    = 0;
    bool                  chainOk   = true;
    std::vector<Plugin *> plugins;
    DictHash             *hash    = NULL;

    if (!mPluginChainData) return false;
    if (!mRegistry)        return false;
        
    plugins = mRegistry->queryByName("DictHash");
    if (plugins.empty()) {
        return false;
    }

    hash = dynamic_cast<DictHash *>(plugins[0]);
    if (!hash) {
        return false;
    }


    if (root == NULL) return false;

    std::string rootName((char *)root->name);
    
    if (rootName != "chain") {
        return false;
    }

    printf("Loading chain...\n");

    xmlChar *attrName = xmlGetProp(root, (const xmlChar *)("name"));
    if (attrName != NULL) {
        chainName = (char *)(attrName);
        xmlFree(attrName);
    }

    attrName = xmlGetProp(root, (const xmlChar *)("period"));
    if (attrName != NULL) {
        period = atoi( (char *)attrName);
        xmlFree(attrName);
    } else {
        period = 0;
    }

    attrName = xmlGetProp(root, (const xmlChar *)("hidden"));
    if (attrName != NULL) {
        std::string hidden = (char *)attrName;
        xmlFree(attrName);

        size_t start = hidden.find_first_not_of(" \t\n");
        size_t end   = hidden.find_last_not_of(" \t\n");

        if (start == std::string::npos) {
            hidden = "";
        } else {
            hidden = hidden.substr(start, end-start+1);
        }
        std::transform(hidden.begin(), 
                       hidden.end(), 
                       hidden.begin(), 
#ifdef _WIN32
                       tolower);
#else
                       (int(*)(int))std::tolower);
#endif

        if ((hidden == "yes") || (hidden == "true")) {
            setHidden(true);
        } else {
            setHidden(false);
        }
    } else {
        setHidden(false);
    }


    printf("\t   Name: %s, period: %d\n", chainName.c_str(), period);

    setName(chainName);
    setPeriod(period);

    // Walk all the chain children. We only really care about:
    //    - Dict nodes
    //    - Plugin nodes
    //
    // and we don't care about recursing.
    child = root->xmlChildrenNode;
    while (child != NULL) {
        std::string childName((char *)(child->name));

        if (childName == "dict") {
            dictName = "";
            
            attrName = xmlGetProp(child, (const xmlChar *)("name"));
            if (attrName != NULL) {
                dictName = (char *)(attrName);
                xmlFree(attrName);
            }

            printf("\t   Name: %s\n", dictName.c_str());

            Dict *dict = hash->newDict(dictName.c_str());
            if (!dict) {
                fprintf(stderr, "WARNING: Unable to create dict \"%s\"\n",
                                         dictName.c_str());
                return false;
            }

            dict->unserialize(doc, child);
    
            setDictName(dictName);
        }

        if (childName == "plugin") {
            std::string           pluginName;

            attrName = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
            if (attrName != NULL) {
                pluginName = (char *)attrName;            
                xmlFree(attrName);
            } else {
                chainOk = false;
            }
            
            if (chainOk) {
                printf("Found plugin; %s\n", pluginName.c_str());

                plugins = mRegistry->queryByName(pluginName);
                if (plugins.empty()) {
                    fprintf(stderr, "ERROR: Plugin %s not loaded..\n", pluginName.c_str());
                    chainOk = false;
                } else {
                    append(plugins[0]);
                }
            }
        }

        child = child->next;
    }

    return chainOk;
}

// -----------------------------------------
//
// virtual 
bool 
Ookala::PluginChain::setUiBool(const std::string &key, 
                               bool               value)
{
    Ui *ui;
    std::vector<Plugin *>plugins = queryByAttribute("Ui");
    
    if (plugins.empty()) {
        return false;
    }

    ui = dynamic_cast<Ui *>(plugins[0]);
    if (!ui) {
        return false;
    }

    return ui->setBool(key, value);
}

// -----------------------------------------
//
// virtual 
bool 
Ookala::PluginChain::setUiString(const std::string &key,  
                                 const std::string &value)
{
    Ui *ui;
    std::vector<Plugin *>plugins = queryByAttribute("Ui");
    
    if (plugins.empty()) {
        return false;
    }

    ui = dynamic_cast<Ui *>(plugins[0]);
    if (!ui) {
        return false;
    }

    return ui->setString(key, value);
}

// -----------------------------------------
//
// virtual 
bool 
Ookala::PluginChain::setUiInt(const std::string &key, 
                              int32_t            value)
{
    Ui *ui;
    std::vector<Plugin *>plugins = queryByAttribute("Ui");
    
    if (plugins.empty()) {
        return false;
    }

    ui = dynamic_cast<Ui *>(plugins[0]);
    if (!ui) {
        return false;
    }

    return ui->setInt(key, value);
}

// -----------------------------------------
//
// virtual 
bool 
Ookala::PluginChain::setUiDouble(const std::string &key, 
                                 double             value)
{
    Ui *ui;
    std::vector<Plugin *>plugins = queryByAttribute("Ui");
    
    if (plugins.empty()) {
        return false;
    }

    ui = dynamic_cast<Ui *>(plugins[0]);
    if (!ui) {
        return false;
    }

    return ui->setDouble(key, value);
}

// -----------------------------------------
//
// virtual 
bool 
Ookala::PluginChain::setUiRgb(const std::string &key, 
                              Rgb                value)
{
    Ui *ui;
    std::vector<Plugin *>plugins = queryByAttribute("Ui");
    
    if (plugins.empty()) {
        return false;
    }

    ui = dynamic_cast<Ui *>(plugins[0]);
    if (!ui) {
        return false;
    }

    return ui->setRgb(key, value);
}

// -----------------------------------------
//
// virtual 
bool 
Ookala::PluginChain::setUiYxy(const std::string &key, 
                              Yxy                value)
{
    Ui *ui;
    std::vector<Plugin *>plugins = queryByAttribute("Ui");
    
    if (plugins.empty()) {
        return false;
    }

    ui = dynamic_cast<Ui *>(plugins[0]);
    if (!ui) {
        return false;
    }

    return ui->setYxy(key, value);
}
