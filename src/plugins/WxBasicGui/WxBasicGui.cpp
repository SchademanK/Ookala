// --------------------------------------------------------------------------
// $Id: WxBasicGui.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include "wxUtils.h"

#include "Color.h"
#include "PluginChain.h"
#include "PluginRegistry.h"
#include "WxBasicGui.h"
#include "GamutPlot.h"

// Event processing is fun.
//
// There's two big bits of funk happening here. First, there is the
// posting of events to get the gui to do things. These come from the
// setBool/String/Int/...() methods. It's possible that these methods 
// are called from a worker thread, and not the main GUI thread.
//
// As such, we can't just update the values of the Wx items. Instead,
// we need to post events to the main thread to update things. 
//
// This is where the BasicGuiEvent pieces come in.
//
// NOTE: currently, this handling is not synchronous. That is, if you 
// call setString(...), it's not guaranteed that the item will be 
// updated when the call returns. This could be an issue if you are
// setting the background RGB to measure.
//
//
// The next bit of fun involves catching Escape. We want to cancel 
// execution if we hit Escape a bunch of times. However, the way
// that key events are handled in Wx makes this tricky. Key events
// are delivered to the window that they occurred in, adn are not 
// propogated up the hierarchy of windows.  Further, there is some
// weirdness with getting the key events to fire at all on frames.
//
// So, we've created a frame with key event handlers that we want
// to fire with our key handling logic.
//
// But, to even get the key events in the first place, we've added
// a window into the frame to get the event and forward them to
// the parent.
//
// A similar thing will have to be done with any future sub-windows
// that are defined.
//

// ----------------------------------

BEGIN_PLUGIN_REGISTER(1)
    PLUGIN_REGISTER(0, Ookala::WxBasicGui)
END_PLUGIN_REGISTER

// ----------------------------------

//
// Custom event-fu. Be waarrrry.
//

namespace Ookala {
    const wxEventType BasicGuiCommandEvent = wxNewEventType();

    class BasicGuiEvent: public wxCommandEvent
    {
        public:
            BasicGuiEvent(wxEventType commandType = BasicGuiCommandEvent, 
                          int         id          = 0):
                wxCommandEvent(commandType, id) {}

            BasicGuiEvent(const BasicGuiEvent &event):
                wxCommandEvent(event)
            {
                key      = event.key; 
                string   = event.string;
                colorYxy = event.colorYxy;            
            }

            wxEvent *Clone() const 
            { 
                return new BasicGuiEvent(*this); 
            }

            std::string key;
            std::string string;
            Yxy         colorYxy;
            
    };

    typedef void (wxEvtHandler::*BasicGuiEventFunction)(BasicGuiEvent &);
};

#define BasicGuiEventHandler(func)                                      \
    (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction) \
    wxStaticCastEvent(BasicGuiEventFunction, &func)
 
#define EVT_BASIC_GUI(id, fn)                                            \
    DECLARE_EVENT_TABLE_ENTRY( BasicGuiCommandEvent, id, wxID_ANY,      \
    (wxObjectEventFunction)(wxEventFunction)                         \
    (wxCommandEventFunction) wxStaticCastEvent(                      \
    BasicGuiEventFunction, &fn ), (wxObject*) NULL ),

// ----------------------------------

// Create a little class that can catch key events. It seems like
// they never end up on the Frame.

namespace Ookala {

class WxKeyWindow : public wxWindow
{
    public:
        WxKeyWindow(wxWindow *parent, const wxSize &size = wxDefaultSize):
            wxWindow(parent, wxID_ANY, wxDefaultPosition, size) {
                /*SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);*/
            }

    protected:
        void onChar(wxKeyEvent& event) { 

            if (event.GetKeyCode() == WXK_ESCAPE) {
                if (GetParent()) {
                    wxPostEvent(GetParent(), event);
                }
            } else {
                event.Skip();
            }
        }

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WxKeyWindow, wxWindow)
    EVT_CHAR(WxKeyWindow::onChar)
END_EVENT_TABLE()


// ----------------------------------

class WxBasicGuiDialog: public wxFrame
{
    public:
        WxBasicGuiDialog(PluginChain *chain, Color *color);
        virtual ~WxBasicGuiDialog() {}

        void     onCancel(wxCommandEvent &event);
        void     onValueUpdate(BasicGuiEvent &event);
        void     onResize(wxSizeEvent &event);

        void     onChar(wxKeyEvent &event);

    protected:
        PluginChain      *mChain;
        Color            *mColor;

        wxBoxSizer       *mMainHBox,
                         *mButtonVBox,
                         *mStatusVBox;

        wxStaticBoxSizer *mInfoVBox;

        wxFlexGridSizer  *mInfoGrid;

        // The window that sits inside of the frame, which catches
        // key press events and forwards' them up. There was some
        // issue with doing this directly on WxBasicGuiDialog, 
        // so we inserted another window instead.
        WxKeyWindow      *mKeyWindow;

        // These hold the labels and values, respectivly, for
        // Y:, x:, y:, and Temp:
        wxStaticText     *mInfoGridLabel[4];
        wxStaticText     *mInfoGridValue[4];

        // This holds the text at the upper-left side of the screen.
        wxStaticText     *mDisplayColorName;

        // The status bar at the bottom of the frame
        wxStatusBar      *mStatusBar;

        // Status messages.
        std::string       mStatusMajor, 
                          mStatusMinor;

        // Gamut plot to display various colorful plots
        GamutPlot        *mGamutPlot;

        void     setMeasureColor(const uint8_t red,
                        const uint8_t grn, const uint8_t blu);
        void     setForegroundColour(const uint8_t red, 
                        const uint8_t grn, const uint8_t blu);


        DECLARE_EVENT_TABLE()
};

}; // namespace Ookala


// ==================================
//
// WxBasicGuiDialog
//
// ----------------------------------

Ookala::WxBasicGuiDialog::WxBasicGuiDialog(PluginChain *chain, Color *color):
    wxFrame(NULL, -1, _T("Status"), wxDefaultPosition, wxSize(500, 500))
{    
    mChain    = chain;
    mColor    = color;

    /*SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);*/
    SetMinSize(wxSize(500, 500));


    mKeyWindow = new WxKeyWindow(this, wxSize(500, 500));
    mKeyWindow->SetMinSize(wxSize(500, 500));

    mKeyWindow->SetFocus();

    // Put the key window into a sizer on the frame
    wxBoxSizer *frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(mKeyWindow, 1, wxGROW, 0);
    frameSizer->SetMinSize(wxSize(500, 500));


    // Now, start setting up the sizers    
    wxBoxSizer *mainVBox = new wxBoxSizer(wxVERTICAL);
    mMainHBox = new wxBoxSizer(wxHORIZONTAL);


    // -----------------------------
    // Left side of the screen setup
    mStatusVBox = new wxBoxSizer(wxVERTICAL);

    // Add a static box for some info display
    mInfoVBox = new wxStaticBoxSizer(wxVERTICAL, mKeyWindow, _("Color Sensor Measurements"));
    
    mInfoGrid = new wxFlexGridSizer(2, 2, 50);         // 2 column grid



    mDisplayColorName = new wxStaticText(mKeyWindow, wxID_ANY, _T(""),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
    wxFont font = mDisplayColorName->GetFont();
    font.SetPointSize(24);
    mDisplayColorName->SetFont(font);
    mStatusVBox->Add(mDisplayColorName, 0, wxGROW, 2);


    mInfoVBox->Add(mInfoGrid,   0, wxGROW, 2);  // goes inside the text frame
    mStatusVBox->Add(mInfoVBox, 0, wxGROW, 2);  // Text frame goes inside left side
  

#if 1
    // Add a GL Control
    int glAttribs[] = {
        WX_GL_RGBA,
        WX_GL_DOUBLEBUFFER,
        WX_GL_MIN_RED,   8,
        WX_GL_MIN_GREEN, 8,
        WX_GL_MIN_BLUE,  8,
        0
    };

    mGamutPlot = new GamutPlot(mColor, mKeyWindow, wxID_ANY, wxDefaultPosition, 
                wxSize(300,300),
                wxFULL_REPAINT_ON_RESIZE, 
                _T("Gamut Plot"), glAttribs);

    mStatusVBox->Add(300, 30, 0);
    mStatusVBox->Add(mGamutPlot, 0, wxGROW, 2);
    
/*
    // Initially just set 709 primaries.
    Yxy test;
    test.Y = 1;     
    test.x = .64;
    test.y = .33;
    mGamutPlot->setTargetRed(test);
    test.x = .30;
    test.y = .60;
    mGamutPlot->setTargetGreen(test);
    test.x = .15;
    test.y = .06;
    mGamutPlot->setTargetBlue(test);
*/
#else
    mGamutPlot = NULL;
    mStatusVBox->AddSpacer(300);


#endif

    mStatusVBox->Layout();
    mMainHBox->Add(mStatusVBox, 0, wxGROW | wxALL | wxALIGN_TOP | wxALIGN_LEFT, 10);

    // -----------------------------
    // Center screen setup - create an empty box that can grow, so 
    // we can keep the right stuff on the right side, and the left
    // stuff on the left side.
    mMainHBox->Add(new wxBoxSizer(wxVERTICAL), 1, wxALIGN_CENTER, 10);

    // -----------------------------
    // Right side of the screen setup

    mButtonVBox = new wxBoxSizer(wxVERTICAL);

    mButtonVBox->Add(new wxButton(mKeyWindow, wxID_CANCEL, _("Cancel")),  0, wxALL, 2);

    mMainHBox->Add(mButtonVBox, 0, wxALL | wxALIGN_BOTTOM | wxALIGN_RIGHT, 10);

    mainVBox->Add(mMainHBox, 1, wxALL | wxEXPAND);

    // Don't shrink smaller than the window that we've requested to be made
    mainVBox->SetMinSize(wxSize(500, 500));
    mKeyWindow->SetSizer(mainVBox);
    mainVBox->SetSizeHints(mKeyWindow);

    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    mKeyWindow->SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    mInfoVBox->GetStaticBox()->SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);


    // Add the Yxy display
    mInfoGridLabel[0]  = new wxStaticText(mKeyWindow, wxID_ANY, _T("    Y:"),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    mInfoGridLabel[1]  = new wxStaticText(mKeyWindow, wxID_ANY, _T("    x:"),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    mInfoGridLabel[2]  = new wxStaticText(mKeyWindow, wxID_ANY, _T("    y:"),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    mInfoGridLabel[3]  = new wxStaticText(mKeyWindow, wxID_ANY, _T("    Temp:"),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);


    mInfoGridValue[0]  = new wxStaticText(mKeyWindow, wxID_ANY, _T("N/A"),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    mInfoGridValue[1]  = new wxStaticText(mKeyWindow, wxID_ANY, _T("N/A"),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    mInfoGridValue[2]  = new wxStaticText(mKeyWindow, wxID_ANY, _T("N/A"),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    mInfoGridValue[3]  = new wxStaticText(mKeyWindow, wxID_ANY, _T("N/A"),
                         wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);


    mInfoGrid->Add(mInfoGridLabel[0], 0, wxALIGN_RIGHT | wxLEFT,    30);
    mInfoGrid->Add(mInfoGridValue[0], 0, wxGROW | wxRIGHT | wxLEFT, 10);

    mInfoGrid->Add(mInfoGridLabel[1], 0, wxALIGN_RIGHT | wxLEFT,    30);
    mInfoGrid->Add(mInfoGridValue[1], 0, wxGROW | wxRIGHT | wxLEFT, 10);

    mInfoGrid->Add(mInfoGridLabel[2], 0, wxALIGN_RIGHT | wxLEFT,    30);
    mInfoGrid->Add(mInfoGridValue[2], 0, wxGROW | wxRIGHT | wxLEFT, 10);

    mInfoGrid->Add(mInfoGridLabel[3], 0, wxALIGN_RIGHT | wxLEFT,    30);
    mInfoGrid->Add(mInfoGridValue[3], 0, wxGROW | wxRIGHT | wxLEFT, 10);


    // Add in the status bar at the bottom
    mStatusBar = new wxStatusBar(mKeyWindow, wxID_ANY);
    mStatusBar->SetFieldsCount(1);
    mStatusBar->SetStatusText(_T(""));
    mainVBox->Add(mStatusBar, 0, wxGROW | wxALIGN_BOTTOM);

    mainVBox->Show(true);
    
    Maximize(true);

    SetSizer(frameSizer);
    frameSizer->Show(true);
   
    Layout();
    mKeyWindow->Layout();
    mStatusVBox->Layout();

    mKeyWindow->Show(true);
}

// --------------------------------------
// 
void
Ookala::WxBasicGuiDialog::onCancel(wxCommandEvent &event)
{
    if (mChain) {
        mChain->cancel();
    }

    mStatusBar->SetStatusText(_T("Canceling execution. Please wait."));
}

// --------------------------------------
// 
void 
Ookala::WxBasicGuiDialog::onValueUpdate(BasicGuiEvent &event)
{
    bool updateStatusBar = false;

    // Catch status_string_minor
    if (event.key == std::string("Ui::status_string_minor")) {
        mStatusMinor    = event.string;
        updateStatusBar = true;
    }
    
    // Catch status_string_major
    if (event.key == std::string("Ui::status_string_major")) {
        mStatusMajor    = event.string;
        updateStatusBar = true;
    }

    // Catch measured_Yxy - update the gamut plot and CCT field
    if (event.key == std::string("Ui::measured_Yxy")) {
        char buf[1024];

        sprintf(buf, "%.3f cd/m^2", event.colorYxy.Y);
        mInfoGridValue[0]->SetLabel(_U(buf));

        sprintf(buf, "%.4f", event.colorYxy.x);
        mInfoGridValue[1]->SetLabel(_U(buf));

        sprintf(buf, "%.4f", event.colorYxy.y);
        mInfoGridValue[2]->SetLabel(_U(buf));

        if (mGamutPlot) {
            mGamutPlot->setMeasurement(event.colorYxy);
        }

        if (mColor) {
            double cct = mColor->computeCct(event.colorYxy);

            if (cct < 0) {
                mInfoGridValue[3]->SetLabel(_U("Unknown"));
            } else {
                char buf[1024];
                
                sprintf(buf, "%5.0f K", cct);
                mInfoGridValue[3]->SetLabel(_U(buf));
            }
        }
    }

    // Catch target red/green/blue Yxy values
    if (event.key == std::string("Ui::target_red_Yxy")) {
        mGamutPlot->setTargetRed(event.colorYxy);
    }
    if (event.key == std::string("Ui::target_green_Yxy")) {
        mGamutPlot->setTargetGreen(event.colorYxy);
    }
    if (event.key == std::string("Ui::target_blue_Yxy")) {
        mGamutPlot->setTargetBlue(event.colorYxy);
    }
    if (event.key == std::string("Ui::target_white_Yxy")) {
        mGamutPlot->setTargetWhite(event.colorYxy);
    }




    if (updateStatusBar) {
		std::string msg;

        if (!mStatusMajor.empty()) {
            msg = mStatusMajor + ": ";
        } 
        msg += mStatusMinor;
		const wxString wxmsg = _S(msg);
		
        mStatusBar->SetStatusText(wxmsg, atoi(mStatusMajor.c_str()));
    }
}


// --------------------------------------
// 
void
Ookala::WxBasicGuiDialog::onResize(wxSizeEvent &event)
{
    Layout();

    if (mKeyWindow) {
        mKeyWindow->Layout();
    }

}

// --------------------------------------
// 
// NOTE: Key events are only delivered to the item that
// has focus. They aren't propagated up the chain. If 
// we need that behavior, we'll need to implement it ourselves.
//
// This means that if you're making sub-windows, they should
// catch their key down events and forwarded them up if they
// see an escape.
//
void
Ookala::WxBasicGuiDialog::onChar(wxKeyEvent &event)
{
    // If we see an "ESC" press, throw an 'cancel' event
    if (event.GetKeyCode() == WXK_ESCAPE) {
        wxCommandEvent cancelEvent(wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL);
        wxPostEvent(this, cancelEvent);
    } else {
        event.Skip();
    }
}


// --------------------------------------
// 
void 
Ookala::WxBasicGuiDialog::setMeasureColor(const uint8_t red,
                const uint8_t grn, const uint8_t blu)
{
    SetBackgroundColour( wxColour(red, grn, blu) );

    // For neutral colors, display the foreground in either white or
    // black. Otherwise, display the inverse of the background
    if ((red == grn) && (grn == blu)) {
        if (red < 176) {
            setForegroundColour( 255, 255, 255 );
        } else {
            setForegroundColour( 0, 0, 0 );
        }
    } else {
        setForegroundColour( 255-red, 255-grn, 255-blu );
    }
}

// --------------------------------------
// 
void 
Ookala::WxBasicGuiDialog::setForegroundColour(const uint8_t red, 
                      const uint8_t grn, const uint8_t blu)
{
    wxColor rgb(red, grn, blu);

    if (mInfoVBox) {
        if (mInfoVBox->GetStaticBox()) {
            mInfoVBox->GetStaticBox()->SetForegroundColour(rgb);
        }
    }

    for (int i=0; i<4; ++i) {
        mInfoGridLabel[i]->SetForegroundColour(rgb);
        mInfoGridValue[i]->SetForegroundColour(rgb);
    }
}

// --------------------------------------

BEGIN_EVENT_TABLE(Ookala::WxBasicGuiDialog, wxFrame)
    EVT_BUTTON(wxID_CANCEL,     WxBasicGuiDialog::onCancel)
    EVT_BASIC_GUI( wxID_ANY,    WxBasicGuiDialog::onValueUpdate)
    EVT_SIZE(                   WxBasicGuiDialog::onResize)
    EVT_CHAR(                   WxBasicGuiDialog::onChar)
END_EVENT_TABLE()





// ==================================
//
// WxBasicGui
//
// ----------------------------------

Ookala::WxBasicGui::WxBasicGui():
    Ui()
{
    setName("WxBasicGui");

    mDialog    = NULL;
    mGamutPlot = NULL;
}

// ----------------------------------

Ookala::WxBasicGui::WxBasicGui(const WxBasicGui &src):
    Ui(src)
{
    mDialog    = NULL;
    mGamutPlot = NULL;
}

// ----------------------------------
//
// virtual
bool 
Ookala::WxBasicGui::setString(const std::string &key, 
                              const std::string &value)
{
    if (mDialog == NULL) {
        return false;
    }

    // Send a custom event to the dialog, containing the
    // string that should be set.
    BasicGuiEvent event(BasicGuiCommandEvent);

    event.key    = key;
    event.string = value;

    wxPostEvent(mDialog, event);
 
    return true;
}

               
// ----------------------------
//
Ookala::WxBasicGui &
Ookala::WxBasicGui::operator=(const WxBasicGui &src)
{
    if (this != &src) {
        Ui::operator=(src);
    }

    return *this;
}

// ----------------------------------
//
// virtual
bool 
Ookala::WxBasicGui::setYxy(const std::string &key, 
                           Yxy                value)
{
    if (mDialog == NULL) {
        return false;
    }

    // Send a custom event to the dialog, containing the
    // string that should be set.
    BasicGuiEvent event(BasicGuiCommandEvent);

    event.key      = key;
    event.colorYxy = value;

    wxPostEvent(mDialog, event);
 
    return true;
}



// ----------------------------------
//
// virtual protected
bool
Ookala::WxBasicGui::_preRun(PluginChain *chain)
{
    Color *color;

    if (mRegistry == NULL) {
        setErrorString("No PluginRegistry for WxBasicGui.");
        return false;
    }

    std::vector<Plugin *>plugins = mRegistry->queryByName("Color");
    if (plugins.empty()) {
        setErrorString("No Color plugin for WxBasicGui.");
        return false;
    }
    color = dynamic_cast<Color *>(plugins[0]);
    if (!color) {
        setErrorString("No Color plugin for WxBasicGui.");
        return false;
    }
    mDialog = new WxBasicGuiDialog(chain, color);  
    mDialog->Show(true);    
    return true;
}

// ----------------------------------
//
// virtual protected
bool 
Ookala::WxBasicGui::_postRun(PluginChain *chain)
{
    mDialog->Show(false);
    mDialog->Destroy();

    mDialog = NULL;

    return true;
}


