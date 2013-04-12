// --------------------------------------------------------------------------
// $Id: Color.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <math.h>

#include "Color.h"

//
// ----------------------------
//
//
Ookala::Color::Color():
    Plugin()
{
    setName("Color");
}

// ----------------------------
//
Ookala::Color::Color(const Color &src):
    Plugin(src)
{

}

// ----------------------------
//
// virtual
Ookala::Color::~Color()
{

}

// ----------------------------
//
Ookala::Color &
Ookala::Color::operator=(const Color &src)
{
    if (this != &src) {
        Plugin::operator=(src);
    }

    return *this;
}

// ----------------------------
//
// Convert from rgb -> Yxy 
Ookala::Yxy
Ookala::Color::cvtRgbToYxy(const Rgb &src, const Npm &npm)
{
    XYZ  dstXYZ;
    Vec3 xyz, rgb;

    rgb.v0 = src.r;
    rgb.v1 = src.g;
    rgb.v2 = src.b;

    xyz = mat33VectorMul(npm.rgbToXYZ, rgb);

    dstXYZ.X = xyz.v0;
    dstXYZ.Y = xyz.v1;
    dstXYZ.Z = xyz.v2;

    return cvtXYZToYxy(dstXYZ);
}

// ----------------------------
//
// Convert from rgb -> Yxy 
Ookala::Rgb  
Ookala::Color::cvtYxyToRgb(const Yxy &src, const Npm &npm)
{
    Vec3 xyz, rgb;
    Rgb dstRgb;
    XYZ srcXYZ = cvtYxyToXYZ(src);

    xyz.v0 = srcXYZ.X;
    xyz.v1 = srcXYZ.Y;
    xyz.v2 = srcXYZ.Z;

    rgb = mat33VectorMul(npm.invRgbToXYZ, xyz);

    dstRgb.r = rgb.v0;
    dstRgb.g = rgb.v1;
    dstRgb.b = rgb.v2;

    return dstRgb;
}

// ----------------------------
//
// Convert from Yxy -> XYZ
Ookala::XYZ
Ookala::Color::cvtYxyToXYZ(const Yxy &src)
{
    XYZ dst;

    if (src.Y < 1e-6) {
        dst.X = 
        dst.Y = 
        dst.Z = 0;
        return dst;
    }

    dst.X = (src.x * src.Y) / src.y;
    dst.Y = src.Y;
    dst.Z = (1. - src.x - src.y) * src.Y / src.y;

    return dst;
}

// ----------------------------
//
// Convert from XYZ -> Yxy
Ookala::Yxy 
Ookala::Color::cvtXYZToYxy(const XYZ &src)
{
    Yxy dst;
    double sum = src.X + src.Y + src.Z;

    if ((fabs(sum) < 1e-6) || (src.Y < 1e-6)) {
        dst.Y = 
        dst.x = 
        dst.y = 0;

        return dst;
    }

    dst.Y = src.Y;
    dst.x = src.X / sum;
    dst.y = src.Y / sum;

    return dst;
}

// ----------------------------
//
// dstYxy = srcYxyA - srcYxyB
Ookala::Yxy 
Ookala::Color::subYxy(const Yxy &srcA, const Yxy &srcB)
{
    XYZ result, srcAXYZ, srcBXYZ;

    srcAXYZ = cvtYxyToXYZ(srcA);
    srcBXYZ = cvtYxyToXYZ(srcB);
    
    result.X = srcAXYZ.X - srcBXYZ.X;
    result.Y = srcAXYZ.Y - srcBXYZ.Y;
    result.Z = srcAXYZ.Z - srcBXYZ.Z;

    return cvtXYZToYxy(result);
}


// ----------------------------
//
// dstYxy = srcYxyA + srcYxyB
Ookala::Yxy
Ookala::Color::addYxy(const Yxy &srcA, const Yxy &srcB)
{
    XYZ result;

    XYZ srcAXYZ = cvtYxyToXYZ(srcA);
    XYZ srcBXYZ = cvtYxyToXYZ(srcB);

    result.X = srcAXYZ.X + srcBXYZ.X;
    result.Y = srcAXYZ.Y + srcBXYZ.Y;
    result.Z = srcAXYZ.Z + srcBXYZ.Z;
    
    return cvtXYZToYxy(result);
}

// ----------------------------
// 
// Yxy -> Lab
Ookala::Lab
Ookala::Color::cvtYxyToLab(const Yxy &src, const Yxy &white)
{
    Lab    result;
    double e = .008856;
    double k = 903.3;
    double XYZ_r[3], f[3];

    XYZ whiteXYZ = cvtYxyToXYZ(white);
    XYZ srcXYZ   = cvtYxyToXYZ(src);
    
    XYZ_r[0] = srcXYZ.X / whiteXYZ.X;
    XYZ_r[1] = srcXYZ.Y / whiteXYZ.Y;
    XYZ_r[2] = srcXYZ.Z / whiteXYZ.Z;

    for (int i=0; i<3; ++i) {
        if (XYZ_r[i] > e) {
            f[i] = pow(XYZ_r[i], 1./3.);
        } else {
            f[i] = (k*XYZ_r[i] + 16.)/116.;
        }
    }

    result.L = 116. * f[1] - 16.;
    result.a = 500. * (f[0] - f[1]);
    result.b = 200. * (f[1] - f[2]);

    return result;
}

// ----------------------------
// 
// Lab -> Yxy 
Ookala::Yxy
Ookala::Color::cvtLabToYxy(const Lab &src, const Yxy &white)
{
    double e = .008856;
    double k = 903.3;
    double f[3], XYZ_r[3];

    XYZ    dstXYZ, whiteXYZ;

    whiteXYZ = cvtYxyToXYZ(white);

    if (src.L > k*e) {
        XYZ_r[1] = pow((src.L+16.)/116., 3.);
    } else {
        XYZ_r[1] = src.L / k;
    }

    if (XYZ_r[1] > e) {
        f[1] = (src.L + 16.) / 116.;
    } else {
        f[1] = (k*XYZ_r[1] + 16) / 116.;
    }

    f[2] = f[1] - src.b/200.;
    f[0] = f[1] + src.a/500.;

    if (pow(f[0],3.) > e) {
        XYZ_r[0] = pow(f[0], 3.);
    } else {
        XYZ_r[0] = (116.*f[0] - 16.) / k;
    }

    if (pow(f[2], 3.) > e) {
        XYZ_r[2] = pow(f[2], 3.);
    } else {
        XYZ_r[2] = (116.*f[2]-16.) / k;
    }

    dstXYZ.X = whiteXYZ.X * XYZ_r[0];
    dstXYZ.Y = whiteXYZ.Y * XYZ_r[1];
    dstXYZ.Z = whiteXYZ.Z * XYZ_r[2];   

    return cvtXYZToYxy(dstXYZ);
}

// ----------------------------
//
// Compute dE_ab between Yxy values
double 
Ookala::Color::dELab(const Yxy &base, const Yxy &sample, const Yxy &white)
{
    Lab baseLab   = cvtYxyToLab(base,   white);
    Lab sampleLab = cvtYxyToLab(sample, white);

    return sqrt(pow(baseLab.L - sampleLab.L, 2.) + 
                pow(baseLab.a - sampleLab.a, 2.) + 
                pow(baseLab.b - sampleLab.b, 2.));
}

// ----------------------------
//
// Returns a value in K of the CCT. This may return < 0,
// in which case the CCT is "unknown".
double 
Ookala::Color::computeCct(const Yxy &src)
{
    struct cctISOTemperature
    {
        double mr; // temperature in Microreciprocal Kelvin
        double ut; // u coordinate of intersection with blackbody locus, CIE 1960
        double vt; // v coordinate of intersection with blackbody locus
        double tt; // slope of Iso temperature line
    };

    cctISOTemperature cctData[] =
    { //{mr,      ut,       vt,        tt     }
        {0.0,     0.18006,  0.26352,  -0.24341},
        {10.0,    0.18066,  0.26589,  -0.25479},
        {20.0,    0.18133,  0.26846,  -0.26876},
        {30.0,    0.18208,  0.27119,  -0.28539},
        {40.0,    0.18293,  0.27407,  -0.30470},
        {50.0,    0.18388,  0.27709,  -0.32675},
        {60.0,    0.18494,  0.28021,  -0.35156},
        {70.0,    0.18611,  0.28342,  -0.37915},
        {80.0,    0.18740,  0.28668,  -0.40955},
        {90.0,    0.18880,  0.28997,  -0.44278},
        {100.0,   0.19032,  0.29326,  -0.47888},
        {125.0,   0.19462,  0.30141,  -0.58204},
        {150.0,   0.19962,  0.30921,  -0.70471},
        {175.0,   0.20525,  0.31647,  -0.84901},
        {200.0,   0.21142,  0.32312,  -1.01820},
        {225.0,   0.21807,  0.32909,  -1.21680},
        {250.0,   0.22511,  0.33439,  -1.45120},
        {275.0,   0.23247,  0.33904,  -1.72980},
        {300.0,   0.24010,  0.34308,  -2.06370},
        {325.0,   0.24702,  0.34655,  -2.46810},
        {350.0,   0.25591,  0.34951,  -2.96410},
        {375.0,   0.26400,  0.35200,  -3.58140},
        {400.0,   0.27218,  0.35407,  -4.36330},
        {425.0,   0.28039,  0.35577,  -5.37620},
        {450.0,   0.28863,  0.35714,  -6.72620},
        {475.0,   0.29685,  0.35823,  -8.59550},
        {500.0,   0.30505,  0.35907,  -11.3240},
        {525.0,   0.31320,  0.35968,  -15.6280},
        {550.0,   0.32129,  0.36011,  -23.3250},
        {575.0,   0.32931,  0.36038,  -40.7700},
        {600.0,   0.33724,  0.36051,  -116.450},
        {-1.0,    -1.0,     -1.0,     -1.0    } // last entry in list
    };

    // convert (x,y) to CIE 1960 (u,v)
    double scaleBy = 1.0 / (-src.x + 6.0 * src.y + 1.5);
    double uValue  = 2.0 * src.x * scaleBy;
    double vValue  = 3.0 * src.y * scaleBy;

    double dLast = 0.0;
    double mLast = 0.0;
    for ( int index = 0; cctData[index].mr >= 0.0; ++index )
    {
        const double& uThis = cctData[index].ut;
        const double& vThis = cctData[index].vt;
        const double& tThis = cctData[index].tt;
        const double& mThis = cctData[index].mr;

        // dThis is the distance from (us,vs) to this isotemp line
        double dThis = ((vValue - vThis) - tThis * (uValue - uThis))
            / sqrt(1.0 + tThis * tThis);

        // We stop when (dLast/dThis) < 0.0, i.e. when distance changes
        // sign, because this means we have found iso-temperature lines
        // that "straddle" our point.
        if ( dLast/dThis < 0.0 )
        {
            return 1000000.0 / (mLast + (dLast / (dLast - dThis))
                                     * (mThis - mLast));
        }
        dLast = dThis;
        mLast = mThis;
    }

    return -1;
}



// ----------------------------
//
// This computes the matrix that maps RGB -> XYZ.
// 
// Returns false if we can't compute the transforms for some reason.
// 
// Y is ignored on red, green, and blue, but is not ignored on white.
// Notation here follows SMPTE RP 177, fwiw.
//

bool
Ookala::Color::computeNpm(const Yxy &red,     const Yxy &green, 
                  const Yxy &blue,    const Yxy &white,
                  Npm       &npm)
{
    XYZ   whiteXYZ;
    Mat33 p, pInv, c;
    Vec3  w, cRgb;

    // Form a matrix with xyz (that's _little_ xyz) down the columns    
    p.m00 = red.x;
    p.m10 = red.y;
    p.m20 = 1. - red.x - red.y;

    p.m01 = green.x;
    p.m11 = green.y;
    p.m21 = 1. - green.x - green.y;

    p.m02 = blue.x;
    p.m12 = blue.y;
    p.m22 = 1. - blue.x - blue.y;

    // If p is singular, something is horribly wrong.
    if (mat33Inverse(pInv, p) == false) {
        fprintf(stderr, "P Matrix is singular!\n");
        return false;
    }

    // And for a white XYZ vector
    whiteXYZ = cvtYxyToXYZ(white);
    w.v0 = whiteXYZ.X;
    w.v1 = whiteXYZ.Y;
    w.v2 = whiteXYZ.Z;

    cRgb = mat33VectorMul(pInv, w);
 
    c = mat33Identity();
    c.m00 = cRgb.v0;
    c.m11 = cRgb.v1;
    c.m22 = cRgb.v2;

    npm.rgbToXYZ = mat33MatMul(p, c);

    return mat33Inverse(npm.invRgbToXYZ, npm.rgbToXYZ);
}


// ----------------------------
//
// Compute:  [      ]   [            ][        ]
//           [dstVec] = [   srcMat   ][ srcVec ]
//           [      ]   [            ][        ]
//
// where srcMat is row-major.
Ookala::Vec3
Ookala::Color::mat33VectorMul(const Mat33 &srcMat, const Vec3 &srcVec)
{
    Vec3 dst;


    dst.v0 = srcMat.m00*srcVec.v0 + srcMat.m01*srcVec.v1 + 
                                         srcMat.m02*srcVec.v2;
    dst.v1 = srcMat.m10*srcVec.v0 + srcMat.m11*srcVec.v1 + 
                                         srcMat.m12*srcVec.v2;
    dst.v2 = srcMat.m20*srcVec.v0 + srcMat.m21*srcVec.v1 + 
                                         srcMat.m22*srcVec.v2;

    return dst;
}

// ----------------------------
//
// Compute:  [           ]   [            ][             ]
//           [   dstMat  ] = [   srcMatA  ][   srcMatB   ]
//           [           ]   [            ][             ]
//
// Where all parameters are 3x3 row-major matricies
Ookala::Mat33
Ookala::Color::mat33MatMul(const Mat33 &srcA, const Mat33 &srcB)
{
    Mat33  dst;
    double srcMatA[9], srcMatB[9], dstMat[9];

    srcMatA[0] = srcA.m00;
    srcMatA[1] = srcA.m01;
    srcMatA[2] = srcA.m02;
    srcMatA[3] = srcA.m10;
    srcMatA[4] = srcA.m11;
    srcMatA[5] = srcA.m12;
    srcMatA[6] = srcA.m20;
    srcMatA[7] = srcA.m21;
    srcMatA[8] = srcA.m22;

    srcMatB[0] = srcB.m00;
    srcMatB[1] = srcB.m01;
    srcMatB[2] = srcB.m02;
    srcMatB[3] = srcB.m10;
    srcMatB[4] = srcB.m11;
    srcMatB[5] = srcB.m12;
    srcMatB[6] = srcB.m20;
    srcMatB[7] = srcB.m21;
    srcMatB[8] = srcB.m22;

    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            dstMat[3*i+j] = 0;

            for (int k=0; k<3; k++) {
                dstMat[3*i+j] += srcMatA[i*3+k] * srcMatB[k*3+j];
            }
        }
    }

    dst.m00 = dstMat[0];
    dst.m01 = dstMat[1];
    dst.m02 = dstMat[2];
    dst.m10 = dstMat[3];
    dst.m11 = dstMat[4];
    dst.m12 = dstMat[5];
    dst.m20 = dstMat[6];
    dst.m21 = dstMat[7];
    dst.m22 = dstMat[8];

    return dst;
}


// ----------------------------
//
// Given a 3x3 row-major matrix, compute its inverse.
// Returns false if the matrix is singular.
//
//    0   1   2  |  a11(0)  a12(1)  a13(2)
//    3   4   5  |  a21(3)  a22(4)  a23(5)  
//    6   7   8  |  a31(6)  a32(7)  a33(8)
bool
Ookala::Color::mat33Inverse(Mat33 &dst, const Mat33 &src)
{
    double det = 0;

    det  = src.m00 * (src.m11*src.m22 - src.m12*src.m21);
    det -= src.m01 * (src.m10*src.m22 - src.m12*src.m20);
    det += src.m02 * (src.m10*src.m21 - src.m11*src.m20);

    if (fabs(det) < 1e-6) return false;

    dst.m00 = (src.m11*src.m22 - src.m12*src.m21) / det;
    dst.m01 = (src.m02*src.m21 - src.m01*src.m22) / det;
    dst.m02 = (src.m01*src.m12 - src.m02*src.m11) / det;

    dst.m10 = (src.m12*src.m20 - src.m10*src.m22) / det;
    dst.m11 = (src.m00*src.m22 - src.m02*src.m20) / det;
    dst.m12 = (src.m02*src.m10 - src.m00*src.m12) / det;

    dst.m20 = (src.m10*src.m21 - src.m11*src.m20) / det;
    dst.m21 = (src.m01*src.m20 - src.m00*src.m21) / det;
    dst.m22 = (src.m00*src.m11 - src.m01*src.m10) / det;

    return true;
}

// ----------------------------
//
Ookala::Mat33
Ookala::Color::mat33Identity()
{
    Mat33 dst;

    dst.m00 = 1;
    dst.m01 = 0;
    dst.m02 = 0;

    dst.m10 = 0;
    dst.m11 = 1;
    dst.m12 = 0;

    dst.m20 = 0;
    dst.m21 = 0;
    dst.m22 = 1;

    return dst;
}


// ----------------------------
//
double 
Ookala::Color::haltonSequence(int32_t base, int32_t pntIdx) 
{
    double x = 0;
    double f = 1.0/(double)base;

    while (pntIdx) {
        x += f * (double)(pntIdx % base);
        pntIdx /= base;
        f *= 1.0/(double)base;
    }

    return x;

}



