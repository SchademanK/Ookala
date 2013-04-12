// --------------------------------------------------------------------------
// $Id: DictHash.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DICTHASH_H_HAS_BEEN_INCLUDED
#define DICTHASH_H_HAS_BEEN_INCLUDED

#include <string>
#include <map>
#include <vector>

#include "Types.h"
#include "Plugin.h"
#include "Dict.h"

// Setup one plugin type for storing all our dictionary data.
// This is a little easier, and cleaner, to do as a builtin
// plugin. Because we allow for plugins to add in new DictItem
// classes, ever Dict should know about the plugin registery.
//
// If we just add a pointer to the plugin registry to every 
// Dict, we could be asking for trouble. There's no way to ensure
// that the plugin registry has not been freed without the Dict's
// knowledge [That's not entirely true - we could put together
// a mess of smart pointers, but, it's not so clean.]. 
//
// Alternativly, if we always keep our Dict's in a plugin, we'll
// be assured that they'll always have access to the plugin 
// registry.
//
// Finally, there's some gnarly issues with storing Dict's all over
// the place, and how modules get inter-dependnent. It's much simpler
// to just think of all the dict's as having a name/key, and storing
// them in a hash.
//

namespace Ookala {

class PluginChain;
class EXIMPORT DictHash: public Plugin
{
    public:
        DictHash();
        DictHash(const DictHash & src);
        virtual ~DictHash();
        DictHash & operator=(const DictHash &src);

        
        // Create a new dict of a given name. If we already have
        // someone of this name, don't blow it away, but let
        // us append data into in. For example, if we forget
        // to name multiple dict's, we don't want to loose data
        // via overwriting.
        Dict * newDict(std::string key);

        // Remove a dict from our storage.
        bool clearDict(std::string key);

        // Lookup a stored dict. Returns NULL if we don't have
        // anything stored by that name. You shouldn't hold
        // the ptr that is returned, but instead re-query
        // it every time you need to retrieve the reference.
        Dict * getDict(std::string key);

        // Return a vector of all the names of Dicts that we hold.
        std::vector<std::string> getDictNames();

    protected:  
        // Allow updating of Dict values by running the dict hash. 
        //
        // In the chain dict, we're going to look for:
        //
        //     DictHash::dict    --> The dict to be updated [STRING]
        //     DictHash::set     --> Keys of values to update [STRING ARRAY]
        //     
        //
        // So, for example, we might have:
        //
        //     Chain Dict["DictHash::dict"] = "Calibration Records"
        //     Chain Dict["DictHash::set"]  = { "Current Preset" }
        //     Chain Dict["Current Preset"] = "Target 0"
        //
        // will result in 
        //
        //     Calibration Records["Current Preset"] = "Target 0"
        //
        // being set.
        virtual bool _run(PluginChain *chain);    

    private:
        struct _DictHash {                        
            std::map<std::string, Dict> mDictData;
        };  
        
        _DictHash  *mDictHashData;
};

}; // namespace Ookala

#endif
