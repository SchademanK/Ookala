// --------------------------------------------------------------------------
// $Id: DreamColorCalibrationData.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DREAMCOLORCALIBRATIONDATA_H_HAS_BEEN_INCLUDED
#define DREAMCOLORCALIBRATIONDATA_H_HAS_BEEN_INCLUDED


#ifdef EXIMPORT
    #undef EXIMPORT
#endif

#ifdef _WIN32
#ifdef DREAMCOLOR_EXPORTS
#define EXIMPORT _declspec(dllexport)
#else
#define EXIMPORT _declspec(dllimport)
#endif

#else
#define EXIMPORT
#endif



#include <vector>
#include <map>

#include "Types.h"
#include "Plugin.h"
#include "Dict.h"
#include "Ddc.h"

#include "plugins/DreamColor/DreamColorSpaceInfo.h"

namespace Ookala {

class EXIMPORT DreamColorCalibrationData: public DictItem
{
    public:
        DreamColorCalibrationData();
        DreamColorCalibrationData(const DreamColorCalibrationData &src);
        virtual ~DreamColorCalibrationData();
        DreamColorCalibrationData & operator=(
                            const DreamColorCalibrationData &src);


        virtual DreamColorSpaceInfo   getColorSpaceInfo() const;
        virtual void                  setColorSpaceInfo(const DreamColorSpaceInfo &);

        virtual std::vector<uint32_t> getPreLutRed() const;
        virtual void                  setPreLutRed(const std::vector<uint32_t> &);

        virtual std::vector<uint32_t> getPreLutGreen() const;
        virtual void                  setPreLutGreen(const std::vector<uint32_t> &);

        virtual std::vector<uint32_t> getPreLutBlue() const;
        virtual void                  setPreLutBlue(const std::vector<uint32_t> &);

        virtual Mat33                 getMatrix() const;
        virtual void                  setMatrix(const Mat33 &);

        virtual std::vector<uint32_t> getPostLutRed() const;
        virtual void                  setPostLutRed(const std::vector<uint32_t> &);

        virtual std::vector<uint32_t> getPostLutGreen() const;
        virtual void                  setPostLutGreen(const std::vector<uint32_t> &);

        virtual std::vector<uint32_t> getPostLutBlue() const;
        virtual void                  setPostLutBlue(const std::vector<uint32_t> &);

        virtual uint32_t              getRegP0() const;
        virtual void                  setRegP0(uint32_t);

        virtual uint32_t              getRegP1() const;
        virtual void                  setRegP1(uint32_t);

        virtual uint32_t              getRegP2() const;
        virtual void                  setRegP2(uint32_t);

        virtual uint32_t              getRegBrightness() const;
        virtual void                  setRegBrightness(uint32_t);

    private:
    
        struct _DreamColorCalibrationData {
            DreamColorSpaceInfo   info;

            std::vector<uint32_t> preLutR, 
                                  preLutG, 
                                  preLutB;

            Mat33                 matrix;

            std::vector<uint32_t> postLutR, 
                                  postLutG, 
                                  postLutB;

            uint32_t              p0Reg, 
                                  p1Reg, 
                                  p2Reg, 
                                  brightnessReg;
        };

        _DreamColorCalibrationData *mCalibrationData;
};

}; // namespace Ookala

#endif
