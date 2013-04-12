// --------------------------------------------------------------------------
// $Id: DataSavior.cpp 135 2008-12-19 00:49:58Z omcf $
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
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __linux__
#include <unistd.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>



#include "Types.h"
#include "Dict.h"
#include "DictHash.h"
#include "PluginChain.h"
#include "PluginRegistry.h"
#include "DataSavior.h"

// ===================================
// 
// CalibRecordDictItem
//
// -----------------------------------

Ookala::CalibRecordDictItem::CalibRecordDictItem():
    DictItem()
{
    setItemType("calibRecord");
    mIsSerializable = true;

    mCalibRecordDictItemData = new _CalibRecordDictItem;

    mCalibRecordDictItemData->calibrationTime = 0;
    mCalibRecordDictItemData->preset          = 0;

    mCalibRecordDictItemData->targetWhite.x = 
    mCalibRecordDictItemData->targetWhite.y = 
    mCalibRecordDictItemData->targetWhite.Y = 0;
    mCalibRecordDictItemData->targetRed.x   = 
    mCalibRecordDictItemData->targetRed.y   = 
    mCalibRecordDictItemData->targetRed.Y   = 0;
    mCalibRecordDictItemData->targetGreen.x = 
    mCalibRecordDictItemData->targetGreen.y = 
    mCalibRecordDictItemData->targetGreen.Y = 0;
    mCalibRecordDictItemData->targetBlue.x  = 
    mCalibRecordDictItemData->targetBlue.y  = 
    mCalibRecordDictItemData->targetBlue.Y  = 0;

    mCalibRecordDictItemData->measuredWhite.x = 
    mCalibRecordDictItemData->measuredWhite.y = 
    mCalibRecordDictItemData->measuredWhite.Y = 0;
    mCalibRecordDictItemData->measuredRed.x   = 
    mCalibRecordDictItemData->measuredRed.y   = 
    mCalibRecordDictItemData->measuredRed.Y   = 0;
    mCalibRecordDictItemData->measuredGreen.x = 
    mCalibRecordDictItemData->measuredGreen.y = 
    mCalibRecordDictItemData->measuredGreen.Y = 0;
    mCalibRecordDictItemData->measuredBlue.x  = 
    mCalibRecordDictItemData->measuredBlue.y  = 
    mCalibRecordDictItemData->measuredBlue.Y  = 0;
}

// -----------------------------------
//
Ookala::CalibRecordDictItem::CalibRecordDictItem(
                        const CalibRecordDictItem &src):
    DictItem(src)
{
    mCalibRecordDictItemData = new _CalibRecordDictItem;

    if (src.mCalibRecordDictItemData) {
        mCalibRecordDictItemData->deviceId              = 
            src.mCalibRecordDictItemData->deviceId;
        mCalibRecordDictItemData->calibrationTime       = 
            src.mCalibRecordDictItemData->calibrationTime;
        mCalibRecordDictItemData->calibrationPluginName = 
            src.mCalibRecordDictItemData->calibrationPluginName;

        mCalibRecordDictItemData->preset     = src.mCalibRecordDictItemData->preset;
        mCalibRecordDictItemData->presetName = src.mCalibRecordDictItemData->presetName;

        mCalibRecordDictItemData->targetWhite = src.mCalibRecordDictItemData->targetWhite;
        mCalibRecordDictItemData->targetRed   = src.mCalibRecordDictItemData->targetRed;
        mCalibRecordDictItemData->targetGreen = src.mCalibRecordDictItemData->targetGreen;
        mCalibRecordDictItemData->targetBlue  = src.mCalibRecordDictItemData->targetBlue;
        
        mCalibRecordDictItemData->measuredWhite = src.mCalibRecordDictItemData->measuredWhite;
        mCalibRecordDictItemData->measuredRed   = src.mCalibRecordDictItemData->measuredRed;
        mCalibRecordDictItemData->measuredGreen = src.mCalibRecordDictItemData->measuredGreen;
        mCalibRecordDictItemData->measuredBlue  = src.mCalibRecordDictItemData->measuredBlue;

        mCalibRecordDictItemData->measuredRgb  = src.mCalibRecordDictItemData->measuredRgb;
        mCalibRecordDictItemData->measuredYxy  = src.mCalibRecordDictItemData->measuredYxy;

        mCalibRecordDictItemData->luts         = src.mCalibRecordDictItemData->luts;       
    }
}

// -----------------------------------
//
// virtual
Ookala::CalibRecordDictItem::~CalibRecordDictItem()
{
    if (mCalibRecordDictItemData) {
        delete mCalibRecordDictItemData;
        mCalibRecordDictItemData = NULL;
    }
}

// ----------------------------
//
Ookala::CalibRecordDictItem &
Ookala::CalibRecordDictItem::operator=(const CalibRecordDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        if (mCalibRecordDictItemData) {
            delete mCalibRecordDictItemData;
            mCalibRecordDictItemData = NULL;
        }

        if (src.mCalibRecordDictItemData) {
            mCalibRecordDictItemData = new _CalibRecordDictItem();

            *mCalibRecordDictItemData = *(src.mCalibRecordDictItemData);
        }
    }

    return *this;
}



// -----------------------------------
//
std::string 
Ookala::CalibRecordDictItem::getDeviceId()
{
    return mCalibRecordDictItemData->deviceId;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setDeviceId(const std::string &id)
{
    mCalibRecordDictItemData->deviceId = id;
}

// -----------------------------------
//
uint32_t              
Ookala::CalibRecordDictItem::getCalibrationTime()
{
    return mCalibRecordDictItemData->calibrationTime;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setCalibrationTime(uint32_t time)
{
    mCalibRecordDictItemData->calibrationTime = time;
}

// -----------------------------------
//
std::string   
Ookala::CalibRecordDictItem::getCalibrationPluginName()
{
    return mCalibRecordDictItemData->calibrationPluginName;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setCalibrationPluginName(const std::string &str)
{
    mCalibRecordDictItemData->calibrationPluginName = str;
}
     
// -----------------------------------
//   
uint32_t              
Ookala::CalibRecordDictItem::getPreset()
{
    return mCalibRecordDictItemData->preset;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setPreset(uint32_t preset)
{
    mCalibRecordDictItemData->preset = preset;
}

// -----------------------------------
//
std::string  
Ookala::CalibRecordDictItem::getPresetName()
{
    return  mCalibRecordDictItemData->presetName;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setPresetName(const std::string &name)
{
    mCalibRecordDictItemData->presetName = name;
}

// -----------------------------------
//
Ookala::Yxy                   
Ookala::CalibRecordDictItem::getTargetWhite()
{
    return mCalibRecordDictItemData->targetWhite;

}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setTargetWhite(const Yxy &value)
{
    mCalibRecordDictItemData->targetWhite = value;
}

// -----------------------------------
//
Ookala::Yxy                   
Ookala::CalibRecordDictItem::getTargetRed()
{
    return mCalibRecordDictItemData->targetRed;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setTargetRed(const Yxy &value)
{
    mCalibRecordDictItemData->targetRed = value;
}

// -----------------------------------
//
Ookala::Yxy                   
Ookala::CalibRecordDictItem::getTargetGreen()
{
    return mCalibRecordDictItemData->targetGreen;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setTargetGreen(const Yxy &value)
{
    mCalibRecordDictItemData->targetGreen = value;
}

// -----------------------------------
//
Ookala::Yxy                   
Ookala::CalibRecordDictItem::getTargetBlue()
{
    return mCalibRecordDictItemData->targetBlue;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setTargetBlue(const Yxy &value)
{
    mCalibRecordDictItemData->targetBlue = value;
}

// -----------------------------------
//
Ookala::Yxy                   
Ookala::CalibRecordDictItem::getMeasuredWhite()
{
    return mCalibRecordDictItemData->measuredWhite;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setMeasuredWhite(const Yxy &value)
{
    mCalibRecordDictItemData->measuredWhite = value;
}

// -----------------------------------
//
Ookala::Yxy                   
Ookala::CalibRecordDictItem::getMeasuredRed()
{
    return mCalibRecordDictItemData->measuredRed;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setMeasuredRed(const Yxy &value)
{
    mCalibRecordDictItemData->measuredRed = value;
}

// -----------------------------------
//
Ookala::Yxy                   
Ookala::CalibRecordDictItem::getMeasuredGreen()
{
    return mCalibRecordDictItemData->measuredGreen;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setMeasuredGreen(const Yxy &value)
{
    mCalibRecordDictItemData->measuredGreen = value;
}

// -----------------------------------
//
Ookala::Yxy                   
Ookala::CalibRecordDictItem::getMeasuredBlue()
{
    return mCalibRecordDictItemData->measuredBlue;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setMeasuredBlue(const Yxy &value)
{
    mCalibRecordDictItemData->measuredBlue = value;
}

// -----------------------------------
//
std::vector<Ookala::Rgb>      
Ookala::CalibRecordDictItem::getMeasuredRgb()
{
    return mCalibRecordDictItemData->measuredRgb;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setMeasuredRgb(const std::vector<Rgb> &values)
{  
    mCalibRecordDictItemData->measuredRgb = values;
}

// -----------------------------------
//       
std::vector<Ookala::Yxy>      
Ookala::CalibRecordDictItem::getMeasuredYxy()
{
    return mCalibRecordDictItemData->measuredYxy;
}

// -----------------------------------
//
void                  
Ookala::CalibRecordDictItem::setMeasuredYxy(const std::vector<Yxy> &values)
{
    mCalibRecordDictItemData->measuredYxy = values;
}

// -----------------------------------
//
std::vector<std::string> 
Ookala::CalibRecordDictItem::getLutNames()
{
    std::vector<std::string> names;

    for (std::map<std::string, std::vector<uint32_t> >::iterator i = 
              mCalibRecordDictItemData->luts.begin();
         i != mCalibRecordDictItemData->luts.end(); ++i) {
        names.push_back( (*i).first );
    }

    return names;
}

// -----------------------------------
//
std::vector<uint32_t>    
Ookala::CalibRecordDictItem::getLut(const std::string &key)
{
    std::map<std::string, std::vector<uint32_t> >::iterator theIter;

    theIter = mCalibRecordDictItemData->luts.find(key);

    if (theIter == mCalibRecordDictItemData->luts.end()) {
        std::vector<uint32_t> nothing;

        return nothing;
    }

    return (*theIter).second;
}

// -----------------------------------
//
void                     
Ookala::CalibRecordDictItem::setLut(
                            const std::string           &key, 
                            const std::vector<uint32_t> &values)
{
    mCalibRecordDictItemData->luts[key] = values;
}



// -----------------------------------
//
// virtual
bool
Ookala::CalibRecordDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    StringDictItem      stringItem;
    IntDictItem         intItem;
    DoubleArrayDictItem doubleArrayItem;
    IntArrayDictItem    intArrayItem;

    std::vector<double> doubleVals;


    // deviceId
    stringItem.set(mCalibRecordDictItemData->deviceId);
    serializeSubItem(doc, root, "deviceId", &stringItem);


    // calibrationTime
    intItem.set(mCalibRecordDictItemData->calibrationTime);
    serializeSubItem(doc, root, "calibrationTime", &intItem);


    // calibrationPluginName
    stringItem.set(mCalibRecordDictItemData->calibrationPluginName);
    serializeSubItem(doc, root, "calibrationPluginName", &stringItem);

    // preset
    intItem.set(mCalibRecordDictItemData->preset);
    serializeSubItem(doc, root, "preset", &intItem);


    // presetName
    stringItem.set(mCalibRecordDictItemData->presetName);
    serializeSubItem(doc, root, "presetName", &stringItem);


    // targetWhite
    doubleVals.clear();
    doubleVals.push_back(mCalibRecordDictItemData->targetWhite.Y);
    doubleVals.push_back(mCalibRecordDictItemData->targetWhite.x);
    doubleVals.push_back(mCalibRecordDictItemData->targetWhite.y);
    doubleArrayItem.set(doubleVals);
    serializeSubItem(doc, root, "targetWhite", &doubleArrayItem);


    // targetRed
    doubleVals.clear();
    doubleVals.push_back(mCalibRecordDictItemData->targetRed.Y);
    doubleVals.push_back(mCalibRecordDictItemData->targetRed.x);
    doubleVals.push_back(mCalibRecordDictItemData->targetRed.y);
    doubleArrayItem.set(doubleVals);
    serializeSubItem(doc, root, "targetRed", &doubleArrayItem);


    // targetGreen
    doubleVals.clear();
    doubleVals.push_back(mCalibRecordDictItemData->targetGreen.Y);
    doubleVals.push_back(mCalibRecordDictItemData->targetGreen.x);
    doubleVals.push_back(mCalibRecordDictItemData->targetGreen.y);
    doubleArrayItem.set(doubleVals);
    serializeSubItem(doc, root, "targetGreen", &doubleArrayItem);

    // targetBlue
    doubleVals.clear();
    doubleVals.push_back(mCalibRecordDictItemData->targetBlue.Y);
    doubleVals.push_back(mCalibRecordDictItemData->targetBlue.x);
    doubleVals.push_back(mCalibRecordDictItemData->targetBlue.y);
    doubleArrayItem.set(doubleVals);
    serializeSubItem(doc, root, "targetBlue", &doubleArrayItem);

    // measuredWhite
    doubleVals.clear();
    doubleVals.push_back(mCalibRecordDictItemData->measuredWhite.Y);
    doubleVals.push_back(mCalibRecordDictItemData->measuredWhite.x);
    doubleVals.push_back(mCalibRecordDictItemData->measuredWhite.y);
    doubleArrayItem.set(doubleVals);
    serializeSubItem(doc, root, "measuredWhite", &doubleArrayItem);


    // measuredRed
    doubleVals.clear();
    doubleVals.push_back(mCalibRecordDictItemData->measuredRed.Y);
    doubleVals.push_back(mCalibRecordDictItemData->measuredRed.x);
    doubleVals.push_back(mCalibRecordDictItemData->measuredRed.y);
    doubleArrayItem.set(doubleVals);
    serializeSubItem(doc, root, "measuredRed", &doubleArrayItem);


    // measuredGreen
    doubleVals.clear();
    doubleVals.push_back(mCalibRecordDictItemData->measuredGreen.Y);
    doubleVals.push_back(mCalibRecordDictItemData->measuredGreen.x);
    doubleVals.push_back(mCalibRecordDictItemData->measuredGreen.y);
    doubleArrayItem.set(doubleVals);
    serializeSubItem(doc, root, "measuredGreen", &doubleArrayItem);

    // measuredBlue
    doubleVals.clear();
    doubleVals.push_back(mCalibRecordDictItemData->measuredBlue.Y);
    doubleVals.push_back(mCalibRecordDictItemData->measuredBlue.x);
    doubleVals.push_back(mCalibRecordDictItemData->measuredBlue.y);
    doubleArrayItem.set(doubleVals);
    serializeSubItem(doc, root, "measuredBlue", &doubleArrayItem);


    // measuredRgb
    doubleVals.clear();
    for (std::vector<Rgb>::iterator theRgbVal = 
                         mCalibRecordDictItemData->measuredRgb.begin();
            theRgbVal != mCalibRecordDictItemData->measuredRgb.end(); 
                                                    ++theRgbVal) {
        doubleVals.push_back((*theRgbVal).r);
        doubleVals.push_back((*theRgbVal).g);
        doubleVals.push_back((*theRgbVal).b);
    }
    if (!doubleVals.empty()) {
        doubleArrayItem.set(doubleVals);
        serializeSubItem(doc, root, "measuredRgb", &doubleArrayItem);
    }

    // measuredYxy
    doubleVals.clear();
    for (std::vector<Yxy>::iterator theYxyVal = 
                         mCalibRecordDictItemData->measuredYxy.begin();
            theYxyVal != mCalibRecordDictItemData->measuredYxy.end(); 
                                                    ++theYxyVal) {
        doubleVals.push_back((*theYxyVal).Y);
        doubleVals.push_back((*theYxyVal).x);
        doubleVals.push_back((*theYxyVal).y);
    }
    if (!doubleVals.empty()) {
        doubleArrayItem.set(doubleVals);
        serializeSubItem(doc, root, "measuredYxy", &doubleArrayItem);
    }

    // luts
    for (std::map<std::string, std::vector<uint32_t> >::iterator theLut = 
                          mCalibRecordDictItemData->luts.begin();
                theLut != mCalibRecordDictItemData->luts.end(); ++theLut) {
        if (! (*theLut).second.empty()) {
            xmlNodePtr           itemNode;
            std::vector<int32_t> intVals;

            for (std::vector<uint32_t>::iterator theVal = (*theLut).second.begin();
                    theVal != (*theLut).second.end(); ++theVal) {
                intVals.push_back(*theVal);
            }

            intArrayItem.set(intVals);

            itemNode = xmlNewTextChild(root, NULL, (const xmlChar *)"dictitem", NULL);
            xmlSetProp(itemNode, (const xmlChar *)"name", (const xmlChar *)"luts");
            xmlSetProp(itemNode, (const xmlChar *)"key",  
                                            (const xmlChar *)(*theLut).first.c_str());

            xmlSetProp(itemNode, (const xmlChar *)"type", 
                                 (const xmlChar *)intArrayItem.itemType().c_str());
            intArrayItem.serialize(doc, itemNode);
        }
    }



    return true;
}

// -----------------------------------
//
// virtual
bool
Ookala::CalibRecordDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    xmlNodePtr           node     = NULL;
    xmlChar             *attrName = NULL;
    IntDictItem          intItem;
    StringDictItem       stringItem;
    DoubleArrayDictItem  doubleArrayItem;
    IntArrayDictItem     intArrayItem;

    std::vector<double>  doubleVals;
    std::vector<int32_t> intVals;


    // Don't check type here, so we can re-use this for derived classes.

    printf("Loading calibRecord...\n");

    node = root->xmlChildrenNode;
    while (node != NULL) {
        std::string nodeName;
        std::string nodeKey;


        if (node->name != NULL) {
            if (!strcmp((char *)(node->name), "dictitem")) {
                attrName = xmlGetProp(node, (const xmlChar *)("name"));
                if (attrName != NULL) {
                    nodeName = (char *)(attrName);
                    xmlFree(attrName);
                }

                attrName = xmlGetProp(node, (const xmlChar *)("key"));
                if (attrName != NULL) {
                    nodeKey = (char *)(attrName);
                    xmlFree(attrName);
                }
            }
        }

        // deviceId
        if (nodeName == "deviceId") {
            if (!stringItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::deviceId\n");
            } else {
                mCalibRecordDictItemData->deviceId = stringItem.get();
            }
        }


        // calibratonTime  
        if (nodeName == "calibrationTime") {
            if (!intItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::calibrationTime\n");
            } else {
                mCalibRecordDictItemData->calibrationTime = intItem.get();
            }
        }

        // calibrationPluginName
        if (nodeName == "calibrationPluginName") {
            if (!stringItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::calibrationPluginName\n");
            } else {
                mCalibRecordDictItemData->calibrationPluginName = stringItem.get();
            }
        }

        // preset
        if (nodeName == "preset") {
            if (!intItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::preset\n");
            } else {
                mCalibRecordDictItemData->preset = intItem.get();
            }
        }

        // presetName
        if (nodeName == "presetName") {
            if (!stringItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::presetName\n");
            } else {
                mCalibRecordDictItemData->presetName = stringItem.get();
            }
        }


        // targetWhite
        if (nodeName == "targetWhite") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::targetWhite\n");
            } else {
                doubleVals = doubleArrayItem.get();

                if (doubleVals.size() != 3) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::targetWhite\n");
                } else {
                    mCalibRecordDictItemData->targetWhite.Y = doubleVals[0];
                    mCalibRecordDictItemData->targetWhite.x = doubleVals[1];
                    mCalibRecordDictItemData->targetWhite.y = doubleVals[2];
                }
            }
        }



        // targetRed
        if (nodeName == "targetRed") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::targetRed\n");
            } else {
                doubleVals = doubleArrayItem.get();

                if (doubleVals.size() != 3) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::targetRed\n");
                } else {
                    mCalibRecordDictItemData->targetRed.Y = doubleVals[0];
                    mCalibRecordDictItemData->targetRed.x = doubleVals[1];
                    mCalibRecordDictItemData->targetRed.y = doubleVals[2];
                }
            }
        }

        // targetGreen
        if (nodeName == "targetGreen") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::targetGreen\n");
            } else {

                doubleVals = doubleArrayItem.get();

                if (doubleVals.size() != 3) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::targetGreen\n");
                } else {
                    mCalibRecordDictItemData->targetGreen.Y = doubleVals[0];
                    mCalibRecordDictItemData->targetGreen.x = doubleVals[1];
                    mCalibRecordDictItemData->targetGreen.y = doubleVals[2];
                }
            }
        }


        // targetBlue
        if (nodeName == "targetBlue") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::targetBlue\n");
            } else {

                doubleVals = doubleArrayItem.get();

                if (doubleVals.size() != 3) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::targetBlue\n");
                } else {
                    mCalibRecordDictItemData->targetBlue.Y = doubleVals[0];
                    mCalibRecordDictItemData->targetBlue.x = doubleVals[1];
                    mCalibRecordDictItemData->targetBlue.y = doubleVals[2];
                }
            }
        }

        // measuredWhite
        if (nodeName == "measuredWhite") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::measuredWhite\n");
            } else {

                doubleVals = doubleArrayItem.get();

                if (doubleVals.size() != 3) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::measuredWhite\n");
                } else {
                    mCalibRecordDictItemData->measuredWhite.Y = doubleVals[0];
                    mCalibRecordDictItemData->measuredWhite.x = doubleVals[1];
                    mCalibRecordDictItemData->measuredWhite.y = doubleVals[2];
                }
            }
        }

        // measuredRed
        if (nodeName == "measuredRed") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::measuredRed\n");
            } else {
                doubleVals = doubleArrayItem.get();

                if (doubleVals.size() != 3) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::measuredRed\n");
                } else {
                    mCalibRecordDictItemData->measuredRed.Y = doubleVals[0];
                    mCalibRecordDictItemData->measuredRed.x = doubleVals[1];
                    mCalibRecordDictItemData->measuredRed.y = doubleVals[2];
                }
            }
        }

        // measuredGreen
        if (nodeName == "measuredGreen") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::measuredGreen\n");
            } else {
                doubleVals = doubleArrayItem.get();

                if (doubleVals.size() != 3) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::measuredGreen\n");
                } else {
                    mCalibRecordDictItemData->measuredGreen.Y = doubleVals[0];
                    mCalibRecordDictItemData->measuredGreen.x = doubleVals[1];
                    mCalibRecordDictItemData->measuredGreen.y = doubleVals[2];
                }
            }
        }


        // measuredBlue
        if (nodeName == "measuredBlue") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::measuredBlue\n");
            } else {
                doubleVals = doubleArrayItem.get();

                if (doubleVals.size() != 3) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::measuredBlue\n");
                } else {
                    mCalibRecordDictItemData->measuredBlue.Y = doubleVals[0];
                    mCalibRecordDictItemData->measuredBlue.x = doubleVals[1];
                    mCalibRecordDictItemData->measuredBlue.y = doubleVals[2];
                }
            }
        }

        // measuredRgb
        if (nodeName == "measuredRgb") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::measuredBlue\n");
            } else {
                doubleVals = doubleArrayItem.get();

                mCalibRecordDictItemData->measuredRgb.clear();
                for (uint32_t idx=0; idx<doubleVals.size(); idx+=3) {
                    Rgb rgb;

                    if (3*idx   < doubleVals.size())  rgb.r = doubleVals[3*idx];
                    if (3*idx+1 < doubleVals.size())  rgb.g = doubleVals[3*idx+1];
                    if (3*idx+2 < doubleVals.size())  rgb.b = doubleVals[3*idx+2];

                    mCalibRecordDictItemData->measuredRgb.push_back(rgb);
                }
            }
        }

        // measuredYxy
        if (nodeName == "measuredYxy") {
            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing CalibRecordDictItem::measuredBlue\n");
            } else {

                doubleVals = doubleArrayItem.get();

                mCalibRecordDictItemData->measuredYxy.clear();
                for (uint32_t idx=0; idx<doubleVals.size(); idx+=3) {
                    Yxy val;

                    if (3*idx   < doubleVals.size())  val.Y = doubleVals[3*idx];
                    if (3*idx+1 < doubleVals.size())  val.x = doubleVals[3*idx+1];
                    if (3*idx+2 < doubleVals.size())  val.y = doubleVals[3*idx+2];

                    mCalibRecordDictItemData->measuredYxy.push_back(val);
                }
            }
        }

        // luts
        if (nodeName == "luts") {

            if (nodeKey.empty()) {
                fprintf(stderr, "Error parsing CalibRecordDictItem::luts; No key\n");
            } else {
                if (!intArrayItem.unserialize(doc, node)) {
                    fprintf(stderr, 
                        "Error parsing CalibRecordDictItem::luts\n");
                } else {

                    intVals = intArrayItem.get();

                    std::vector<uint32_t> newVals;

                    for (std::vector<int32_t>::iterator theVal = intVals.begin();
                            theVal != intVals.end(); ++theVal) {
                        newVals.push_back(*theVal);
                    }

                    mCalibRecordDictItemData->luts[nodeKey] = newVals;
                }
            }
        }


        node = node->next;
    }


    return true;
}

// -----------------------------------
//
// virtual
void
Ookala::CalibRecordDictItem::debug()
{
    printf("deviceId: %s\n", mCalibRecordDictItemData->deviceId.c_str());
    printf("calibrationTime: %d\n", mCalibRecordDictItemData->calibrationTime);
    printf("calibrationPluginName: %s\n", mCalibRecordDictItemData->calibrationPluginName.c_str());
    printf("preset: %d\n", mCalibRecordDictItemData->preset);
    printf("presetName: %s\n", mCalibRecordDictItemData->presetName.c_str());
    printf("targetWhite: %f %f %f\n", mCalibRecordDictItemData->targetWhite.Y, 
                                      mCalibRecordDictItemData->targetWhite.x, 
                                      mCalibRecordDictItemData->targetWhite.y);
    printf("targetRed: %f %f %f\n",   mCalibRecordDictItemData->targetRed.Y,   
                                      mCalibRecordDictItemData->targetRed.x,   
                                      mCalibRecordDictItemData->targetRed.y);
    printf("targetGreen: %f %f %f\n", mCalibRecordDictItemData->targetGreen.Y, 
                                      mCalibRecordDictItemData->targetGreen.x, 
                                      mCalibRecordDictItemData->targetGreen.y);
    printf("targetBlue: %f %f %f\n",  mCalibRecordDictItemData->targetBlue.Y,  
                                      mCalibRecordDictItemData->targetBlue.x,  
                                      mCalibRecordDictItemData->targetBlue.y);

    printf("measuredWhite: %f %f %f\n", mCalibRecordDictItemData->measuredWhite.Y, 
                                        mCalibRecordDictItemData->measuredWhite.x, 
                                        mCalibRecordDictItemData->measuredWhite.y);
    printf("measuredRed: %f %f %f\n",   mCalibRecordDictItemData->measuredRed.Y,   
                                        mCalibRecordDictItemData->measuredRed.x,   
                                        mCalibRecordDictItemData->measuredRed.y);
    printf("measuredGreen: %f %f %f\n", mCalibRecordDictItemData->measuredGreen.Y, 
                                        mCalibRecordDictItemData->measuredGreen.x, 
                                        mCalibRecordDictItemData->measuredGreen.y);
    printf("measuredBlue: %f %f %f\n",  mCalibRecordDictItemData->measuredBlue.Y,  
                                        mCalibRecordDictItemData->measuredBlue.x,  
                                        mCalibRecordDictItemData->measuredBlue.y);

    printf("Sampled Data:\n");
    
    uint32_t max = mCalibRecordDictItemData->measuredRgb.size();
    if (mCalibRecordDictItemData->measuredYxy.size() > max) {
        max = mCalibRecordDictItemData->measuredYxy.size();
    }

    for (uint32_t i=0; i<max; ++i) {
        if (i < mCalibRecordDictItemData->measuredRgb.size()) {
            printf("%.5f %.5f %.5f -> ", 
                mCalibRecordDictItemData->measuredRgb[i].r, 
                mCalibRecordDictItemData->measuredRgb[i].g, 
                mCalibRecordDictItemData->measuredRgb[i].b);
        } else {
            printf("                       -> "); 
        }

        if (i < mCalibRecordDictItemData->measuredYxy.size()) {
            printf("%.5f %.5f %.5f\n",
                mCalibRecordDictItemData->measuredYxy[i].Y, 
                mCalibRecordDictItemData->measuredYxy[i].x, 
                mCalibRecordDictItemData->measuredYxy[i].x);
        } else {
            printf("\n"); 
        }
    }
}


// -----------------------------------
//
// protected
bool
Ookala::CalibRecordDictItem::serializeSubItem(
                    xmlDocPtr   doc,  xmlNodePtr root,
                    std::string name, DictItem *item)
{
    xmlNodePtr          itemNode;

    itemNode = xmlNewTextChild(root, NULL, (const xmlChar *)"dictitem", NULL);
    xmlSetProp(itemNode, (const xmlChar *)"name", (const xmlChar *)name.c_str());
    xmlSetProp(itemNode, (const xmlChar *)"type", 
                         (const xmlChar *)item->itemType().c_str());
    item->serialize(doc, itemNode);

    return true;
}



// ===================================
// 
// DataSavior
//
// -----------------------------------
//
Ookala::DataSavior::DataSavior():
    Plugin()
{
    setName("DataSavior");
}

// -----------------------------------
//
Ookala::DataSavior::DataSavior(const DataSavior &src):
    Plugin(src)
{

}

// -----------------------------------
//
// virtual
Ookala::DataSavior::~DataSavior()
{
}

// ----------------------------
//
Ookala::DataSavior &
Ookala::DataSavior::operator=(const DataSavior &src)
{
    if (this != &src) {
        Plugin::operator=(src);
    }

    return *this;
}


// -----------------------------------
//
// virtual
bool
Ookala::DataSavior::save(std::vector<Dict *>      dicts,
                         std::vector<std::string> filenames)
{
printf("DataSavior::save() - %d elements\n", (int)dicts.size());

    xmlDocPtr doc = xmlNewDoc((const xmlChar *)("1.0"));
    if (doc == NULL) {
        setErrorString("Error creating XML document.");
        return false;
    }

    xmlNodePtr cur = xmlNewNode(NULL,(const xmlChar *)("DataSavior"));

    xmlDocSetRootElement(doc, cur);

    for (std::vector<Dict *>::iterator theDict = dicts.begin();
            theDict != dicts.end(); ++theDict) {
        (*theDict)->serialize(doc, cur);
    }
    
    // We want to save the same data to multiple places. For example,
    // we might want to save a local copy and copy on a network
    // mount. The net-mount would be useful for book-keeping by
    // admins, while the local version would be useful if we 
    // get disconnected from the net-mount.

    for (std::vector<std::string>::iterator theName = filenames.begin();
            theName != filenames.end(); ++theName) {
        std::string finalName = getFilename(*theName);

        xmlSaveFormatFile(finalName.c_str(), doc, 1);
    }


    xmlFreeDoc(doc);


    return true;
}

// -----------------------------------
//
// virtual
bool
Ookala::DataSavior::load(std::vector<std::string> filenames)
{
    xmlDocPtr              doc;
    xmlNodePtr             root, child;
    DictHash              *hash = NULL;
    std::vector<Plugin *>  plugins;

    if (filenames.empty()) {
        setErrorString("No file names to load.");
        return false;
    }

    if (mRegistry == NULL) {
        setErrorString("No PluginRegistry in DataSavior.");
        return false;
    }


    // Choose the newest file to deal with

    std::string newestFilename = "";
    time_t      newestMTime    = 0;
    for (std::vector<std::string>::iterator theName = filenames.begin();
            theName != filenames.end(); ++theName) {
        std::string realFilename = getFilename(*theName);

#ifdef WIN32
        struct _stat statbuf;
        if (_stat(realFilename.c_str(), &statbuf) != 0) {
#else
        struct stat statbuf;
        if (stat(realFilename.c_str(), &statbuf) != 0) {
#endif
            continue;
        }

        if (statbuf.st_mtime > newestMTime) {
            newestMTime    = statbuf.st_mtime;
            newestFilename = realFilename;
        }
    }

    if ((newestFilename == "")        || 
         (newestFilename.size() == 0) ||
         (newestMTime == 0)) {

        setErrorString("No files found.");
        return false;
    }

    printf("Loading %s...\n", newestFilename.c_str());

    plugins = mRegistry->queryByName("DictHash");
    if (plugins.empty()) {
        fprintf(stderr, "No DictHash found\n");
        return false;
    }

    hash = dynamic_cast<DictHash *>(plugins[0]);
    if (!hash) {
        fprintf(stderr, "No DictHash found\n");
        return false;
    }
 
    doc = xmlParseFile(newestFilename.c_str());
    if (doc == NULL) {
        setErrorString(std::string("Can't open ") + newestFilename);
        return false;
    }

    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        setErrorString(std::string("No root element in ") + newestFilename);
        return false;
    }

    child = root->xmlChildrenNode;
    while (child != NULL) {

        std::string nodeName((char *)child->name);
        if (nodeName == "dict") {
            
            std::string dictName;

            xmlChar *attrName = xmlGetProp(child, (const xmlChar *)("name"));
            if (attrName != NULL) {
                dictName = (char *)(attrName);
                xmlFree(attrName);
            }

            Dict *dict = hash->newDict(dictName.c_str());
            if (!dict) {
                setErrorString(std::string("Unable to create dict in ")
                                                         + newestFilename);
                return false;
            }

            if (!dict->unserialize(doc, child)) {
                setErrorString(std::string("Error unserializing dict in ")
                                                         + newestFilename);
                return false;
            }

            dict->debug();
        }

        child = child->next;
    }

    xmlFreeDoc(doc);

    return true;
}

// -----------------------------------
//
// Perform any string substitution on file names that we
// get as input to form proper file names
//
// protected
std::string
Ookala::DataSavior::getFilename(std::string filePattern)
{
    std::string replaceSrc("%s");
    std::string replaceDst;
    std::string replaced, orig;
    char        buf[1024];

    gethostname(buf, 1023);
    replaceDst = buf;

    orig = filePattern;

    size_t start = filePattern.find(replaceSrc);
    if (start == std::string::npos) {
        return filePattern;
    }

    replaced = orig.replace(start, replaceSrc.size(), replaceDst);

    printf("%s -> %s\n", filePattern.c_str(), replaced.c_str());

    return replaced;
}


// -----------------------------------
//
// This responds to commands found in the chain dict. IT
// will look for:
//
//    DataSavior::save      [bool]
//    DataSavior::load      [bool]
//    DataSavior::fileNames [stringArray]
//    DataSavior::dictNames [stringArray]
//      + When saving, save out these dicts.
//
// Order of operations is:
//    1) load
//    2) save
//
// So then you can re-sync data files by issuing a command
// to both load and save.
//
// virtual protected
bool
Ookala::DataSavior::_run(PluginChain *chain)
{
    DictHash            *hash            = NULL;
    Dict                *chainDict       = NULL;
    Dict                *dict            = NULL;
    DictItem            *item            = NULL;
    BoolDictItem        *boolItem        = NULL;
    StringArrayDictItem *stringArrayItem = NULL;

    bool                     doSave      = false;
    bool                     doLoad      = false;

    std::vector<std::string> fileNames;
    std::vector<std::string> dictNames;
    std::vector<Dict *>      saveDicts;


    setErrorString("");

    if (!mRegistry) {
        setErrorString("No PluginRegistry set for DataSavior.");
        return false;
    }
    if (!chain) {
        return true;
    }

    std::vector<Plugin *> plugins = mRegistry->queryByName("DictHash");
    if (plugins.empty()) {
        setErrorString("No DictHash plugin found.");
        return false;
    }
    hash = dynamic_cast<DictHash *>(plugins[0]);
    if (!hash) {
        setErrorString("No DictHash plugin found.");
        return false;
    }
    
    chainDict = hash->getDict(chain->getDictName().c_str());
    if (!chainDict) {
        setErrorString("No chain dict found.");
        return false;
    }

    // If we're going to do anything, we need filenames.
    item = chainDict->get("DataSavior::fileNames");
    if (!item) {
        setErrorString("DataSavior::fileNames [stringArray] not found.");
        return false;
    }
    stringArrayItem = dynamic_cast<StringArrayDictItem *>(item);
    if (!stringArrayItem) {
        setErrorString("DataSavior::fileNames [stringArray] not found.");
        return false;
    }
    fileNames = stringArrayItem->get();


    // Check if we're supposed to save
    doSave = false;
    item = chainDict->get("DataSavior::save");
    if (item) {
        boolItem = dynamic_cast<BoolDictItem *>(item);
        if (boolItem) {
            doSave = boolItem->get();
        }
    }

    // And check if we're supposed to load
    doLoad = false;
    item = chainDict->get("DataSavior::load");
    if (item) {
        boolItem = dynamic_cast<BoolDictItem *>(item);
        if (boolItem) {
            doLoad = boolItem->get();
        }
    }

    // If we're supposed to save, we'll need to have a list
    // of dictNames to save.
    if (doSave) {
        item = chainDict->get("DataSavior::dictNames");
        if (!item) {
            setErrorString("DataSavior::dictNames [stringArray] not found.");
            return false;
        }
        stringArrayItem = dynamic_cast<StringArrayDictItem *>(item);
        if (!stringArrayItem) {
            setErrorString("DataSavior::dictNames [stringArray] not found.");
            return false;
        }
        dictNames = stringArrayItem->get();        

        for (std::vector<std::string>::iterator theName = dictNames.begin();
                theName != dictNames.end(); ++theName) {
            dict = hash->getDict((*theName).c_str());
        
            if (dict) {
                saveDicts.push_back(dict);
            }
        }
        
    }
    
    // Load should happen before save, as then we can re-sync data files
    // by issuing both a load and a save
    if (doLoad) {

        if (!load(fileNames)) {
            return false;
        }
    }

    if (doSave) {
        if (!save(saveDicts, fileNames)) {
            return false;
        }
    }
 

    return true;
}






