// --------------------------------------------------------------------------
// $Id: WxLutSweep.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <stdio.h>
#include <stdlib.h>

#include <vector>

#ifdef __linux__
#include <unistd.h>
#endif

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "PluginChain.h"
#include "PluginRegistry.h"
#include "WxLutSweep.h"

#include "plugins/WsLut/WsLut.h"

// ----------------------------------

BEGIN_PLUGIN_REGISTER(1)
    PLUGIN_REGISTER(0, Ookala::WxLutSweep)
END_PLUGIN_REGISTER

// ----------------------------------

namespace Ookala {

enum {
    SLIDER_GAMMA,
};

class GammaSweepDialog: public wxDialog
{
    public:
        GammaSweepDialog(WsLut *lut);
        virtual ~GammaSweepDialog() { }

    protected:        
        WsLut       *mLut;
        wxSlider    *mGammaSlider;

        void updateGamma(wxScrollEvent &event);

        DECLARE_EVENT_TABLE()
};

}; // namespace Ookala


// ==================================
//
// GammaSweepDialog
//
// ----------------------------------

Ookala::GammaSweepDialog::GammaSweepDialog(WsLut *lut):
    wxDialog((wxWindow *)NULL, (wxWindowID)-1, (const wxString)_T("Gamma Boost")),
    mLut(lut)
{
    wxSizer         *sizer = new wxBoxSizer(wxVERTICAL);

    mGammaSlider = new wxSlider(this, SLIDER_GAMMA, 0, -30, 30, 
                wxDefaultPosition, wxDefaultSize, 
                wxSL_HORIZONTAL);
    mGammaSlider->SetMinSize(wxSize(320, 40));

    sizer->Add(mGammaSlider,
                   wxSizerFlags().Align(wxEXPAND | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL));

    sizer->Add(
        CreateStdDialogButtonSizer(wxOK | wxCANCEL),
         0, wxALIGN_CENTER ); 

    SetSizer(sizer);
    sizer->SetSizeHints(this);
    sizer->Fit(this);
}

// ----------------------------------
//
void
Ookala::GammaSweepDialog::updateGamma(wxScrollEvent &event)
{
    double boost, val;

    if (mGammaSlider->GetValue() >= 0) {
        boost = 1. / (1. + (mGammaSlider->GetValue() * .1));
    } else {
        boost = 1. + fabs(mGammaSlider->GetValue() * .1);
    }

    printf("Blotto! %d -> %f\n", 
            mGammaSlider->GetValue(), boost);

    std::vector<double> newLut;
    
    for (uint32_t i=0; i<mLut->getSize(); ++i) {
        val = static_cast<double>(i) / static_cast<double>(mLut->getSize()-1);

        newLut.push_back( pow(val, boost) );
    }
    mLut->set(newLut, newLut, newLut);
}

// ----------------------------------

BEGIN_EVENT_TABLE(Ookala::GammaSweepDialog, wxDialog)
    EVT_COMMAND_SCROLL_THUMBRELEASE(SLIDER_GAMMA, GammaSweepDialog::updateGamma)
    EVT_COMMAND_SCROLL_LINEUP(      SLIDER_GAMMA, GammaSweepDialog::updateGamma)
    EVT_COMMAND_SCROLL_LINEDOWN(    SLIDER_GAMMA, GammaSweepDialog::updateGamma)
    EVT_COMMAND_SCROLL_PAGEUP(      SLIDER_GAMMA, GammaSweepDialog::updateGamma)
    EVT_COMMAND_SCROLL_PAGEDOWN(    SLIDER_GAMMA, GammaSweepDialog::updateGamma)
    EVT_COMMAND_SCROLL         (    SLIDER_GAMMA, GammaSweepDialog::updateGamma)

END_EVENT_TABLE()


// ==================================
//
// WxLutSweep
//
// ----------------------------------

Ookala::WxLutSweep::WxLutSweep():
  Plugin()
{
    setName("WxLutSweep");
}

// ----------------------------------
//
Ookala::WxLutSweep::WxLutSweep(const WxLutSweep &src):
    Plugin(src)
{
}

// ----------------------------------
//

Ookala::WxLutSweep::~WxLutSweep()
{

}

// ----------------------------
//
Ookala::WxLutSweep &
Ookala::WxLutSweep::operator=(const WxLutSweep &src)
{
    if (this != &src) {
        Plugin::operator=(src);
    }

    return *this;
}


// ----------------------------------
//
// virtual protected
bool
Ookala::WxLutSweep::_checkDeps()
{
    setErrorString("");

    if (!mRegistry) {
        setErrorString("No Registry found for WxLutSweep.");

        return false;
    }

    std::vector<Plugin *>plugins = mRegistry->queryByName("WsLut");

    if (plugins.empty()) {
        setErrorString("No WsLut plugin found for WxLutSweep.");
        return false;
    }

    return true;
}

// ----------------------------------
//
// virtual protected
bool
Ookala::WxLutSweep::_run(PluginChain *chain)
{
printf("WxLutSweep::_run()\n");
    std::vector<uint32_t> origRedLut, origGreenLut, origBlueLut;

    setErrorString("");

    if (mRegistry == NULL) {
        setErrorString("Plugin called with no registry.");
        return false;
    }

    std::vector<Plugin *> plugins = mRegistry->queryByName("WsLut");
    if (plugins.empty()) {
        setErrorString("No WsLut found.");
        return false; 
    }

    WsLut *lut = dynamic_cast<WsLut *>(plugins[0]);
    if (!lut) {
        setErrorString("No WsLut found.");
        return false; 
    }

    // Save the initial Lut state, so we can restore on cancel
    if (!lut->get(origRedLut, origGreenLut, origBlueLut)) {
        setErrorString(lut->errorString());
        return false;
    }


    GammaSweepDialog dlg(lut);

    if (dlg.ShowModal() == wxID_CANCEL) {
        printf("Cancelled, restore original lut\n");

        if (!lut->set(origRedLut, origGreenLut, origBlueLut)) {
            setErrorString(lut->errorString());
            return false;
        }

        chain->cancel();
    }

    return true;
}


