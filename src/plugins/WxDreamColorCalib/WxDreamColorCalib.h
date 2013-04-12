// --------------------------------------------------------------------------
// $Id: WxDreamColorCalib.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef WXDREAMCOLORCALIB_H_HAS_BEEN_INCLUDED
#define WXDREAMCOLORCALIB_H_HAS_BEEN_INCLUDED

#include "Plugin.h"

namespace Ookala {

class WxDreamColorCalib: public Plugin
{
    public:
        WxDreamColorCalib();
        WxDreamColorCalib(const WxDreamColorCalib &src);
        virtual ~WxDreamColorCalib();
        WxDreamColorCalib & operator=(const WxDreamColorCalib &src);


        PLUGIN_ALLOC_FUNCS(WxDreamColorCalib);

    protected:
        
        virtual bool _checkDeps();

        virtual bool _run(PluginChain *chain);
};

}; // namespace Ookala
#endif
