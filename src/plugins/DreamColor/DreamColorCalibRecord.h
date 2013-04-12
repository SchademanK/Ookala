// --------------------------------------------------------------------------
// $Id: DreamColorCalibRecord.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DREAMCOLORCALIBRECORD_H_HAS_BEEN_INCLUDED
#define DREAMCOLORCALIBRECORD_H_HAS_BEEN_INCLUDED

#include "DataSavior.h"

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

namespace Ookala {

struct _DreamColorCalibRecord;
class EXIMPORT DreamColorCalibRecord: public CalibRecordDictItem
{
    public:
        DreamColorCalibRecord();
        DreamColorCalibRecord(const DreamColorCalibRecord &src);
        virtual ~DreamColorCalibRecord();

        DreamColorCalibRecord & operator=(
                              const DreamColorCalibRecord &src);

        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);


        // Get/Set the brightness register value.
        virtual bool     setBrightnessReg(uint32_t value);
        virtual uint32_t getBrightnessReg();

    private:

        _DreamColorCalibRecord *mDreamColorCalibRecordData;
};

}; // namespace Ookala



#endif

