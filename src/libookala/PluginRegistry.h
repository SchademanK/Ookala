// --------------------------------------------------------------------------
// $Id: PluginRegistry.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef PLUGINREGISTRY_H_HAS_BEEN_INCLUDED
#define PLUGINREGISTRY_H_HAS_BEEN_INCLUDED

#include <string>
#include <vector>

#include "Types.h"
#include "Plugin.h"
#include "Dict.h"

namespace Ookala {

//
// Registry class for holding all our dynamic loadable
// plugins. Also provides query ability for plugins
// to get access to others.
//
class EXIMPORT PluginRegistry
{
    public:
        PluginRegistry();    
        PluginRegistry(const PluginRegistry &src);
   
        virtual ~PluginRegistry();
        PluginRegistry & operator=(const PluginRegistry &src);

        // Load in a DSO that contains plugins. Will return false
        // if we coulnd't find any plugins in the DSO.
        // 
        virtual bool loadPlugin(const char *filename);
        
        // Load a plugin, optionally with dict item create and
        // delete, from within your code.
        //
        // This should be called something like:
        //    
        //   loadPlugin( new fooPlugin(), fooPlugin::createItem,
        //                                fooPlugin::deleteItem);
        //
        virtual bool loadPlugin(Plugin             *plugin, 
                                DictItemCreateFunc  dictItemCreate,
                                DictItemDeleteFunc  dictItemDelete);
        
        // Look through various places and load plugins that seem
        // to be valid.
        virtual bool registerPlugins();       

        // Search for a plugin with an exact match to the given
        // name. If nothing is found, return an empty vector.
        std::vector<Plugin *> queryByName(const std::string name);
        
        // Search for a plugin that has a given set of attributes.
        // If a plugin matches all specified attributes, it's counted
        // as a hit and returned. If nothing matches, we return
        // an empty vector.
        std::vector<Plugin *> queryByAttribute(const std::string attrib);
        
        std::vector<Plugin *> queryByAttributes(
                                  const std::vector<std::string> attribs);
        
        // Return a pointer to a particular plugin, by index. Returns
        // a vector of pointers, for consistancy with other query
        // methods.
        std::vector<Plugin *> queryByIndex(const int idx);
        
        // Return the number of loaded plugins
        const int numPlugins();
        
        // Walk our list of plugins and see if we're able to create 
        // and destroy various types of plugins. This allows for
        // plugins to instanciate new data types and have them
        // saved/loaded in the dict
        DictItem * createDictItem(const std::string typeName);
        
        bool       deleteDictItem(DictItem *item);
              

    private:
        struct _PluginRegistry {
            std::vector<struct PluginInfo> mPluginInfo;

            // Handles to libs that have have been opened and will 
            // eventually need to be closed.
            std::vector<LibHandle>mLibHandles;
        };

        _PluginRegistry *mPluginRegistryData;          
};

}; // namespace Ookala

#endif

