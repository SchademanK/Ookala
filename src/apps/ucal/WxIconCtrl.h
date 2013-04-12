// --------------------------------------------------------------------------
// $Id: WxIconCtrl.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef WXICONCTRL_H_HAS_BEEN_INCLUDED
#define WXICONCTRL_H_HAS_BEEN_INCLUDED

#include <map>

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"   
#endif

#include "wx/taskbar.h"

#include "Plugin.h"

class WxIconCtrl: public Ookala::Plugin
{
    public:
        WxIconCtrl();
        WxIconCtrl(const WxIconCtrl &src);
        virtual ~WxIconCtrl();
        WxIconCtrl & operator=(const WxIconCtrl &src);


        // This allows us to set a pointer to the WxTaskBarIcon
        // control that we should update icons for.
        bool setTaskbarIcon(wxTaskBarIcon *taskbarIcon);

        // This is what you probably want for changing the icons.
        bool setIcon(const std::string &iconKey);
        
    protected:
        wxTaskBarIcon                *mTaskbarIcon;

        std::map<std::string, wxIcon> mIcons;

        // This is currently looking for, in the chain dict:
        //
        //    WxIconCtrl::set  -> name of icon to set [STRING]
        //    WxIconCtrl::load -> keys to filename entries in the same 
        //                        dict [STRING ARRAY].
        //
        //
        // For loading, we might setup something like:
        //       Chain Dict["WxIconCtrl::load"] = {"Icon0_good", "Icon0_bad"}
        //       Chain Dict["Icon0_good"]       = "/usr/share/icons/foo_good.xpm" 
        //       Chain Dict["Icon0_bad"]        = "/usr/share/icons/foo_bad.xpm" 
        //
        // Then, we can later do things like:
        //
        //       Chain Dict["WxIconCtrl::set"]    = "Icon0_good"
        //
        // At this point, we can only load in xpm files for icons.

        virtual bool _run(Ookala::PluginChain *chain);

        // Pull icon filenames from the chain dict and load them.
        virtual bool loadIcons(Ookala::Dict *chainDict);
};


#endif

