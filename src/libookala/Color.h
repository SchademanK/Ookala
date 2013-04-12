// --------------------------------------------------------------------------
// $Id: Color.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef COLOR_H_HAS_BEEN_INCLUDED
#define COLOR_H_HAS_BEEN_INCLUDED

#include "Types.h"
#include "Plugin.h"

namespace Ookala {

// Various color space related utilities

class EXIMPORT Color: public Plugin
{
    public:
        Color();
        Color(const Color &src);
        virtual ~Color();
        Color & operator=(const Color &src);

        // Convert rgb <-> Yxy
        Yxy  cvtRgbToYxy(const Rgb &src, const Npm &npm);
        Rgb  cvtYxyToRgb(const Yxy &src, const Npm &npm);

        // Convert from Yxy <-> XYZ
        XYZ  cvtYxyToXYZ(const Yxy &src);
        Yxy  cvtXYZToYxy(const XYZ &src);

        // dstYxy = srcA - srcB
        Yxy subYxy(const Yxy &srcA, const Yxy &srcB);
                                         
        // dstYxy = srcYxyA + srcYxyB
        Yxy addYxy(const Yxy &srcA, const Yxy &srcB);

        // Convert from Yxy <-> Lab
        Lab cvtYxyToLab(const Yxy &src, const Yxy &white);
        Yxy cvtLabToYxy(const Lab &src, const Yxy &white);

        // Compute dE_ab between Yxy values
        double dELab(const Yxy &base, const Yxy &sample, const Yxy &white);

        // Returns a value in K of the CCT. This may return < 0,
        // in which case the CCT is "unknown".
        double computeCct(const Yxy &src);

        // This computes the matrix that maps RGB -> XYZ.
        //
        // Returns false if we can't compute the transforms 
        // for some reason (generally, because the matrix
        // is singular).
        //
        // Y is ignored on red, green, and blue, but is not 
        // ignored on white.
        //
        // Note that we're NOT normalizing white to 1.0,
        // but rather using the Y of white that you pass in.
        bool computeNpm(const Yxy &red,   const Yxy &green, 
                        const Yxy &blue,  const Yxy &white,
                        Npm       &npm);

        
        // Compute:  [      ]   [            ][        ]
        //           [dstVec] = [   srcMat   ][ srcVec ]
        //           [      ]   [            ][        ]
        //
        // where srcMat is row-major.
        Vec3 mat33VectorMul(const Mat33 &srcMat, const Vec3 &srcVec);

        // Compute:  [           ]   [            ][             ]
        //           [   dstMat  ] = [   srcMatA  ][   srcMatB   ]
        //           [           ]   [            ][             ]
        //
        // Where all parameters are 3x3 row-major matricies
        Mat33 mat33MatMul(const Mat33 &srcA, const Mat33 &srcB);

        // Given a 3x3 row-major matrix, compute its inverse.
        // Returns false if the matrix is singular.
        bool mat33Inverse(Mat33 &dst, const Mat33 &src);

        // Return an identity matrix;
        Mat33 mat33Identity();


        // Generate 'random' numbers from the Halton sequence.
        //
        // numPnts is the number of points you want to draw
        // pntIdx is the i-th point in the sequence  [0-numPnts-1]
        // base is a prime number.
        //
        // To generate 2d points, we would get:
        //
        //   for (int i=0; i<32; ++i) {
        //      x = (double)i / 32.;
        //      y = haltonSequence(2, i, 32);
        //   }
        //
        // And in 3d:
        //
        //   for (int i=0; i<32; ++i) {
        //      x = (double)i / 32.;
        //      y = haltonSequence(2, i);
        //      z = haltonSequence(3, i);
        //   }
        //
        // This will give values over the unit interval.
        double haltonSequence(int32_t base, int32_t pntIdx); 


};

}; // namespace Ookala

#endif

