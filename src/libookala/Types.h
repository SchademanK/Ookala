// --------------------------------------------------------------------------
// $Id: Types.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef TYPES_H_HAS_BEEN_INCLUDED
#define TYPES_H_HAS_BEEN_INCLUDED

// Until windows gets C99 complient.
#ifdef _WIN32
#include <windows.h>
typedef          __int8  int8_t;
typedef          __int16 int16_t;
typedef          __int32 int32_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#endif

#include "Plugin.h"

namespace Ookala {

//
// Misc shared types
//
class DictItem;
class Plugin;

// Convenience function pointers for creation and deletion
typedef Plugin *(*CreateFunc)();
typedef void (*DeleteFunc)(Plugin *);


// DeleteFunc returns 0 if we deleted the object, -1  otherwise
typedef DictItem *(*DictItemCreateFunc)(const char *);
typedef int (*DictItemDeleteFunc)(DictItem *);

// Struct for registering function create/delete function
// pointers for dso's
struct PluginAllocInfo {
    std::string        name;
    uint32_t           majorVersion;
    uint32_t           minorVersion;
    CreateFunc         createFunc;
    DeleteFunc         deleteFunc;
    DictItemCreateFunc dictItemCreateFunc;
    DictItemDeleteFunc dictItemDeleteFunc;
};


// Handles to dynamic libs will change on a per-platform basis.
#ifdef __linux__
typedef void * LibHandle;
#else

#ifdef _WIN32
typedef HMODULE LibHandle;
#else
#error "Undefined dynamic lib handle in Types.h"
#endif


#endif



// Convenience function pointer for registration
typedef struct PluginAllocInfo * (*RegisterFunc)(int *);

struct PluginInfo {
    std::string         dsoName,
                        pluginName;
    bool                registered;
    bool                builtinPlugin;         // True if plugin is not 
                                               // from a dso
    Plugin             *plugin;

    CreateFunc          createFunc;            // These will be NULL
    DeleteFunc          deleteFunc;            // for builtin plugins
    DictItemCreateFunc  dictItemCreateFunc;    //   ""          ""
    DictItemDeleteFunc  dictItemDeleteFunc;    //   ""          ""
    LibHandle           handle;                //   ""          ""
};


struct Yxy {
    double Y;
    double x;   
    double y;
};

struct XYZ {
    double X;
    double Y;
    double Z;
};

struct Lab {
    double L;
    double a;
    double b;
};

struct Rgb {
    double r;
    double g;
    double b;
};

struct Mat33 {
    double m00, m01, m02;
    double m10, m11, m12;
    double m20, m21, m22;
};

struct Vec3 {
    double v0, v1, v2;
};

struct Npm
{
    Mat33 rgbToXYZ;
    Mat33 invRgbToXYZ;
};

}; // namespace Ookala


#endif


