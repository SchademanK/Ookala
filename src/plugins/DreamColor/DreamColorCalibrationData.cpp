// --------------------------------------------------------------------------
// $Id: DreamColorCalibrationData.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include "DreamColorCalibrationData.h"

// ======================================
//
// DreamColorCalibrationData
//
// --------------------------------------

Ookala::DreamColorCalibrationData::DreamColorCalibrationData():
    DictItem()
{
    setItemType("DreamColorCalibrationData");

    mIsSerializable = true;

    mCalibrationData = new _DreamColorCalibrationData;
}

// --------------------------------------
//

Ookala::DreamColorCalibrationData::DreamColorCalibrationData(
                const DreamColorCalibrationData &src):
    DictItem(src)
{
     mCalibrationData = new _DreamColorCalibrationData;

    if (src.mCalibrationData) {
        mCalibrationData->info     = src.mCalibrationData->info;
        mCalibrationData->preLutR  = src.mCalibrationData->preLutR;
        mCalibrationData->preLutG  = src.mCalibrationData->preLutG;
        mCalibrationData->preLutB  = src.mCalibrationData->preLutB;

        mCalibrationData->matrix   = src.mCalibrationData->matrix;

        mCalibrationData->postLutR = src.mCalibrationData->postLutR;
        mCalibrationData->postLutG = src.mCalibrationData->postLutG;
        mCalibrationData->postLutB = src.mCalibrationData->postLutB;

        mCalibrationData->p0Reg         = src.mCalibrationData->p0Reg;
        mCalibrationData->p1Reg         = src.mCalibrationData->p1Reg;
        mCalibrationData->p2Reg         = src.mCalibrationData->p2Reg;
        mCalibrationData->brightnessReg = src.mCalibrationData->brightnessReg;
    }
   
}


// --------------------------------------
//
// virtual 
Ookala::DreamColorCalibrationData::~DreamColorCalibrationData()
{
    if (mCalibrationData) {
        delete mCalibrationData;
        mCalibrationData = NULL;
    }
}

// ----------------------------
//
Ookala::DreamColorCalibrationData &
Ookala::DreamColorCalibrationData::operator=(
            const DreamColorCalibrationData &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        if (mCalibrationData) {
            delete mCalibrationData;
            mCalibrationData = NULL;
        }

        if (src.mCalibrationData) {
            mCalibrationData  = new _DreamColorCalibrationData();
            *mCalibrationData = *(src.mCalibrationData);
        }
    }

    return *this;
}



// --------------------------------------
//
// virtual 
Ookala::DreamColorSpaceInfo       
Ookala::DreamColorCalibrationData::getColorSpaceInfo() const
{
    if (mCalibrationData) {
        return mCalibrationData->info;
    }

    DreamColorSpaceInfo info;

    return info;
}

// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setColorSpaceInfo(
                            const DreamColorSpaceInfo &info)
{
    if (mCalibrationData) {
        mCalibrationData->info = info;
    }
}

// --------------------------------------
//
// virtual 
std::vector<uint32_t>     
Ookala::DreamColorCalibrationData::getPreLutRed() const
{
    if (mCalibrationData) {
        return mCalibrationData->preLutR;
    }

    std::vector<uint32_t> lut;
    return lut;
}

// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setPreLutRed(
                        const std::vector<uint32_t> &lut)
{
    if (mCalibrationData) {
        mCalibrationData->preLutR = lut;
    }
}

// --------------------------------------
//
// virtual 
std::vector<uint32_t>     
Ookala::DreamColorCalibrationData::getPreLutGreen() const
{
    if (mCalibrationData) {
        return mCalibrationData->preLutG;
    }

    std::vector<uint32_t> lut;
    return lut;

}

// --------------------------------------
//
// virtual 
void                     
Ookala::DreamColorCalibrationData::setPreLutGreen(
                            const std::vector<uint32_t> &lut)
{
    if (mCalibrationData) {
        mCalibrationData->preLutG = lut;
    }
}


// --------------------------------------
//
// virtual 
std::vector<uint32_t>     
Ookala::DreamColorCalibrationData::getPreLutBlue() const
{
    if (mCalibrationData) {
        return mCalibrationData->preLutB;
    }

    std::vector<uint32_t> lut;
    return lut;
}


// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setPreLutBlue(
                                const std::vector<uint32_t> &lut)
{
    if (mCalibrationData) {
        mCalibrationData->preLutB = lut;
    }
}


// --------------------------------------
//
// virtual 
Ookala::Mat33                     
Ookala::DreamColorCalibrationData::getMatrix() const
{
    if (mCalibrationData) {
        return mCalibrationData->matrix;
    }

    Mat33 mat;

    mat.m00 = mat.m11 = mat.m22 = 1;
    mat.m01 = mat.m02 = mat.m10 = mat.m12 = mat.m20 = mat.m21 = 0;

    return mat;
}


// --------------------------------------
//
// virtual 
void                     
Ookala::DreamColorCalibrationData::setMatrix(const Mat33 &matrix)
{
    if (mCalibrationData) {
        mCalibrationData->matrix = matrix;
    }
}


// --------------------------------------
//
// virtual 
std::vector<uint32_t>     
Ookala::DreamColorCalibrationData::getPostLutRed() const
{
    if (mCalibrationData) {
        return mCalibrationData->postLutR;
    }

    std::vector<uint32_t> lut;
    return lut;
}


// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setPostLutRed(
                                const std::vector<uint32_t> &lut)
{
    if (mCalibrationData) {
        mCalibrationData->postLutR = lut;
    }
}


// --------------------------------------
//
// virtual 
std::vector<uint32_t>    
Ookala::DreamColorCalibrationData::getPostLutGreen() const
{
    if (mCalibrationData) {
        return mCalibrationData->postLutG;
    }

    std::vector<uint32_t> lut;
    return lut;
}


// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setPostLutGreen(
                            const std::vector<uint32_t> &lut)
{
    if (mCalibrationData) {
        mCalibrationData->postLutG = lut;
    }
}

// --------------------------------------
//
// virtual 
std::vector<uint32_t>     
Ookala::DreamColorCalibrationData::getPostLutBlue() const
{
    if (mCalibrationData) {
        return mCalibrationData->postLutB;
    }

    std::vector<uint32_t> lut;
    return lut;
}


// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setPostLutBlue(
                            const std::vector<uint32_t> &lut)
{
    if (mCalibrationData) {
        mCalibrationData->postLutB = lut;
    }
}


// --------------------------------------
//
// virtual 
uint32_t                 
Ookala::DreamColorCalibrationData:: getRegP0() const
{
    if (mCalibrationData) {
        return mCalibrationData->p0Reg;
    }
    return 0;
}

// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setRegP0(uint32_t value)
{
    if (mCalibrationData) {
        mCalibrationData->p0Reg = value;
    }
}


// --------------------------------------
//
// virtual 
uint32_t                  
Ookala::DreamColorCalibrationData::getRegP1() const
{
    if (mCalibrationData) {
        return mCalibrationData->p1Reg;
    }
    return 0;
}

// --------------------------------------
//
// virtual 
void                     
Ookala::DreamColorCalibrationData::setRegP1(uint32_t value)
{
    if (mCalibrationData) {
        mCalibrationData->p1Reg = value;
    }
}


// --------------------------------------
//
// virtual 
uint32_t                  
Ookala::DreamColorCalibrationData::getRegP2() const
{
    if (mCalibrationData) {
        return mCalibrationData->p2Reg;
    }
    return 0;
}


// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setRegP2(uint32_t value)
{
    if (mCalibrationData) {
        mCalibrationData->p2Reg = value;
    }
}



// --------------------------------------
//
// virtual 
uint32_t                  
Ookala::DreamColorCalibrationData::getRegBrightness() const
{
    if (mCalibrationData) {
        return mCalibrationData->brightnessReg;
    }
    return 0;
}


// --------------------------------------
//
// virtual 
void                      
Ookala::DreamColorCalibrationData::setRegBrightness(uint32_t value)
{
    if (mCalibrationData) {
        mCalibrationData->brightnessReg = value;
    }
}

