// --------------------------------------------------------------------------
// $Id: Plugin.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef PLUGIN_H_HAS_BEEN_INCLUDED
#define PLUGIN_H_HAS_BEEN_INCLUDED

#include <string.h>
#include <assert.h>

#include <string>
#include <vector>

#ifdef _WIN32
#pragma warning(disable: 4786)
#endif


#ifdef EXIMPORT
    #undef EXIMPORT
#endif

#ifdef _WIN32
#ifdef LIBOOKALA_DLL_EXPORTS
#define EXIMPORT _declspec(dllexport)
#else
#define EXIMPORT _declspec(dllimport)
#endif

#else
#define EXIMPORT
#endif

#include "Types.h"

namespace Ookala {

//
// Base class for dynamically loadable objects.
//

class PluginRegistry;
class Dict;
class PluginChain;
class CalibRecordDictItem;

class EXIMPORT Plugin
{
    public:
        Plugin();

        // Derived classes MUST implement a copy constructor, since
        // we're having to do nasty things to get around stl exporting
        // under windows. At the very least, you should make sure 
        // that the base class copy constructor gets hit.
        Plugin(const Plugin &src);

        virtual ~Plugin();

        Plugin & operator=(const Plugin &src);

        // Set a pointer back to the PluginRegistry, so we know
        // how to query for other plugins. This is mainly used
        // just by the PluginRegistry at registration time.
        virtual bool setPluginRegistry(PluginRegistry *data);
        virtual PluginRegistry *getPluginRegistry();

        // Get the name of the plugin
        virtual const std::string name() const;
       
        // Get the error string, which contains a description of the
        // last reported error.
        virtual const std::string errorString();

        // After being loaded, each node should be given a chance
        // to fail. This could happen if support for a particular 
        // device was loaded, but that device could not be found.
        //
        // NOTE: This function should _NOT_ rely on any other 
        // plugins (or request them from the PluginRegistry).
        // In fact, to enforce this, postLoad() will be called
        // prior to pointers to the PluginRegistry being handed out.
        //
        // postLoad() should only be called once for each plugin.
        //
        // If you're a derived class, you want to override _postLoad(),
        // and _NOT_ this method. This fellow is a wrapper around 
        // _postLoad().
        virtual bool postLoad();

        // After we have tested that plugins have passed the postLoad()
        // test, we should make sure that all their dependancies have
        // loaded successfully. In this function, plugins should request
        // everything they need from the PluginRegistry. If they don't find
        // something they need, they should fail. Also, for everything
        // they find, they should call checkDeps() on that plugin and
        // test for failure.
        //
        // Since checkDeps might occur recursivly, we shouldn't put in
        // anything that can't be run multiple times without badness.
        //
        // If you're a derived class, you want to override _checkDeps(),
        // and _NOT_ this method. This fellow is a wrapper around 
        // _checkDeps() that ensures things only happen once.
        virtual bool checkDeps();

        // After checking dependancies, allow plugins to have a final 
        // check of things. This can be used to break cyclical dependances
        // that might occur during checkDeps().
        //
        // As usual, the _real_ implementation is down in _postCheckDeps()
        virtual bool postCheckDeps();

        // Prior to running a chain, we should give nodes the opportunity
        // to do any per-run things. Here we might put things like 
        // showing a gui, connecting to gui signals, and so on.
        //
        // Like other things, this is a wrapper around _preRun()
        // below, so that's the one you'll want to override.
        //
        // Utility classes, like colorimeters or luts, probably won't
        // be added into the chain since they don't really have 
        // any meaningful run() functionality. So this is probably
        // not so useful for those classes (similarly with postRun())
        //
        // preRun() should alreays run, no matter what the cancelled state 
        // of the chain is.
        virtual bool preRun(PluginChain *chain);

        // Each node may have a point that does most of
        // the heavy lifting. For a calibration algorithm, 
        // run() should manage the loop of measuring and
        // adjusting. For things like colorimeters that don't
        // really do any one-shot actions, run() should 
        // do nothing (though other mechanisms higher up may 
        // be exposed).
        //
        // If you're implementing your own nodes, _run() is the 
        // method you want, not run().
        //
        // If something goes horribly ary during the course of
        // execution, run() (and _run(), acoordingly), should
        // return false.
        //
        // run() should periodically test the cancelled state of the
        // chain [chain->wasCancelled()] and stop running if things
        // were cancelled. 
        virtual bool run(PluginChain *chain);

        // After running a chain, we should give the nodes the opportunity
        // to do any shutdown that they might need.
        //
        // Like other things, this is a wrapper around _postRun()
        // below, so that's the one you'll want to override.
        //
        // postRun() should alreays run, no matter what the cancelled state 
        // of the chain is.

        virtual bool postRun(PluginChain *chain);

        // Check if the given goal string matches our given name
        bool matchName(std::string nameGoal);

        // Check if we have a single attribute
        bool matchAttribute(std::string attributeGoal);

        // Check if we have _all_ the listed attributes
        bool matchAttributes(std::vector<std::string> attributeGoals);


        // If we're a calibration sort of plugin, we should allow people
        // to verify that we're still in a calibrated state. This involves
        // taking back in a calibration record that we generated, and
        // checking out some fields against current device state.
        virtual bool checkCalibRecord(CalibRecordDictItem *calibRec);

    protected:
        
        // State to enforce that each node is only checkDeps()'ed once
        bool mHasCheckedDeps;
        bool mCheckDepReturn;

        // Raw pointer to the PluginRegister. This will be NULL
        // during postLoad(), but should be valid for checkDeps()
        // and _run().
        PluginRegistry *mRegistry;

       // Each node type should have a unique name, so
        // others can request a specific node by name.
        void         setName(const std::string &name);
        
        // Error string - holds an error message from the last thing
        // that went horribly wrong in this plugin.
        void         setErrorString(const std::string &errorString);

        // Add an attribute to our list
        //
        // Each node should also specify a list of 
        // attributes that is provides. This way, if other 
        // node don't need a particular node, but rather one
        // of a given class, the other nodes can make requests
        // based on classes.
        //
        // A class might be "UI", "ddc/ci", "CRT Colorimeter",
        // or "LED LCD Colorimeter".         
        void         addAttribute(std::string attrib);

        // The real implementation that is called by postLoad().
        // NOTE: If you're extending the plugin class, _this_ is the
        // version you want to override.
        virtual bool _postLoad();

        // The real implementation that is called by checkDeps().
        // See the above note on checkDeps() to see what the purpose
        // of the function is. 
        // NOTE: If you're extending the plugin class, _this_ is the
        // version you want to override, _NOT_ checkDeps()
        virtual bool _checkDeps();

        // The real implementation of postCheckDeps()
        virtual bool _postCheckDeps();

        // The real implementation that is called by preRun() above.
        virtual bool _preRun(PluginChain *chain);

        // The real implementation that is called by run(); See
        // the above for run() to see what all the fuss is about
        virtual bool _run(PluginChain *chain);

        // The real implementation that is called by postRun() above.
        virtual bool _postRun(PluginChain *chain);

    private:
        struct _Plugin {
            std::string              mName;
            std::string              mErrorString;
            std::vector<std::string> mAttributes;       
        };

        _Plugin *mPluginData;

};

}; // namespace Ookala


// In order to support multiple plugins in one .so, things are
// slightly tricky with registration.
//
// For each plugin object, we'll need to define static member
// functions for creation and deletion from the other side
// of the .so-divide. Below is a helper macro, that we should
// use like:
//
//     class foo: public Plugin
//     {
//         foo();
//         virtual ~foo();
//
//         PLUGIN_ALLOC_FUNCS(foo)
//         ...
//     }
//
// PLUGIN_ALLOC_FUNCS should be used once on _every_ plugin
// class.
//
// To expose the allocation functions to the other side of the
// world, we'll have to pass back function pointers to the 
// allocation functions. This should be done once _PER SO_
// that we build, and should include an entry for each 
// plugin in the .so.
//
// For example, lets say we just have our one plugin "foo"
// in a given .so. Then, somewhere outside of a class, we
// need to include:
//
//    BEGIN_PLUGIN_REGISTER(1)
//       PLUGIN_REGISTER(0, foo)
//    END_PLUGIN_REGISTER
//
// If we had three plugins in the .so, we would instead use:
//
//    BEGIN_PLUGIN_REGISTER(3)
//       PLUGIN_REGISTER(0, larry)
//       PLUGIN_REGISTER(1, curly)
//       PLUGIN_REGISTER(2, shemp)
//    END_PLUGIN_REGISTER
//
//
// Next, it's possible to define new data types in plugins, and
// have them stored in dicts. This really only gets tricky at
// un-serialization time. The dict needs to search throught
// the list of plugins to see who is able to create each
// type of object that it might come across in a file.
//
// If you want to allow for exposing new data types, you can
// use this technique. It will subsume the functionaity of
// PLUGIN_ALLOC_FUNCS, so PLUGIN_ALLOC_FUNCS is not needed.
//
// The mechanism for this is a blend of PLUGIN_ALLOC_FUNCS
// and *_PLUGIN_REGISTER. If this behavior is desired, first
// derive a class from DictItem in your plugin. And, remember
// to hang onto its name [itemType()]. Next, setup the plugin:
//
//     class MyDictItem: public DictItem
//     {
//        ...
//     };
//
//     class foo: public Plugin
//     {
//         foo();
//         virtual ~foo();
//
//         // NOTE: NO USE OF PLUGIN_ALLOC_FUNCS!
//
//         BEGIN_DICTITEM_ALLOC_FUNCS(foo)
//             DICTITEM_ALLOC("cool_item", MyDictItem)
//         END_DICTITEM_ALLOC_FUNCS
//         BEGIN_DICTITEM_DELETE_FUNCS
//             DICTITEM_DELETE("cool_item")
//         END_DICTITEM_DELETE_FUNCS

//     }
//
// Here, "cool_item" is the string returned by MyDictItem::itemType()
//
// This should allow unserilization of plugin-defined data types
// by anyone.



#define PLUGIN_ALLOC_FUNCS(TYPE)                                \
           static Ookala::Plugin * create() {                   \
               return static_cast<Ookala::Plugin *>(new TYPE);  \
           }                                                    \
           static void destroy(Ookala::Plugin *data) {          \
               delete data;                                     \
           }                                                    \
           static Ookala::DictItem * dictitem_create(           \
                           const char *desiredItemName) {       \
               return NULL;                                     \
           }                                                    \
           static int dictitem_destroy(Ookala::DictItem *item) {\
               return -1;                                       \
           } 


// --------------------------------------------------

#ifdef _WIN32

#define BEGIN_PLUGIN_REGISTER(NUM)                                                \
            extern "C" __declspec(dllexport) Ookala::PluginAllocInfo *            \
                                                pluginRegister(int *numPlugins) { \
                *numPlugins = NUM;                                                \
                Ookala::PluginAllocInfo *data = new Ookala::PluginAllocInfo[NUM];   
#else

#define BEGIN_PLUGIN_REGISTER(NUM)                                                \
            extern "C" Ookala::PluginAllocInfo * pluginRegister(int *numPlugins) { \
                *numPlugins = NUM;                                                \
                Ookala::PluginAllocInfo *data = new Ookala::PluginAllocInfo[NUM];   
#endif




#define PLUGIN_REGISTER(IDX, TYPE)                                    \
                assert(IDX >= 0);                                     \
                assert(IDX < *numPlugins);                            \
                data[IDX].majorVersion = 1;                           \
                data[IDX].minorVersion = 0;                           \
                data[IDX].createFunc = TYPE::create;                  \
                data[IDX].deleteFunc = TYPE::destroy;                 \
                data[IDX].dictItemCreateFunc = TYPE::dictitem_create; \
                data[IDX].dictItemDeleteFunc = TYPE::dictitem_destroy;\
                data[IDX].name.assign(#TYPE);   


#define END_PLUGIN_REGISTER                         \
                return data;                        \
            }

// --------------------------------------------------

#define BEGIN_DICTITEM_ALLOC_FUNCS(OBJTYPE)                       \
           static Ookala::Plugin * create() {                     \
               return static_cast<Ookala::Plugin *>(new OBJTYPE); \
           }                                                      \
           static void destroy(Ookala::Plugin *data) {            \
               delete data;                                       \
           }                                                      \
                                                                  \
           static Ookala::DictItem * dictitem_create(             \
                           const char *desiredItemName) {   


#define DICTITEM_ALLOC(ITEMNAME, ITEMTYPE)                          \
           if (!strcmp(ITEMNAME, desiredItemName)) {                \
               return static_cast<Ookala::DictItem *>(new ITEMTYPE);\
           }

#define END_DICTITEM_ALLOC_FUNCS                          \
               return NULL;                               \
           }                                              

#define BEGIN_DICTITEM_DELETE_FUNCS                              \
           static int dictitem_destroy(Ookala::DictItem *data) { \

#define DICTITEM_DELETE(ITEMNAME)                                 \
               if (!strcmp(ITEMNAME, data->itemType().c_str())) { \
                   delete data;                                   \
                   return 0;                                      \
               }
                
#define END_DICTITEM_DELETE_FUNCS                         \
               return -1;                                 \
           }



#endif

