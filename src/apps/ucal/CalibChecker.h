// --------------------------------------------------------------------------
// $Id: CalibChecker.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef CALIBCHECKER_H_HAS_BEEN_DEFINED
#define CALIBCHECKER_H_HAS_BEEN_DEFINED

#include "Plugin.h"

class CalibChecker: public Ookala::Plugin
{
    public:
        CalibChecker();
        CalibChecker(const CalibChecker &src);
        virtual ~CalibChecker();
        CalibChecker & operator=(const CalibChecker &src);


    protected:
        // This is looking for items in the chain dict:
        //
        //    CalibChecker::calibRecordDict -> The dict in which to
        //                                     search for data items.
        //
        //    CalibChecker::calibRecordName -> key to an item of type
        //                                     CalibRecordDictItem
        //
        //    CalibChecker::calibMetaRecordName -> key to an item of type
        //                                         string which is the key
        //                                         in the same dict to an item
        //                                         of type CalibRecordDictItem
        //                                  
        //                                         Dict["Current Preset"] = "Target 0"
        //                                         Dict["Target 0"]       = CalibRecordDictItem

        virtual bool _run(Ookala::PluginChain *chain);

        // Check if LUTs are in the form that they were when we calibrated.
        virtual bool checkLuts(Ookala::CalibRecordDictItem *calibItem);

        // Check if the display is in the same state as when it was calibrated.
        virtual bool checkDisplayState(Ookala::CalibRecordDictItem *calibItem);

        // Set the icon, based on a WxIconCtrl that we can find. We're currently
        // using this to set:
        //   - "[calibRecordName] notCalib"
        //   - "[calibRecordName] calib"
        //   - "[calibRecordName] badLut"
        //   - "[calibRecordName] badDisplayState"

        virtual bool setIcon(Ookala::PluginChain *chain, const std::string &iconName);

};


#endif

