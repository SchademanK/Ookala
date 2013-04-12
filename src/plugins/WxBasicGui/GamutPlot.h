// --------------------------------------------------------------------------
// $Id: GamutPlot.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef GAMUTPLOT_H_HAS_BEEN_DEFINED
#define GAMUTPLOT_H_HAS_BEEN_DEFINED

#include <wx/wx.h>

#if !wxUSE_GLCANVAS
    #error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

// Not sure why there is this discrepancy in file naming between
// wxGtk and wxMsw.
#ifdef WIN32
    #include <wx/glcanvas.h>
#else
    #include <wx/glcanvas.h>
#endif


#include "Types.h"

namespace Ookala {

class Color;
class GamutPlot: public wxGLCanvas
{
    public:
        GamutPlot( Color *color, wxWindow *parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = 0, const wxString& name = _T("GamutPlot"),
            int *attribList=0 );

        virtual ~GamutPlot();

        // Set the target gamut to plot. Y for red/grn/blue is 
        // not of use, only xy. For white, Yxy is used.
        void setTargetRed(const Yxy &red);
        void setTargetGreen(const Yxy &green);
        void setTargetBlue(const Yxy &blue);

        void setTargetWhite(const Yxy &white);

        void setMeasurement(const Yxy &measurement);

        // Set the zoom level, centered on a particular point
        void setZoom(const double zoomFactor, const double xZoomOrigin,
                     const double yZoomOrigin);

        // Event handlers
        void redraw(wxPaintEvent &event);
        void resize(wxSizeEvent &event);
        void zoomIn(wxMouseEvent &event);
        void zoomOut(wxMouseEvent &event);
        void zoomReset(wxMouseEvent &event);

    private:
        DECLARE_EVENT_TABLE()

        Color    *mColor;
        bool      mHasTargetRgb[3], mHasTargetWhite;

        Yxy       mTargetRed,
                  mTargetGreen,
                  mTargetBlue,
                  mTargetWhite;

        bool      mHasMeasurement;
        Yxy       mMeasurement;      // The latest measured point
    
        double    mZoomFactor,       // Zoom control values
                  mInvZoomFactor,
                  mXZoomOrigin,
                  mYZoomOrigin; 
};

}; // namespace Ookala


#endif

