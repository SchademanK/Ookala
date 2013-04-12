// --------------------------------------------------------------------------
// $Id: Plugin.cpp 135 2008-12-19 00:49:58Z omcf $
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

#ifdef WIN32
#pragma warning(disable: 4786)
#endif

#include "Dict.h"
#include "Plugin.h"
#include "DataSavior.h"


// ====================================
//
// Plugin
//
// ------------------------------------


Ookala::Plugin::Plugin()
{
    mHasCheckedDeps = false;
    mCheckDepReturn = false;
    mRegistry       = NULL;
    

    mPluginData = new _Plugin;
}

// ------------------------------------
//
Ookala::Plugin::Plugin(const Plugin &src)
{
    mHasCheckedDeps = src.mHasCheckedDeps;
    mCheckDepReturn = src.mCheckDepReturn;
    mRegistry       = src.mRegistry;
   

    mPluginData = new _Plugin;  
    if (src.mPluginData) {
        mPluginData->mAttributes = src.mPluginData->mAttributes;
        mPluginData->mName       = src.mPluginData->mName;
        mPluginData->mErrorString= src.mPluginData->mErrorString;
    }       
}


// ------------------------------------
//
// virtual
Ookala::Plugin::~Plugin()
{ 
    if (mPluginData) {
        delete mPluginData;
        mPluginData = NULL;
    }
}

// ----------------------------
//
Ookala::Plugin &
Ookala::Plugin::operator=(const Plugin &src)
{
    if (this != &src) {
        if (mPluginData) {
            delete mPluginData;
            mPluginData = NULL;
        }

        if (src.mPluginData) {
            mPluginData  = new _Plugin();
            *mPluginData = *(src.mPluginData);
        }

        mHasCheckedDeps = src.mHasCheckedDeps;
        mCheckDepReturn = src.mCheckDepReturn;
    }

    return *this;
}


// ------------------------------------
//
// virtual 
bool
Ookala::Plugin::setPluginRegistry(PluginRegistry *data)
{
    mRegistry = data;

    return true;
}

// ------------------------------------
//
// virtual 
Ookala::PluginRegistry *
Ookala::Plugin::getPluginRegistry()
{
    return mRegistry;
}


// ------------------------------------
//
const std::string
Ookala::Plugin::name() const
{
    return mPluginData->mName;
}


// ------------------------------------
//
const std::string
Ookala::Plugin::errorString()
{
    return mPluginData->mErrorString;
}

// ------------------------------------
//
// virtual
bool
Ookala::Plugin::postLoad()
{
    return _postLoad();
}


// ------------------------------------
//
//
// This serves as a wrapper to _checkDeps() to make
// sure it only is hit once.
//
// virtual
bool
Ookala::Plugin::checkDeps()
{
    if (mHasCheckedDeps == false) {
        mCheckDepReturn = _checkDeps();
    }

    return mCheckDepReturn;
}

// ------------------------------------
//
// virtual
bool
Ookala::Plugin::postCheckDeps()
{
    return _postCheckDeps();
}


// ------------------------------------
//
// virtual

bool
Ookala::Plugin::preRun(PluginChain *chain)
{
    return _preRun(chain);
}


// ------------------------------------
//
// virtual 

bool 
Ookala::Plugin::run(PluginChain *chain)
{
    return _run(chain);
}


// ------------------------------------
//
// virtual

bool
Ookala::Plugin::postRun(PluginChain *chain)
{
    return _postRun(chain);
}


// ------------------------------------
//

bool
Ookala::Plugin::matchName(std::string nameGoal)
{
    if (nameGoal == mPluginData->mName) {
        return true;
    }
    return false;
}


// ------------------------------------
//

bool
Ookala::Plugin::matchAttribute(std::string attributeGoal)
{
    if (!mPluginData) {
        return false;
    }

    for (std::vector<std::string>::iterator theAttr = 
                       mPluginData->mAttributes.begin();
            theAttr != mPluginData->mAttributes.end(); ++theAttr) {
        if ((*theAttr) == attributeGoal) {
            return true;
        }
    }   

    return false;    
}

// ------------------------------------
//

bool
Ookala::Plugin::matchAttributes(std::vector<std::string> attributeGoals)
{
    if (!mPluginData) {
        return false;
    }
    if (attributeGoals.size() == 0) {
        return false;
    }
      
    for (std::vector<std::string>::iterator theAttr = 
                       mPluginData->mAttributes.begin();
            theAttr != mPluginData->mAttributes.end(); ++theAttr) { 
        bool found = false;

        for (std::vector<std::string>::iterator j = attributeGoals.begin();
                        j != attributeGoals.end(); ++j) {
            if ((*theAttr) == (*j)) {
                found = true;
            }                
        }

        if (found == false) {
            return false;
        }       
    }
   

    return true;
}

// ------------------------------------
//
// virtual

bool
Ookala::Plugin::checkCalibRecord(CalibRecordDictItem *calibRec)
{
    return false;
}



// ------------------------------------
//
// protected

void        
Ookala::Plugin::setName(const std::string &name)
{
    mPluginData->mName = name;
}

// ------------------------------------
//
// protected

void
Ookala::Plugin::setErrorString(const std::string &errorString)
{
    mPluginData->mErrorString = errorString;
}        

// ------------------------------------
//
// protected

void
Ookala::Plugin::addAttribute(std::string attrib)
{
    if (!mPluginData) {
        return;
    }

    mPluginData->mAttributes.push_back(attrib); 
}

// ------------------------------------
//
// virtual protected

bool
Ookala::Plugin::_postLoad()
{
    return true;
}

// ------------------------------------
//
// virtual protected

bool
Ookala::Plugin::_checkDeps()
{
    return true;
}

// ------------------------------------
//
// virtual protected

bool
Ookala::Plugin::_postCheckDeps()
{
    return true;
}

// ------------------------------------
//
// virtual protected

bool
Ookala::Plugin::_preRun(PluginChain *chain)
{
    return true;
}

// ------------------------------------
//
// virtual protected

bool
Ookala::Plugin::_run(PluginChain *chain)
{
    return true;
}


// ------------------------------------
//
// virtual protected

bool
Ookala::Plugin::_postRun(PluginChain *chain)
{
    return true;
}

