// --------------------------------------------------------------------------
// $Id: Interpolate.cpp 135 2008-12-19 00:49:58Z omcf $
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

#ifdef _WIN32
#pragma warning(disable: 4786)
#endif

#include <stdio.h>
#include <math.h>

#include "Interpolate.h"


// ----------------------------
//
//
Ookala::Interpolate::Interpolate():
    Plugin()
{
    setName("Interpolate");
}

// ----------------------------
//
Ookala::Interpolate::Interpolate(const Interpolate &src):
    Plugin(src)
{
}

// ----------------------------
//
// virtual
Ookala::Interpolate::~Interpolate()
{

}

// ----------------------------
//
Ookala::Interpolate &
Ookala::Interpolate::operator=(const Interpolate &src)
{
    if (this != &src) {
        Plugin::operator=(src);
    }

    return *this;
}

// ----------------------------
//
// Linear interpolation of a sampled function. This won't 
// extrapolate at the end points, just clamp to the ends.        
double 
Ookala::Interpolate::linearInterp(const double x, 
                    std::map<double, double> &sampledF)
{
    // Clamp if we're off either end
    if (x <= (*sampledF.begin()).first) {
        return (*sampledF.begin()).second;
    }
    if ( sampledF.lower_bound(x) == sampledF.end() ) {
        std::map<double, double>::iterator last = sampledF.end();
        last--;
        return (*last).second; 
    }

    // Find the bounding data points
    std::map<double, double>::iterator min = sampledF.begin();
    std::map<double, double>::iterator max = sampledF.begin();
    max++;
    
    while (max != sampledF.end()) {

        if (((*min).first <= x) && ((*max).first >= x)) {
            break;
        }

        min++;
        max++;
    }

    if (max == sampledF.end()) {
        // Value not found, apparently.
        max--;
        return (*max).second;
    }

    // Divide-by-zero guard
    if ((*max).first - (*min).first < 1e-5) {
        return (*min).second;
    }

    double t = (x - (*min).first) / ((*max).first - (*min).first);

    if (t < 0) t = 0;
    if (t > 1) t = 1;

    return (1-t)*(*min).second + t*(*max).second;
}


// ----------------------------
//
double 
Ookala::Interpolate::catmullRomInterp(const double x, 
                    std::map<double, double> &sampledF)
{
    // Clamp if we're off either end
    if (x <= (*sampledF.begin()).first) {
        return (*sampledF.begin()).second;
    }
    if ( sampledF.lower_bound(x) == sampledF.end() ) {
        std::map<double, double>::iterator last = sampledF.end();
        last--;
        return (*last).second; 
    }

    // Find the bounding data points
    std::map<double, double>::iterator p1 = sampledF.begin();
    std::map<double, double>::iterator p2 = sampledF.begin();
    p2++;
    
    while (p2 != sampledF.end()) {

        if (((*p1).first <= x) && ((*p2).first >= x)) {
            break;
        }

        p1++;
        p2++;
    }

    if (p2 == sampledF.end()) {
        // Value not found, apparently.
        p2--;
        return (*p2).second;
    }

    // Find the points outside the points
    std::map<double, double>::iterator p0 = p1;
    std::map<double, double>::iterator p3 = p2;

    if (p0 != sampledF.begin()) {
        p0--;
    }   

    p3++;
    if (p3 == sampledF.end()) {
        p3--;
    }


    // Divide-by-zero guard
    if ((*p2).first - (*p1).first < 1e-5) {
        return (*p1).second;
    }

    double t = (x - (*p1).first) / ((*p2).first - (*p1).first);

    if (t < 0) t = 0;
    if (t > 1) t = 1;

    return evalCatmullRom(t, (*p0).second, (*p1).second,
                             (*p2).second, (*p3).second);
}

// ----------------------------
//
// We're finding the x that yields the closest thing to fx
// when interpolated using the data. Basically, we want
//    fx = catmullRomInterp(x, sampledF)
// where we know fx and sampledF, and need to find x.
double 
Ookala::Interpolate::catmullRomInverseInterp(const double fx, 
                               std::map<double, double> &sampledF)
{
    if (fx <= (*sampledF.begin()).second) {
        return (*sampledF.begin()).first;
    }

    if (fx >= (*sampledF.rbegin()).second) {
        return (*sampledF.rbegin()).first;
    }

    // Find the two points in the data that bound fx - remember
    // that we're checking .second here, not .first
    std::map<double, double>::iterator p1, p2;

    p1 = sampledF.begin();
    p2 = sampledF.begin();
    p2++;

    while (p2 != sampledF.end()) {
        if (((*p1).second <= fx) && ((*p2).second >= fx)) {
            break;
        }

        p1++;
        p2++;
    }
    if (p2 == sampledF.end()) {
        p2--;
        return (*p2).first;
    }

    std::map<double, double>::iterator p0, p3;

    p0 = p1;
    if (p1 != sampledF.begin()) {
        p0--; 
    }
    
    p3 = p2;
    p3++;
    if (p3 == sampledF.end()) {
        p3--;
    }

    // Find the parameter t which, when used to interpolate across the 
    // 4 points in the set, gives fx within some tolerance.
    // There is a closed-form solution, but an iterative solution is 
    // probably easier to read. Newton's method is troublesome as it 
    // might bounce outside the range we would like it to stay - so 
    // just use binary search for now.
    double t        = 0.50;
    double stepSize = 0.25;
    double ft       = evalCatmullRom(t, (*p0).second, (*p1).second,
                                        (*p2).second, (*p3).second);
    for (uint32_t iter=0; iter<100; iter++) {
        
        if (fabs(ft - fx) < 0.00001) break;

        if (ft < fx) {
            t += stepSize;
        } else {
            t -= stepSize;
        }
        stepSize *= 0.5;
        ft  = evalCatmullRom(t, (*p0).second, (*p1).second,
                                (*p2).second, (*p3).second);
    }

    if (fabs(ft - fx) > .00001) {
        fprintf(stderr, "catmullRomInverseInterp(): Failed to converge\n");
    }


    return evalCatmullRom(t, (*p0).first, (*p1).first,
                             (*p2).first, (*p3).first);
}

// ----------------------------
//
// Evaluate a Catmull-Rom curve where t is in [0,1], t=0 gives
// p1 and t=1 gives p2.
//
// protected
double
Ookala::Interpolate::evalCatmullRom(double t, 
                        double p0, double p1, double p2, double p3)
{
    // Evaluate the cubic equation:
    //   a t^3 + b t^2 + c t + d 
    // where:
    //   a = .5 (3 p1 -   p0 - 3 p2 + p3)
    //   b = .5 (2 p0 - 5 p1 + 4 p2 - p3)
    //   c = .5 (  p2 -   p0)
    //   d = .5 (2 p1)
    
    double a = 0.5*(3.0*p1 -     p0 - 3.0*p2 + p3);
    double b = 0.5*(2.0*p0 - 5.0*p1 + 4.0*p2 - p3);
    double c = 0.5*(  p2 -       p0);
    double d = 0.5*(2.0*p1);
    
    return ((a * t + b) * t + c) * t + d;
}



