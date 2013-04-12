// --------------------------------------------------------------------------
// $Id: DictHash.cpp 135 2008-12-19 00:49:58Z omcf $
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
#include <vector>
#include <string>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


#include "DictHash.h"
#include "PluginChain.h"
#include "PluginRegistry.h"

// ======================================
//
// DictHash
//
// --------------------------------------

Ookala::DictHash::DictHash():
    Plugin()
{        
    setName("DictHash");

    mDictHashData = new _DictHash();
}

// --------------------------------------
//
Ookala::DictHash::DictHash(const DictHash &src):
    Plugin(src)
{
    mDictHashData = new _DictHash();

    if (src.mDictHashData) {
        mDictHashData->mDictData = src.mDictHashData->mDictData;
    }
}

// --------------------------------------
//
Ookala::DictHash::~DictHash()
{
    if (mDictHashData) {
        delete mDictHashData;
        mDictHashData = NULL;
    }
}

// ----------------------------
//
Ookala::DictHash &
Ookala::DictHash::operator=(const DictHash &src)
{
    if (this != &src) {
        Plugin::operator=(src);

        if (mDictHashData) {
            delete mDictHashData;
            mDictHashData = NULL;
        }

        if (src.mDictHashData) {
            mDictHashData  = new _DictHash();
            *mDictHashData = *(src.mDictHashData);
        }
    }

    return *this;
}


// --------------------------------------
//
Ookala::Dict *
Ookala::DictHash::newDict(std::string key)
{
    Dict *oldDict;

    if (!mDictHashData) return NULL;

    // First, test if we already have something by this name.
    // If so, blow it away.
    oldDict = getDict(key);
    if (oldDict) {
        return oldDict;
    }

    Dict theNewDict(mRegistry);
        
    mDictHashData->mDictData.insert( std::pair< std::string, Dict >(key, theNewDict))  ;

    theNewDict.setName(key);

    return getDict(key);
}

// --------------------------------------
//
bool
Ookala::DictHash::clearDict(std::string key)
{
    std::map<std::string, Dict>::iterator theIter;

    if (!mDictHashData) return false;

    theIter = mDictHashData->mDictData.find(key);
    if (theIter == mDictHashData->mDictData.end()) {
        return false;
    }
    
    mDictHashData->mDictData.erase(theIter);

    return true;
}

// --------------------------------------
//
Ookala::Dict *
Ookala::DictHash::getDict(std::string key)
{
    std::map<std::string, Dict>::iterator theIter;

    if (!mDictHashData) return NULL;

    theIter = mDictHashData->mDictData.find(key);
    if (theIter == mDictHashData->mDictData.end()) {
        return NULL;
    }

    (*theIter).second.setName(key);

    return &((*theIter).second);
}

// --------------------------------------
//
std::vector<std::string>
Ookala::DictHash::getDictNames()
{
    std::vector<std::string> names;

    if (!mDictHashData) return names;

    for (std::map<std::string, Dict>::iterator theIter = 
                       mDictHashData->mDictData.begin();
            theIter != mDictHashData->mDictData.end(); ++theIter) {
        names.push_back( (*theIter).first );
    }

    return names;
}

// --------------------------------------
//
// In the chain dict, we're going to look for:
//
//     DictHash::dict    --> The dict to be updated [STRING]
//     DictHash::set     --> Keys of values to update [STRING ARRAY]
//     
bool
Ookala::DictHash::_run(PluginChain *chain)
{
    Dict                *srcDict         = NULL;
    Dict                *dstDict         = NULL;
    DictItem            *item            = NULL;
    StringDictItem      *stringItem      = NULL;
    StringArrayDictItem *stringArrayItem = NULL;

    std::vector<std::string> keysToSet;

    if (!mDictHashData) return false;

    if (!chain) {
        setErrorString("No chain specified for DictHash::_run().");
        return false;
    }

    // First, find the source dict - which is the one specified by the chain.
    srcDict = getDict(chain->getDictName());
    if (!srcDict) {
        setErrorString( std::string("Chain dict ") + chain->getDictName() +
                            std::string(" does not exist."));        
        return false;
    }
    
    // Now, look for the dst dict - whose name is given by DictHash::dict
    // in the source dict.
    item = srcDict->get("DictHash::dict");
    if (!item) {
        setErrorString("No destination dict specified for DictHash::_run().");
        return false;
    }
    stringItem = dynamic_cast<StringDictItem *>(item);
    if (!stringItem) {
        setErrorString("No destination dict specified for DictHash::_run().");
        return false;
    }
    dstDict = newDict(stringItem->get());
    if (!dstDict) {
        setErrorString("Unable to get destination dict for DictHash::_run().");
        return false;
    }

    // Get the string array item named "DictHash::set" in the source dict
    // It contains the keys of things in the source dict that should be
    // copied into the destiation dict.
    item = srcDict->get("DictHash::set");
    if (!item) {
        setErrorString("Unable to get DictHash::set [String Array] for DictHash::_run().");
        return false;
    }
    stringArrayItem = dynamic_cast<StringArrayDictItem *>(item);
    if (!item) {
        setErrorString("DictHash::set is not of type [String Array] for DictHash::_run().");
        return false;
    }

    keysToSet = stringArrayItem->get();


    // Now that we have a list of keys, walk over all the keys and copy
    // data from the source dict to the destination dict.
    for (std::vector<std::string>::iterator theKey = keysToSet.begin();
                theKey != keysToSet.end(); ++theKey) {

        item = srcDict->get((*theKey));
        if (!item) {
            fprintf(stderr, "Warning: Can't copy key %s; Not found in source dict %s\n",
                                  (*theKey).c_str(), srcDict->getName().c_str());
            continue;
        }

        if (!dstDict->set((*theKey), item)) {
            fprintf(stderr, "Warning: Unable to set key %s in dest dict %s\n",
                                   (*theKey).c_str(), dstDict->getName().c_str());
        }
    }

    return true;
}

