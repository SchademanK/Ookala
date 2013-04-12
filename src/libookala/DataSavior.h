// --------------------------------------------------------------------------
// $Id: DataSavior.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DATASAVIOR_H_HAS_BEEN_INCLUDED
#define DATASAVIOR_H_HAS_BEEN_INCLUDED

#include <string>
#include <vector>
#include <map>

#include "Plugin.h"
#include "Dict.h"

namespace Ookala {

//
// A generic record for saving calibration data of interest.
//

class EXIMPORT CalibRecordDictItem: public DictItem
{
    public:
        CalibRecordDictItem();
        CalibRecordDictItem(const CalibRecordDictItem &src);
        virtual ~CalibRecordDictItem();
        CalibRecordDictItem & operator=(const CalibRecordDictItem &src);


        std::string           getDeviceId();
        void                  setDeviceId(const std::string &);

        uint32_t              getCalibrationTime();
        void                  setCalibrationTime(uint32_t);

        std::string           getCalibrationPluginName();
        void                  setCalibrationPluginName(const std::string &str);
        
        uint32_t              getPreset();
        void                  setPreset(uint32_t);

        std::string           getPresetName();
        void                  setPresetName(const std::string &);

        Yxy                   getTargetWhite();
        void                  setTargetWhite(const Yxy &);

        Yxy                   getTargetRed();
        void                  setTargetRed(const Yxy &);

        Yxy                   getTargetGreen();
        void                  setTargetGreen(const Yxy &);

        Yxy                   getTargetBlue();
        void                  setTargetBlue(const Yxy &);

        Yxy                   getMeasuredWhite();
        void                  setMeasuredWhite(const Yxy &);

        Yxy                   getMeasuredRed();
        void                  setMeasuredRed(const Yxy &);

        Yxy                   getMeasuredGreen();
        void                  setMeasuredGreen(const Yxy &);

        Yxy                   getMeasuredBlue();
        void                  setMeasuredBlue(const Yxy &);

        std::vector<Rgb>      getMeasuredRgb();
        void                  setMeasuredRgb(const std::vector<Rgb> &);
       
        std::vector<Yxy>      getMeasuredYxy();
        void                  setMeasuredYxy(const std::vector<Yxy> &);

        std::vector<std::string> getLutNames();
        std::vector<uint32_t>    getLut(const std::string &);
        void                     setLut(const std::string &, 
                                        const std::vector<uint32_t> &);
        
        // Need to fill these in for classes derived from DictItem
        // so saving and loading can behave properly.
        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

        virtual void debug();

    protected:
        bool serializeSubItem(xmlDocPtr   doc,  xmlNodePtr root,
                              std::string name, DictItem *item);

    private:
        struct _CalibRecordDictItem {
            std::string           deviceId;
            uint32_t              calibrationTime;       // seconds since epoch
            std::string           calibrationPluginName;

            uint32_t              preset;
            std::string           presetName;
    
            Yxy                   targetWhite;
            Yxy                   targetRed;
            Yxy                   targetGreen;
            Yxy                   targetBlue;

            Yxy                   measuredWhite;
            Yxy                   measuredRed;
            Yxy                   measuredGreen;
            Yxy                   measuredBlue;

            std::vector<Rgb>      measuredRgb;
            std::vector<Yxy>      measuredYxy;

            // Hash into Luts based on some calibration-specific
            // naming convention. 
            std::map<std::string, std::vector<uint32_t> > luts;
        };

        _CalibRecordDictItem *mCalibRecordDictItemData;


};


//
// A simple plugin for dealing with persistant data 
// saving and loading.
//
// Filenames are treated to a substitution with of
// "%s" with the current host name.
// 

class EXIMPORT DataSavior: public Plugin
{
    public:
        DataSavior();
        DataSavior(const DataSavior &src);
        virtual ~DataSavior();
        DataSavior & operator=(const DataSavior &src);


         BEGIN_DICTITEM_ALLOC_FUNCS(DataSavior)
             DICTITEM_ALLOC("calibRecord", CalibRecordDictItem)
         END_DICTITEM_ALLOC_FUNCS
         BEGIN_DICTITEM_DELETE_FUNCS
             DICTITEM_DELETE("calibRecord")
         END_DICTITEM_DELETE_FUNCS

        // Save the contents of our dictionary to disk. We'll write
        // to _all_ the file names that we can, to keep everyone
        // in sync.
        virtual bool save(std::vector<Dict *>      dicts, 
                          std::vector<std::string> filenames);

        // Load the contents of the dictionary from disk. This 
        // will blow away anything currently stored, so be careful!
        // This probably isn't something you want to do frequently.
        //
        // load() will read in the newest file in filenames
        virtual bool load(std::vector<std::string> filenames);
                          


    protected:

        // Perform any string substitution on file names that we
        // get as input to form proper file names
        std::string getFilename(std::string filePattern);
        

        // This responds to commands found in the chain dict. IT
        // will look for:
        //
        //    DataSavior::save      [bool]
        //    DataSavior::load      [bool]
        //    DataSavior::fileNames [stringArray]
        //    DataSavior::dictNames [stringArray]
        //      + When saving, save out these dicts.
        //
        virtual bool _run(PluginChain *chain);
};

}; // namespace Ookala

#endif

