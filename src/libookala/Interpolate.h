// --------------------------------------------------------------------------
// $Id: Interpolate.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef INTERPOLATE_H_HAS_BEEN_INCLUDED
#define INTERPOLATE_H_HAS_BEEN_INCLUDED

#include <map>
#include "Types.h"
#include "Plugin.h"

namespace Ookala {

class EXIMPORT Interpolate: public Plugin
{
    public:
        Interpolate();
        Interpolate(const Interpolate &src);
        virtual ~Interpolate();
        Interpolate & operator=(const Interpolate &src);
        
        // Linear interpolation of a sampled function. This won't 
        // extrapolate at the end points, just clamp to the ends.        
        double linearInterp(const double x, 
                            std::map<double, double> &sampledF);

        // Catmull-Rom interpolation of a sampled function. This won't 
        // extrapolate at the end points, just clamp to the ends.        
        double catmullRomInterp(const double x, 
                            std::map<double, double> &sampledF);


        // Given a sampled function, sampledF - in the same form
        // as the forwared interpolation before - we want to
        // know the x value which gives us fx.
        // If we're outside the bounds of the data, then
        // we'll just get the lowest or hightest x in the data.
        //
        // Basically, we want:
        //    fx = catmullRomInterp(x, sampledF)
        // where we know fx and sampledF, and need to find x.
        double catmullRomInverseInterp(const double fx,
                            std::map<double, double> &sampledF);

    protected:
        // Evaluate a Catmull-Rom curve where t is in [0,1], t=0 gives
        // p1 and t=1 gives p2.
        double evalCatmullRom(double t, double p0, double p1, 
                                            double p2, double p3);

}; 

}; // namespace Ookala

#endif
