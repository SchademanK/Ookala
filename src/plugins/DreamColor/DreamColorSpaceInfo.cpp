// --------------------------------------------------------------------------
// $Id: DreamColorSpaceInfo.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Dict.h"

#include "DreamColorSpaceInfo.h"

// ====================================
//
// DreamColorSpaceInfo
//
// ------------------------------------

Ookala::DreamColorSpaceInfo::DreamColorSpaceInfo():
    DictItem()
{
    setItemType("DreamColorSpaceInfo");
    mIsSerializable = true;

    mDreamColorSpaceInfoData = new _DreamColorSpaceInfo;

    mDreamColorSpaceInfoData->connId   = 0;
    mDreamColorSpaceInfoData->presetId = 0;

    mDreamColorSpaceInfoData->enabled                 = true;
    mDreamColorSpaceInfoData->calibrated              = true;
    mDreamColorSpaceInfoData->calibratedTimeRemaining = 1000;
    mDreamColorSpaceInfoData->name                    = "foo";

    mDreamColorSpaceInfoData->white.x = 0.313;
    mDreamColorSpaceInfoData->white.y = 0.329;
    mDreamColorSpaceInfoData->white.Y = 50.0;

    mDreamColorSpaceInfoData->red.x   = 0.64;
    mDreamColorSpaceInfoData->red.y   = 0.33;

    mDreamColorSpaceInfoData->green.x = 0.30;
    mDreamColorSpaceInfoData->green.y = 0.60;

    mDreamColorSpaceInfoData->blue.x  = 0.15;
    mDreamColorSpaceInfoData->blue.y  = 0.06;

    mDreamColorSpaceInfoData->trcGamma = 2.4;
    mDreamColorSpaceInfoData->trcA0    = 0.04045;
    mDreamColorSpaceInfoData->trcA1    = 12.92;
    mDreamColorSpaceInfoData->trcA2    = 0.055;
    mDreamColorSpaceInfoData->trcA3    = 0.055;
}

// -------------------------------------
//
Ookala::DreamColorSpaceInfo::DreamColorSpaceInfo(
                        const DreamColorSpaceInfo &src):
    DictItem(src)
{
    
    mDreamColorSpaceInfoData = new _DreamColorSpaceInfo;

    if (src.mDreamColorSpaceInfoData) {

        mDreamColorSpaceInfoData->calibRecordName         =     
                 src.mDreamColorSpaceInfoData->calibRecordName;

        mDreamColorSpaceInfoData->connId                  =    
                     src.mDreamColorSpaceInfoData->connId;

        mDreamColorSpaceInfoData->presetId                = 
                                    src.mDreamColorSpaceInfoData->presetId;

        mDreamColorSpaceInfoData->enabled                 = 
                                    src.mDreamColorSpaceInfoData->enabled;

        mDreamColorSpaceInfoData->calibrated              = 
                                    src.mDreamColorSpaceInfoData->calibrated;

        mDreamColorSpaceInfoData->calibratedTimeRemaining = 
                                    src.mDreamColorSpaceInfoData->calibratedTimeRemaining;

        mDreamColorSpaceInfoData->name                    = 
                                    src.mDreamColorSpaceInfoData->name;
        
        mDreamColorSpaceInfoData->white = src.mDreamColorSpaceInfoData->white;
        mDreamColorSpaceInfoData->red   = src.mDreamColorSpaceInfoData->red;
        mDreamColorSpaceInfoData->green = src.mDreamColorSpaceInfoData->green;
        mDreamColorSpaceInfoData->blue  = src.mDreamColorSpaceInfoData->blue;

        mDreamColorSpaceInfoData->trcGamma = src.mDreamColorSpaceInfoData->trcGamma;
        mDreamColorSpaceInfoData->trcA0    = src.mDreamColorSpaceInfoData->trcA0;
        mDreamColorSpaceInfoData->trcA1    = src.mDreamColorSpaceInfoData->trcA1;
        mDreamColorSpaceInfoData->trcA2    = src.mDreamColorSpaceInfoData->trcA2;
        mDreamColorSpaceInfoData->trcA3    = src.mDreamColorSpaceInfoData->trcA3;

    } else {

        mDreamColorSpaceInfoData->connId   = 0;
        mDreamColorSpaceInfoData->presetId = 0;

        mDreamColorSpaceInfoData->enabled                 = true;
        mDreamColorSpaceInfoData->calibrated              = true;
        mDreamColorSpaceInfoData->calibratedTimeRemaining = 1000;
        mDreamColorSpaceInfoData->name                    = "foo";

        mDreamColorSpaceInfoData->white.x = 0.313;
        mDreamColorSpaceInfoData->white.y = 0.329;
        mDreamColorSpaceInfoData->white.Y = 50.0;

        mDreamColorSpaceInfoData->red.x   = 0.64;
        mDreamColorSpaceInfoData->red.y   = 0.33;

        mDreamColorSpaceInfoData->green.x = 0.30;
        mDreamColorSpaceInfoData->green.y = 0.60;

        mDreamColorSpaceInfoData->blue.x  = 0.15;
        mDreamColorSpaceInfoData->blue.y  = 0.06;

        mDreamColorSpaceInfoData->trcGamma = 2.4;
        mDreamColorSpaceInfoData->trcA0    = 0.04045;
        mDreamColorSpaceInfoData->trcA1    = 12.92;
        mDreamColorSpaceInfoData->trcA2    = 0.055;
        mDreamColorSpaceInfoData->trcA3    = 0.055;
    }
}

// -------------------------------------
//
// virtual
Ookala::DreamColorSpaceInfo::~DreamColorSpaceInfo()
{
    if (mDreamColorSpaceInfoData) {
        delete mDreamColorSpaceInfoData;
        mDreamColorSpaceInfoData = NULL;
    }
}

// ----------------------------
//
Ookala::DreamColorSpaceInfo &
Ookala::DreamColorSpaceInfo::operator=(const DreamColorSpaceInfo &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        if (mDreamColorSpaceInfoData) {
            delete mDreamColorSpaceInfoData;
            mDreamColorSpaceInfoData = NULL;
        }

        if (src.mDreamColorSpaceInfoData) {
            mDreamColorSpaceInfoData  = new _DreamColorSpaceInfo();
            *mDreamColorSpaceInfoData = *(src.mDreamColorSpaceInfoData);
        }
    }

    return *this;
}


// -------------------------------------
//
// The name of the calibration record to write into when
// we're done calibrating
//
// virtual
std::string 
Ookala::DreamColorSpaceInfo::getCalibRecordName()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->calibRecordName;
    }

    return std::string("");
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setCalibRecordName(const std::string &name)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->calibRecordName = name;
    }
}

// -------------------------------------
//
// virtual
uint32_t    
Ookala::DreamColorSpaceInfo::getConnId()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->connId;
    }

    return 0;
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setConnId(uint32_t conn)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->connId = conn;
    }
}

// -------------------------------------
//
// virtual
uint32_t    
Ookala::DreamColorSpaceInfo::getPresetId()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->presetId;
    }

    return 0;
}


// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setPresetId(uint32_t preset)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->presetId = preset;
    }    
}


// -------------------------------------
//
// virtual
bool        
Ookala::DreamColorSpaceInfo::getEnabled()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->enabled;
    }    

    return false;
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setEnabled(bool enabled)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->enabled = enabled;
    }    
}


// -------------------------------------
//
// virtual
bool        
Ookala::DreamColorSpaceInfo::getCalibrated()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->calibrated;
    }    

    return false;
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setCalibrated(bool calibrated)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->calibrated = calibrated;
    }    
}

// -------------------------------------
//
// virtual
uint32_t    
Ookala::DreamColorSpaceInfo::getCalibratedTimeRemaining()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->calibratedTimeRemaining;
    }    

    return 0;
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setCalibratedTimeRemaining(uint32_t time)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->calibratedTimeRemaining = time;
    }    
}

// -------------------------------------
//
// The name of this preset, to appear in the OSD.
//
// virtual
std::string 
Ookala::DreamColorSpaceInfo::getName()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->name;
    }    

    return std::string("");
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setName(const std::string &name)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->name = name;
    }    
}


// -------------------------------------
//
// virtual
Ookala::Yxy         
Ookala::DreamColorSpaceInfo::getWhite()
{
    Yxy value;

    value.x = 0;
    value.y = 0;
    value.Y = 0;

    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->white;
    }    

    return value;
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setWhite(const Yxy &value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->white = value;
    }
}


// -------------------------------------
//
// virtual
Ookala::Yxy         
Ookala::DreamColorSpaceInfo::getRed()
{
    Yxy value;

    value.x = 0;
    value.y = 0;
    value.Y = 0;

    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->red;
    }    

    return value;
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setRed(const Yxy &value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->red = value;
    }
}

// -------------------------------------
//
// virtual
Ookala::Yxy        
Ookala::DreamColorSpaceInfo::getGreen()
{
    Yxy value;

    value.x = 0;
    value.y = 0;
    value.Y = 0;

    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->green;
    }    

    return value;
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setGreen(const Yxy &value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->green = value;
    }
}

// -------------------------------------
//
// virtual
Ookala::Yxy         
Ookala::DreamColorSpaceInfo::getBlue()
{
    Yxy value;

    value.x = 0;
    value.y = 0;
    value.Y = 0;

    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->blue;
    }    

    return value;
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setBlue(const Yxy &value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->blue = value;
    }
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::getTrc(double &gamma, double &a0, double &a1, 
                                           double &a2, double &a3)
{
    gamma = 1.0;
    a0    = 0;
    a1    = 1;
    a2    = 0;
    a3    = 0;

    if (mDreamColorSpaceInfoData) {
        gamma = mDreamColorSpaceInfoData->trcGamma;        
        a0    = mDreamColorSpaceInfoData->trcA0;        
        a1    = mDreamColorSpaceInfoData->trcA1;        
        a2    = mDreamColorSpaceInfoData->trcA2;        
        a3    = mDreamColorSpaceInfoData->trcA3;        
    }
}

// -------------------------------------
//
// virtual
void        
Ookala::DreamColorSpaceInfo::setTrc(double  gamma, double  a0, double  a1, 
                                           double  a2, double  a3)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->trcGamma = gamma;        
        mDreamColorSpaceInfoData->trcA0 = a0;      
        mDreamColorSpaceInfoData->trcA1 = a1;   
        mDreamColorSpaceInfoData->trcA2 = a2;   
        mDreamColorSpaceInfoData->trcA3 = a3;   
    }
}

// -------------------------------------
//
// virtual
double
Ookala::DreamColorSpaceInfo::getTrcGamma()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->trcGamma;
    }
    return 0;
}

// -------------------------------------
//
// virtual
void
Ookala::DreamColorSpaceInfo::setTrcGamma(double value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->trcGamma = value;
    }
}

// -------------------------------------
//
// virtual
double
Ookala::DreamColorSpaceInfo::getTrcA0()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->trcA0;
    }
    return 0;
}

// -------------------------------------
//
// virtual
void
Ookala::DreamColorSpaceInfo::setTrcA0(double value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->trcA0 = value;
    }
}

// -------------------------------------
//
// virtual
double
Ookala::DreamColorSpaceInfo::getTrcA1()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->trcA1;
    }
    return 0;
}

// -------------------------------------
//
// virtual
void
Ookala::DreamColorSpaceInfo::setTrcA1(double value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->trcA1 = value;
    }
}

// -------------------------------------
//
// virtual
double
Ookala::DreamColorSpaceInfo::getTrcA2()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->trcA2;
    }
    return 0;
}

// -------------------------------------
//
// virtual
void
Ookala::DreamColorSpaceInfo::setTrcA2(double value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->trcA2 = value;
    }
}

// -------------------------------------
//
// virtual
double
Ookala::DreamColorSpaceInfo::getTrcA3()
{
    if (mDreamColorSpaceInfoData) {
        return mDreamColorSpaceInfoData->trcA3;
    }
    return 0;
}

// -------------------------------------
//
// virtual
void
Ookala::DreamColorSpaceInfo::setTrcA3(double value)
{
    if (mDreamColorSpaceInfoData) {
        mDreamColorSpaceInfoData->trcA3 = value;
    }
}




// -------------------------------------
//
// Here, we're loading a composite data type. The format should 
// be:
//      <dictitem type="DreamColorSpaceInfo" name="...">
//
//          <dictitem type="int" name="presetId">
//              <value>0</value>
//          </dictitem>            
//
//          <dictitem type="bool" name="enabled">
//              <value>true</value>
//          </dictitem>            
//
//          ...
//
//
//      </dictitem>
//
// Values we're looking for:
//     name:                    type:
// -----------------------------------
//      calibRecordName          string
//      presetId                 int
//      enabled                  bool
//      calibrated               bool
//      calibratedTimeRemaining  int
//      name                     string
//      white                    double array, Y,x,y
//      red                      double array, x, y
//      green                    double array, x, y
//      blue                     double array, x, y
//      trcGamma                 double
//      trcA0                    double
//      trcA1                    double
//      trcA2                    double
//      trcA3                    double

//
// virtual
bool
Ookala::DreamColorSpaceInfo::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    xmlNodePtr          node     = NULL;
    xmlChar            *attrName = NULL;
    BoolDictItem        boolItem;
    IntDictItem         intItem;
    StringDictItem      stringItem;
    DoubleDictItem      doubleItem;
    DoubleArrayDictItem doubleArrayItem;

    attrName = xmlGetProp(root, (const xmlChar *)("type"));
    if (attrName == NULL) {
        return false;
    }
    if (strcmp((char *)(attrName), "DreamColorSpaceInfo")) {
        return false;
    }

    printf("Loading DreamColorSpaceInfo...\n");

    // Walk over all our children and pull out the values that
    // we care about.

    node = root->xmlChildrenNode;
    while (node != NULL) {
        std::string nodeName;

        if (node->name != NULL) {
            if (!strcmp((char *)(node->name), "dictitem")) {
                attrName = xmlGetProp(node, (const xmlChar *)("name"));
                if (attrName != NULL) {
                    nodeName = (char *)(attrName);
                    xmlFree(attrName);
                }
            }
        }

        if (nodeName == "calibRecordName") {
            if (!stringItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::calibRecordName\n");
                return false;
            }

            setCalibRecordName(stringItem.get());
        }

        if (nodeName == "presetId") {
            if (!intItem.unserialize(doc, node)) {
                fprintf(stderr, "Error parsing DreamColorSpaceInfo::presetId\n");
                return false;
            }

            setPresetId(intItem.get());
        }

        if (nodeName == "enabled") {
            if (!boolItem.unserialize(doc, node)) {
                fprintf(stderr, "Error parsing DreamColorSpaceInfo::enabled\n");
                return false;
            }

            setEnabled(boolItem.get());
        }

        if (nodeName == "calibrated") {
            if (!boolItem.unserialize(doc, node)) {
                fprintf(stderr, "Error parsing DreamColorSpaceInfo::calibrated\n");
                return false;
            }

            setCalibrated(boolItem.get());
        }

        if (nodeName == "calibratedTimeRemaining") {
            if (!intItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::calibratedTimeRemaining\n");
                return false;
            }

            setCalibratedTimeRemaining(intItem.get());
        }

        if (nodeName == "name") {
            if (!stringItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::name\n");
                return false;
            }

            setName(stringItem.get());
        }

        if (nodeName == "white") {
            Yxy white;

            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::white\n");
                return false;
            }

            if (doubleArrayItem.get().size() != 3) {
                fprintf(stderr, 
                    "Insufficient components for DreamColorSpaceInfo::white\n");
                fprintf(stderr, 
                    "\tExpecting Y,x,y\n");
            }
            white.Y = doubleArrayItem.get()[0];
            white.x = doubleArrayItem.get()[1];
            white.y = doubleArrayItem.get()[2];

            setWhite(white);
        }

        if (nodeName == "red") {
            Yxy red;

            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::red\n");
                return false;
            }

            if (doubleArrayItem.get().size() != 2) {
                fprintf(stderr, 
                    "Insufficient components for DreamColorSpaceInfo::red\n");
                fprintf(stderr, 
                    "\tExpecting x,y\n");
            }
            red.Y = 0;
            red.x = doubleArrayItem.get()[0];
            red.y = doubleArrayItem.get()[1];

            setRed(red);
        }

        if (nodeName == "green") {
            Yxy green;

            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::green\n");
                return false;
            }

            if (doubleArrayItem.get().size() != 2) {
                fprintf(stderr, 
                    "Insufficient components for DreamColorSpaceInfo::green\n");
                fprintf(stderr, 
                    "\tExpecting x,y\n");
            }
            green.Y = 0;
            green.x = doubleArrayItem.get()[0];
            green.y = doubleArrayItem.get()[1];

            setGreen(green);
        }

        if (nodeName == "blue") {
            Yxy blue;

            if (!doubleArrayItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::blue\n");
                return false;
            }

            if (doubleArrayItem.get().size() != 2) {
                fprintf(stderr, 
                    "Insufficient components for DreamColorSpaceInfo::blue\n");
                fprintf(stderr, 
                    "\tExpecting x,y\n");
            }
            blue.Y = 0;
            blue.x = doubleArrayItem.get()[0];
            blue.y = doubleArrayItem.get()[1];
    
            setBlue(blue);
        }

        if (nodeName == "trcGamma") {
            double trcA[4], gamma;

            if (!doubleItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::trcGamma\n");
                return false;
            }

            getTrc(gamma,            trcA[0], trcA[1], trcA[2], trcA[3]);
            setTrc(doubleItem.get(), trcA[0], trcA[1], trcA[2], trcA[3]);
        }

        if (nodeName == "trcA0") {
            double trcA[4], gamma;

            if (!doubleItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::trcA0\n");
                return false;
            }

            getTrc(gamma,          trcA[0], trcA[1], trcA[2], trcA[3]);
            setTrc(gamma, doubleItem.get(), trcA[1], trcA[2], trcA[3]);
        }

        if (nodeName == "trcA1") {
            double trcA[4], gamma;

            if (!doubleItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::trcA1\n");
                return false;
            }

            getTrc(gamma, trcA[0],          trcA[1], trcA[2], trcA[3]);
            setTrc(gamma, trcA[0], doubleItem.get(),  trcA[2], trcA[3]);
        }

        if (nodeName == "trcA2") {
            double trcA[4], gamma;

            if (!doubleItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::trcA2\n");
                return false;
            }

            getTrc(gamma, trcA[0], trcA[1],          trcA[2], trcA[3]);
            setTrc(gamma, trcA[0], trcA[1], doubleItem.get(), trcA[3]);
        }

        if (nodeName == "trcA3") {
            double trcA[4], gamma;

            if (!doubleItem.unserialize(doc, node)) {
                fprintf(stderr, 
                    "Error parsing DreamColorSpaceInfo::trcA3\n");
                return false;
            }

            getTrc(gamma, trcA[0], trcA[1], trcA[2], trcA[3]);
            setTrc(gamma, trcA[0], trcA[1], trcA[2], doubleItem.get());
        }

        node = node->next;
    }

    return true;
}



