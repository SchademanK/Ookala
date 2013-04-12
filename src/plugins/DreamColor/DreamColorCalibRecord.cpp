// --------------------------------------------------------------------------
// $Id: DreamColorCalibRecord.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "DreamColorCalibRecord.h"

// ==========================================
//
//  _DreamColorCalibRecord
// 
// ------------------------------------------

struct Ookala::_DreamColorCalibRecord
{
    uint32_t brightnessReg;
};  

// ==========================================
//
//  DreamColorCalibRecord
// 
// ------------------------------------------

Ookala::DreamColorCalibRecord::DreamColorCalibRecord():
    CalibRecordDictItem()
{
    setItemType("DreamColorCalibRecord");
    mIsSerializable = true;

    mDreamColorCalibRecordData = new _DreamColorCalibRecord;
}


// ------------------------------------------
//

Ookala::DreamColorCalibRecord::DreamColorCalibRecord(
                        const DreamColorCalibRecord &src):
    CalibRecordDictItem(src)
{
    setItemType("DreamColorCalibRecord");
    mIsSerializable = true;

    mDreamColorCalibRecordData = new _DreamColorCalibRecord;

    if (src.mDreamColorCalibRecordData) {
        mDreamColorCalibRecordData->brightnessReg = 
            src.mDreamColorCalibRecordData->brightnessReg;
    }   
}

// ------------------------------------------
//
// virtual

Ookala::DreamColorCalibRecord::~DreamColorCalibRecord()
{
    if (mDreamColorCalibRecordData) {
        delete mDreamColorCalibRecordData;
        mDreamColorCalibRecordData = NULL;
    }
}

// ------------------------------------------
//

Ookala::DreamColorCalibRecord & 
Ookala::DreamColorCalibRecord::operator=(
                      const DreamColorCalibRecord &src)
{
    if (this != &src) {
        CalibRecordDictItem::operator=(src);

        if (mDreamColorCalibRecordData) {
            delete mDreamColorCalibRecordData;
            mDreamColorCalibRecordData = NULL;
        }

        if (src.mDreamColorCalibRecordData) {
            mDreamColorCalibRecordData = new _DreamColorCalibRecord;
            mDreamColorCalibRecordData->brightnessReg = 
                src.mDreamColorCalibRecordData->brightnessReg;
        }   
    }
        
    return *this;
}

// ------------------------------------------
//
// virtual
bool 
Ookala::DreamColorCalibRecord::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    IntDictItem         intItem;

    if (!CalibRecordDictItem::serialize(doc, root)) {
        return false;
    }

    intItem.set(mDreamColorCalibRecordData->brightnessReg);
    serializeSubItem(doc, root, "brightnessReg", &intItem);

    return true;
}

// ------------------------------------------
//
// virtual
bool
Ookala::DreamColorCalibRecord::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    IntDictItem         intItem;
    xmlNodePtr          node     = NULL;
    xmlChar             *attrName = NULL;

    if (!CalibRecordDictItem::unserialize(doc, root)) {
        return false;
    }    

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

        // brightnessReg
        if (nodeName == "brightnessReg") {
            if (!intItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorCalibRecord::brightnessReg\n");
            } else {
                mDreamColorCalibRecordData->brightnessReg = intItem.get();
            }
        }


        node = node->next;
    }

    return true;
}

// ------------------------------------------
//
// virtual
bool
Ookala::DreamColorCalibRecord::setBrightnessReg(uint32_t value)
{
    if (!mDreamColorCalibRecordData) {
        return false;
    }

    mDreamColorCalibRecordData->brightnessReg = value;

    return true;
}

// ------------------------------------------
//
// virtual
uint32_t
Ookala::DreamColorCalibRecord::getBrightnessReg()
{
    if (!mDreamColorCalibRecordData) {
        return 0;
    }

    return mDreamColorCalibRecordData->brightnessReg;
}

