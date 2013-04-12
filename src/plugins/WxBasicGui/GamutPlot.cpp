// --------------------------------------------------------------------------
// $Id: GamutPlot.cpp 135 2008-12-19 00:49:58Z omcf $
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

//#include <GL/gl.h>

#include "Color.h"
#include "GamutPlot.h"
#include "SpectralLocus.h"

// ----------------------------------------
//

Ookala::GamutPlot::GamutPlot(Color *color, wxWindow *parent, wxWindowID id,
            const wxPoint& pos, const wxSize& size,
            long style, const wxString& name,
            int *attribList):
    wxGLCanvas(parent, id, pos, size, style, name, attribList),
    mZoomFactor(1),
    mInvZoomFactor(1),
    mXZoomOrigin(.3333),
    mYZoomOrigin(.3333) 
{
    mColor           = color;

    mHasTargetRgb[0] = 
    mHasTargetRgb[1] = 
    mHasTargetRgb[2] = false;

    mHasTargetWhite  = false;
    mHasMeasurement  = false;
}

// ----------------------------------------
//
// virtual
Ookala::GamutPlot::~GamutPlot()
{

}

// ----------------------------------------
//
void
Ookala::GamutPlot::setTargetRed(const Yxy &redYxy)
{
    mTargetRed       = redYxy;
    mHasTargetRgb[0] = true;

    if ((mHasTargetRgb[0]) && (mHasTargetRgb[1]) &&
                   (mHasTargetRgb[2]))  {
    
        Refresh();
    }
}

// ----------------------------------------
//
void
Ookala::GamutPlot::setTargetGreen(const Yxy &greenYxy)
{
    mTargetGreen     = greenYxy;
    mHasTargetRgb[1] = true;

    if ((mHasTargetRgb[0]) && (mHasTargetRgb[1]) &&
                   (mHasTargetRgb[2]))  {
    
        Refresh();
    }
}

// ----------------------------------------
//
void
Ookala::GamutPlot::setTargetBlue(const Yxy &blueYxy)
{
    mTargetBlue        = blueYxy;
    mHasTargetRgb[2]   = true;

    if ((mHasTargetRgb[0]) && (mHasTargetRgb[1]) &&
                   (mHasTargetRgb[2]))  {
    
        Refresh();
    }
}


// ----------------------------------------
//
void
Ookala::GamutPlot::setTargetWhite(const Yxy &whiteYxy)
{
    mTargetWhite       = whiteYxy;
    mHasTargetWhite    = true;

    Refresh();
}


// ----------------------------------------
//
void
Ookala::GamutPlot::setMeasurement(const Yxy &measurement)
{
    mMeasurement       = measurement;
    mHasMeasurement    = true;

    Refresh();
}


// ----------------------------------------
//
void
Ookala::GamutPlot::setZoom(const double zoomFactor, const double xZoomOrigin,
                      const double yZoomOrigin)
{
    mZoomFactor    = zoomFactor;
    mInvZoomFactor = 1.0 / zoomFactor;
    mXZoomOrigin   = xZoomOrigin;
    mYZoomOrigin   = yZoomOrigin;

    // Post a refresh
    Refresh();
}

// ----------------------------------------
//
void
Ookala::GamutPlot::redraw(wxPaintEvent& event)
{     
    wxPaintDC dc(this);

    if (!GetContext()) return;

    SetCurrent();

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.005, 0.65, -0.005, 0.65, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    // Save view before zooming.
    glPushMatrix();
    glTranslated(mXZoomOrigin, mYZoomOrigin, 0.0);
    glScaled(mZoomFactor, mZoomFactor, 0.0);
    glTranslated(-mXZoomOrigin, -mYZoomOrigin, 0.0);

    // Draw grid lines.
    const double gridInc = 0.1 * mInvZoomFactor;
    glBegin(GL_LINES);
    for ( double grid = 0.0; grid < 0.95; grid += gridInc )
    {
        glColor3d(0.2, 0.2, 0.2);
        glVertex2d(0.0, grid);
        glVertex2d(0.9, grid);
        glColor3d(0.25, 0.25, 0.25);
        glVertex2d(grid, 0.0);
        glVertex2d(grid, 0.9);
    }
    glEnd();

    // Enable line smoothing
    glEnable(GL_BLEND); 
    glDisable(GL_DEPTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    glEnable(GL_LINE_SMOOTH);

    // Draw color locus.
    glColor3d(0.7, 0.7, 0.7);

    // Plot u'v' instead of xy
    glBegin(GL_LINE_LOOP);
    for (uint32_t idx=0; idx<sizeof(xySpectralLocus)/(2*sizeof(GLdouble)); ++idx) {
        GLdouble sum =  -2. * xySpectralLocus[2*idx] + 
                        12. * xySpectralLocus[2*idx+1] + 3.;
        glVertex2d(4.*xySpectralLocus[2*idx]/sum , 9.*xySpectralLocus[2*idx+1]/sum);
    }
    glEnd();


    // If we have a target color gamut, draw that.
    if ((mHasTargetRgb[0]) && (mHasTargetRgb[1]) && 
                    (mHasTargetRgb[2])) {
        GLdouble sum;

        // Draw monitor gamut.
        glBegin(GL_LINE_LOOP);
        glColor3d(1.0, 0.0, 0.0);

        sum = -2.*mTargetRed.x + 12.*mTargetRed.y + 3.;
        glVertex2d(4.*mTargetRed.x/sum,   9.*mTargetRed.y/sum);
        glColor3d(0.0, 1.0, 0.0);

        sum = -2.*mTargetGreen.x + 12.*mTargetGreen.y + 3.;
        glVertex2d(4.*mTargetGreen.x/sum,   9.*mTargetGreen.y/sum);
        glColor3d(0.0, 0.0, 1.0);

        sum = -2.*mTargetBlue.x + 12.*mTargetBlue.y + 3.;
        glVertex2d(4.*mTargetBlue.x/sum,   9.*mTargetBlue.y/sum);
        glColor3d(1.0, 0.0, 0.0);
        glEnd();
    }

    if (mHasTargetWhite) {
        Yxy      valueYxy;
        Lab      valueLab;
        GLdouble sum;

        valueLab.L = 100;

        for (double rad=1; rad<4; ++rad) {

            glColor3f( 1./rad, 1./rad, 1./rad);

            glBegin(GL_LINE_LOOP);
            for (double theta=0; theta<6.28; theta+=.1) {
                valueLab.a = rad*cos(theta);
                valueLab.b = rad*sin(theta);

                valueYxy = mColor->cvtLabToYxy(valueLab, mTargetWhite);

                sum = -2.*valueYxy.x + 12.*valueYxy.y + 3.;
                glVertex2d(4.*valueYxy.x/sum,   9.*valueYxy.y/sum);
            }    
            glEnd();
        }
    }

    // Disable line smoothing
    glDisable(GL_BLEND); 
    glEnable(GL_DEPTH);
    glDisable(GL_LINE_SMOOTH);


    // If we have a point to display, show it.
    if (mHasMeasurement) {
        GLdouble sum = -2.*mMeasurement.x + 12.*mMeasurement.y + 3.;

        glColor3d(0.9, 0.9, 0.9);

        glPointSize(5);
        glBegin(GL_POINTS);
           glVertex2d(4.*mMeasurement.x/sum, 9.*mMeasurement.y/sum);
        glEnd();
        glPointSize(1);
    }

    glPopMatrix();

    SwapBuffers();
}

// ----------------------------------------
//

void 
Ookala::GamutPlot::resize(wxSizeEvent& event)
{
    SetCurrent();

    glViewport(0, 0, GLsizei(event.GetSize().GetWidth()),
                 GLsizei(event.GetSize().GetHeight()));
}


// ----------------------------------------
//

void 
Ookala::GamutPlot::zoomIn(wxMouseEvent &event)
{
    double newZoom = mZoomFactor + 1.0;
    if ( newZoom > 30 )
    {
        newZoom = 1.0;
    }

    if (mHasTargetWhite) {
        GLdouble sum = -2.*mTargetWhite.x + 12.*mTargetWhite.y + 3.;
        setZoom(newZoom, 4.*mTargetWhite.x/sum, 9.*mTargetWhite.y/sum);
    } else {
        setZoom(newZoom, .2105, .4737); // Equal-energy in u'v'
    }
}

// ----------------------------------------
//

void 
Ookala::GamutPlot::zoomOut(wxMouseEvent &event)
{
    double newZoom = mZoomFactor - 1.0;
    if ( newZoom < 1.0 )
    {
        newZoom = 30;
    }

    if (mHasTargetWhite) {
        GLdouble sum = -2.*mTargetWhite.x + 12.*mTargetWhite.y + 3.;
        setZoom(newZoom, 4.*mTargetWhite.x/sum, 9.*mTargetWhite.y/sum);
    } else {
        setZoom(newZoom, .2105, .4737); // Equal-energy in u'v'
    }
}

// ----------------------------------------
//

void 
Ookala::GamutPlot::zoomReset(wxMouseEvent &event)
{
    if (mHasTargetWhite) {
        GLdouble sum = -2.*mTargetWhite.x + 12.*mTargetWhite.y + 3.;
        setZoom(1.0, 4.*mTargetWhite.x/sum, 9.*mTargetWhite.y/sum);
    } else {
        setZoom(1.0, .2105, .4737); // Equal-energy in u'v'
    }
}

// ----------------------------------------


BEGIN_EVENT_TABLE(Ookala::GamutPlot, wxGLCanvas)
    EVT_SIZE(GamutPlot::resize)
    EVT_PAINT(GamutPlot::redraw)
    EVT_LEFT_UP(GamutPlot::zoomIn)
    EVT_MIDDLE_UP(GamutPlot::zoomReset)
    EVT_RIGHT_UP(GamutPlot::zoomOut)
END_EVENT_TABLE()


