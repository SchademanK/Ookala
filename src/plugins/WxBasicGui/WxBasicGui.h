// --------------------------------------------------------------------------
// $Id: WxBasicGui.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef WXBASICGUI_H_HAS_BEEN_INCLUDED
#define WXBASICGUI_H_HAS_BEEN_INCLUDED

#include "Plugin.h"
#include "Ui.h"

namespace Ookala {

class WxBasicGuiDialog;
class GamutPlot;
class WxBasicGui: public Ui
{
    public:
        WxBasicGui();
        WxBasicGui(const WxBasicGui &src);
        virtual ~WxBasicGui() {}
        WxBasicGui & operator=(const WxBasicGui &src);

        PLUGIN_ALLOC_FUNCS(WxBasicGui)

        // These should be safe for hitting by any worker threads.
        virtual bool setString(const std::string &key, 
                               const std::string &value);     

        virtual bool setYxy(const std::string &key, 
                            Yxy                value);     

    protected:
        
        // Setup the gui prior to running the chain. These should 
        // be hit from the main thread.
        virtual bool _preRun(PluginChain *chain);

        // Tear-down the gui after we've finished running the chain.
        virtual bool _postRun(PluginChain *chain);

        WxBasicGuiDialog *mDialog;
        GamutPlot        *mGamutPlot;
};

}; // namespace Ookala 

#endif

