// --------------------------------------------------------------------------
// $Id: DreamColorSpaceInfo.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DREAMCOLORSPACEINFO_H_HAS_BEEN_INCLUDED
#define DREAMCOLORSPACEINFO_H_HAS_BEEN_INCLUDED

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

#include "Types.h"
#include "Plugin.h"
#include "Dict.h"
#include "Ddc.h"

namespace Ookala {

// Description of a color space
class EXIMPORT DreamColorSpaceInfo: public DictItem
{
    public:
        DreamColorSpaceInfo();
        DreamColorSpaceInfo(const DreamColorSpaceInfo &src);
        virtual ~DreamColorSpaceInfo();
        DreamColorSpaceInfo & operator=(const DreamColorSpaceInfo &src);


        // The name of the calibration record to write into when
        // we're done calibrating
        virtual std::string getCalibRecordName();
        virtual void        setCalibRecordName(const std::string &name);

        virtual uint32_t    getConnId();
        virtual void        setConnId(uint32_t conn);

        virtual uint32_t    getPresetId();
        virtual void        setPresetId(uint32_t preset);
        
        virtual bool        getEnabled();
        virtual void        setEnabled(bool enabled);

        virtual bool        getCalibrated();
        virtual void        setCalibrated(bool calibrated);

        virtual uint32_t    getCalibratedTimeRemaining();
        virtual void        setCalibratedTimeRemaining(uint32_t time);

        // The name of this preset, to appear in the OSD.
        virtual std::string getName();
        virtual void        setName(const std::string &name);

        virtual Yxy         getWhite();
        virtual void        setWhite(const Yxy &value);
        
        virtual Yxy         getRed();
        virtual void        setRed(const Yxy &value);

        virtual Yxy         getGreen();
        virtual void        setGreen(const Yxy &value);

        virtual Yxy         getBlue();
        virtual void        setBlue(const Yxy &value);

        virtual void        getTrc(double &gamma, double &a0, double &a1, 
                                                  double &a2, double &a3);
        virtual void        setTrc(double  gamma, double  a0, double  a1, 
                                                  double  a2, double  a3);

        virtual double      getTrcGamma();
        virtual void        setTrcGamma(double value);

        virtual double      getTrcA0();
        virtual void        setTrcA0(double value);

        virtual double      getTrcA1();
        virtual void        setTrcA1(double value);

        virtual double      getTrcA2();
        virtual void        setTrcA2(double value);

        virtual double      getTrcA3();
        virtual void        setTrcA3(double value);

        // Need to fill these in for classes derived from DictItem
        // so saving and loading can behave properly.
        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root) {return true;}
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

    private:
        struct _DreamColorSpaceInfo {
            std::string calibRecordName;

            uint32_t    connId;
            uint32_t    presetId;

            bool        enabled;
            bool        calibrated;
            uint32_t    calibratedTimeRemaining;
            std::string name;
            
            Yxy         white;
            Yxy         red;
            Yxy         green;
            Yxy         blue;

            double      trcGamma;
            double      trcA0;
            double      trcA1;
            double      trcA2;
            double      trcA3;
        };

        _DreamColorSpaceInfo *mDreamColorSpaceInfoData;
};

}; // namespace Ookala 
#endif

