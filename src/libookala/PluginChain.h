// --------------------------------------------------------------------------
// $Id: PluginChain.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef PLUGINEXECUTIONCHAIN_H_HAS_BEEN_INCLUDED
#define PLUGINEXECUTIONCHAIN_H_HAS_BEEN_INCLUDED

#include <vector>
#include <string>

// --------------------------------------------------------------------------

#include <libxml/xmlmemory.h>

  // xmlmemory.h pulls in "libxml/xmlversion.h" that redefines a conflicting
  // ATTRIBUTE_PRINTF in some versions - if so, remove conflict with 
  // wxWidgets needs (not pretty but can contribute to GCC compile errors 
  // elsewhere later)

#ifdef ATTRIBUTE_PRINTF
  #undef ATTRIBUTE_PRINTF
#endif
#ifdef ATTRIBUTE_UNUSED
  #undef ATTRIBUTE_UNUSED
#endif
// --------------------------------------------------------------------------

#include <libxml/parser.h>


#include "Types.h"
#include "Plugin.h"
#include "Mutex.h"

namespace Ookala {

class PluginRegistry;
class Dict;
class EXIMPORT PluginChain
{
    public:

        PluginChain(PluginRegistry *registry);  
        PluginChain(const PluginChain &src);

        virtual ~PluginChain();
        PluginChain & operator=(const PluginChain &src);

                                
        std::string errorString();
        void        setErrorString(const std::string &str);

        // Give the chain a name.        
        void              setName(const std::string name);
        const std::string name();
                           
        // Allow us to point find a dictionary to query for 
        // per-chain data, useful for setting calibration target 
        // values and inter-plugin communication.
        void        setDictName(const std::string dictName);        
        std::string getDictName();
        
        // As a convience, to down the verbage of dicthash queries.
        Dict *      getDict();
        
        
        // run() starts executing plugins, by calling their preRun()
        // run() and postRun() methods. It will also reset the 
        // cancelled flag at it's start.
        virtual bool run();
        
        // Remove all plugins in this chain. 
        virtual bool clear();
        
        // Add another plugin to the end of the chain.
        virtual bool append(Plugin *plugin);
        
        // Set the cancel flag to indicate we should stop    
        virtual bool cancel();
        
        // Processing plugins should test this periodically to see
        // if they have been cancelled
        virtual bool wasCancelled();
        
        // These are just like the functionality in PluginRegistry,
        // but the search over the plugins in the chain, instead of
        // over all loaded plugins.
 
        // Search for a plugin based on its name. If we don't find
        // anything, return an empty vector.
        std::vector<Plugin *> queryByName(const std::string name);
        
        // Search for a plugin that has a given set of attributes.
        // In order for a plugin to qualify, it should match against
        // all supplied attributes.
        // If nothing is found, an empty vector is returned.
        std::vector<Plugin *> queryByAttribute(const std::string attrib);
        
        std::vector<Plugin *> queryByAttributes(
                                     std::vector<std::string> attribs);
        
        // Periodic execution of chains. Some chains should run ever
        // once and a while, perhaps to check the current status
        // of something.
        //
        // Rather than make the user track the update time, we'll
        // track things internally. Then, the user just has
        // to test if the chain needs periodic execution, and run
        // it if execution is needed.
        //
        // We can extend this to encompass plugins that should
        // only be run once. Periods of < 0 indicate that the
        // chain only needs to be run once, on startup.
        //
        // So, when apps start up, they should test for 
        // 'initialization' plugins (and anyone else periodic)
        // by making a pass over all chains, running those
        // which indicate needsPeriodicExecution().
        //
        // A period of 0 means to ignore the period.
        //
        // Period values > 0 are measured in seconds.
        //
        virtual int32_t getPeriod();       
        virtual bool    setPeriod(int32_t period);       
        virtual bool    needsPeriodicExecution();
        
        // Chains can also have a 'hidden' attribute. Interpret
        // this as you will, but some apps may use this to hide
        // chains from menu's. Typically, these would be background
        // chains that are also periodic.
        virtual bool getHidden();
        virtual bool setHidden(bool hidden);

        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

        // Convience UI setting methods
        //
        // Often, running plugins will want to update some 
        // UI that might be present. Instead of putting the
        // onus on them to search the chain for a Ui plugin,
        // we'll replicate all the Ui methods here. Then,
        // we can handle the searching and make life easier
        // for plugin writers.

        virtual bool setUiBool(  const std::string &key, 
                                 bool               value);
        
        virtual bool setUiString(const std::string &key,  
                                 const std::string &value);
        
        virtual bool setUiInt(   const std::string &key, 
                                 int32_t            value);
        
        virtual bool setUiDouble(const std::string &key, 
                                 double             value);
        
        virtual bool setUiRgb(   const std::string &key, 
                                 Rgb                value);
        
        virtual bool setUiYxy   (const std::string &key, 
                                 Yxy                value);
        
    protected:        
        PluginRegistry       *mRegistry;

        bool                  mCancelFlag;

        time_t                mLastExecutionTime;
        int32_t               mPeriod;
        bool                  mHidden;

        Mutex                 mCancelMutex,
                              mChainMutex;

    private:
        struct _PluginChain {
            std::string           mName;
            std::vector<Plugin *> mPluginChain;

            std::string           mDictName;

            std::string           mErrorString;
        };

        _PluginChain         *mPluginChainData;


        PluginChain() {}
};

}; // namespace Ookala

#endif


