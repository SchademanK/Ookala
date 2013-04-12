// --------------------------------------------------------------------------
// $Id: Dict.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <string>
#include <map>
#include <iostream>
#include <sstream>

#include <assert.h>

#include <algorithm>

#include "Dict.h"
#include "PluginRegistry.h"


// =======================================
//
// DictItem
//
// ----------------------------------------


Ookala::DictItem::DictItem()
{
    mIsSerializable          = true;

    mDictItemData            = new _DictItem;        
    mDictItemData->mItemType = "none";

}

// ----------------------------------------
//
Ookala::DictItem::DictItem(const DictItem &src)
{
    mDictItemData = new _DictItem;

    mDictItemData->mItemType = src.mDictItemData->mItemType;
    mIsSerializable          = src.mIsSerializable;
}

// ----------------------------------------
//
// virtual
Ookala::DictItem::~DictItem()
{
    if (mDictItemData) {
        delete mDictItemData;
        mDictItemData = NULL;
    }
}

// ----------------------------
//
Ookala::DictItem &
Ookala::DictItem::operator=(const DictItem &src)
{
    if (this != &src) {
        //DictItem::operator=(src);

        if (mDictItemData) {
            delete mDictItemData;
            mDictItemData = NULL;
        }

        if (src.mDictItemData) {
            mDictItemData  = new _DictItem();
            *mDictItemData = *(src.mDictItemData);
        }

        mIsSerializable = src.mIsSerializable;
    }

    return *this;
}


// ----------------------------------------
//
// virtual
const std::string 
Ookala::DictItem::itemType() const 
{ 
    if (mDictItemData) {
        return mDictItemData->mItemType;
    }
    return std::string("");    
}

// ----------------------------------------
//
// virtual
bool 
Ookala::DictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    return false;
}


// ----------------------------------------
//
// root should point to a <dictitem></dictitem> element.
//
// virtual
bool 
Ookala::DictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    return false;
}

// ----------------------------------------
//
// virtual
void
Ookala::DictItem::debug()
{
}

// ----------------------------------------
//
// protected
void        
Ookala::DictItem::setItemType(const std::string &type)
{
    if (mDictItemData) {
        mDictItemData->mItemType = type;
    }
}

// ----------------------------------------
//
// root should be pointing to a <dictitem></dictitem> element.
//
// tags will contain the XXX of all <XXX>YYY</XXX> that follow,
// but will only descend one level.
//
// contents will contain the YYY of all <XXX>YYY</XXX> that
// follow, also only descending one level.
//
// Both vectors should come back the same length.
//
// virtual protected
bool
Ookala::DictItem::getXmlDirectChildren(
                           xmlDocPtr doc, xmlNodePtr root,
                           std::vector<std::string> &tags,
                           std::vector<std::string> &contents)
{
    xmlNodePtr node;
    std::string tagStr, contentStr;

    tags.clear();
    contents.clear();

    node = root->xmlChildrenNode;
    while (node != NULL) {

        tagStr     = "";
        contentStr = "";

        if (node->name != NULL) {
            tagStr = (char *)node->name;
        }

        xmlChar *value = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        if (value != NULL) {
            contentStr = (char *)value;

            xmlFree(value);
        }
        
        tags.push_back(tagStr);
        contents.push_back(contentStr);

        node = node->next;
    }

    if (tags.size() != contents.size()) {
        fprintf(stderr, 
            "WARNING: mismatch in DictItem::getXmlDirectChildren\n");
    }
    return true;
}


// ========================================
//
// BoolDictItem
//
// ----------------------------------------

Ookala::BoolDictItem::BoolDictItem() :  
    DictItem()
{
    setItemType("bool");
    mValue    = false;
}

// ----------------------------------------
//
Ookala::BoolDictItem::BoolDictItem(const BoolDictItem &src):
    DictItem(src)
{
    mValue = src.mValue;
}

// ----------------------------
//
Ookala::BoolDictItem &
Ookala::BoolDictItem::operator=(const BoolDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        mValue = src.mValue;
    }

    return *this;
}


// ----------------------------------------
//
// root points to the place whose child should be a 
// <dictitem></dictitem> tag.
//
// virtual
bool 
Ookala::BoolDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{

    if (mValue) {
        xmlNodeAddContent(root, (const xmlChar *)"true");
    } else {
        xmlNodeAddContent(root, (const xmlChar *)"false");
    }

    return true;
}

// ----------------------------------------
//
// root should point to a <dictitem></dictitem> element.
//
// Example:
//      <dictitem type="bool" name="show parameter gui">false</dictitem>
//
// virtual
bool 
Ookala::BoolDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{

    mValue = false;

    std::string contentStr;

    // So, if we have <dictitem type=bool>true</dictitem>, we would
    // want to do:
    //
    //      xmlNodeListGetString(doc, root->xmlChildrenNode, 1);
    //
    // to retrieve the text.
    
    xmlChar *value = xmlNodeListGetString(doc, root->xmlChildrenNode, 1);
    if (value != NULL) {
        contentStr = (char *)value;
        xmlFree(value);
    } 

    // Trim any leading + trailing space, and take 
    // to lower case.
    size_t start = contentStr.find_first_not_of(" \t\n");
    size_t end   = contentStr.find_last_not_of(" \t\n");

    if (start == std::string::npos) {
        contentStr = "";
    } else {
        contentStr = contentStr.substr(start, end-start+1);
    }
    std::transform(contentStr.begin(), 
                   contentStr.end(), 
                   contentStr.begin(), 
#ifdef _WIN32
                   tolower);
#else
                   (int(*)(int))std::tolower);                  
#endif

    if (contentStr == "true") {
        mValue = true;
    } else {
        mValue = false;
    }

    return true;
}

// ----------------------------------------

void
Ookala::BoolDictItem::set(const bool value)
{
    mValue = value;
}

// ----------------------------------------

bool
Ookala::BoolDictItem::get()
{
    return mValue;
}

// ----------------------------------------
//
// virtual
void
Ookala::BoolDictItem::debug()
{
    if (mValue) {
        fprintf(stderr, "true\n");
    } else {
        fprintf(stderr, "false\n");
    }
}



// ========================================
//
// IntDictItem
//
// ----------------------------------------


Ookala::IntDictItem::IntDictItem() :  
    DictItem()
{
    setItemType("int");
    mValue    = 0;
}


// ----------------------------------------
//
Ookala::IntDictItem::IntDictItem(const IntDictItem &src):
    DictItem(src)
{
    mValue = src.mValue;
}

// ----------------------------------------
//
// root points to the place whose child should be a 
// <dictitem></dictitem> tag.

// virtual
bool 
Ookala::IntDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    std::ostringstream value;

    value << mValue;

    xmlNodeAddContent(root, (const xmlChar *)(value.str().c_str()));

    return true;
}

// ----------------------------
//
Ookala::IntDictItem &
Ookala::IntDictItem::operator=(const IntDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        mValue = src.mValue;
    }

    return *this;
}

// ----------------------------------------
//
// root should point to a <dictitem></dictitem> element.
//
// Example:
//      <dictitem type="int" name="Meaning of life"> 42 </dictitem>
//
// virtual
bool 
Ookala::IntDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    mValue   = 0;

    xmlChar *value = xmlNodeListGetString(doc, root->xmlChildrenNode, 1);
    if (value != NULL) {
        mValue = atoi((char *)value);
        xmlFree(value);
    } 

    return true;
}

// ----------------------------------------

void
Ookala::IntDictItem::set(const int32_t value)
{
    mValue = value;
}

// ----------------------------------------

int32_t
Ookala::IntDictItem::get()
{
    return mValue;
}

// ----------------------------------------
//
// virtual
void
Ookala::IntDictItem::debug()
{
    printf("%d\n", mValue);
}


// ========================================
//
// DoubleDictItem
//
// ----------------------------------------

Ookala::DoubleDictItem::DoubleDictItem() :  
    DictItem()
{
    setItemType("double");
    mValue    = 0.0;
}

// ----------------------------------------
//

Ookala::DoubleDictItem::DoubleDictItem(const DoubleDictItem &src) :  
    DictItem(src)
{
    mValue = src.mValue;
}

// ----------------------------
//
Ookala::DoubleDictItem &
Ookala::DoubleDictItem::operator=(const DoubleDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        mValue = src.mValue;
    }

    return *this;
}


// ----------------------------------------
//
// root points to the place whose child should be a 
// <dictitem></dictitem> tag.
//
// virtual
bool 
Ookala::DoubleDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    std::ostringstream value;

    value << mValue;

    xmlNodeAddContent(root, (const xmlChar *)(value.str().c_str()));
    return true;
}


// ----------------------------------------
//
// root should point to a <dictitem></dictitem> element.
// If we don't find a name, return false;
//
// Example:
//      <dictitem type="double" name="Meaning of life">42.0001</dictitem>
//
// virtual
bool
Ookala::DoubleDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    mValue   = 0;

    xmlChar *value = xmlNodeListGetString(doc, root->xmlChildrenNode, 1);
    if (value != NULL) {
        mValue = atof((char *)value);
        xmlFree(value);
    } 

    return true;
}

// ----------------------------------------

void
Ookala::DoubleDictItem::set(const double value)
{
    mValue = value;
}

// ----------------------------------------

double
Ookala::DoubleDictItem::get()
{
    return mValue;
}

// ----------------------------------------
//
// virtual
void
Ookala::DoubleDictItem::debug()
{
    printf("%f\n", mValue);
}


// ========================================
//
// StringDictItem
//
// ----------------------------------------


Ookala::StringDictItem::StringDictItem() :  
    DictItem()
{
    setItemType("string");

    mStringDictItemData = new _StringDictItem;    
}

// ----------------------------------------
//
Ookala::StringDictItem::StringDictItem(const StringDictItem &src):
    DictItem(src)
{
    mStringDictItemData = new _StringDictItem;

    mStringDictItemData->mValue = src.mStringDictItemData->mValue;
}

// ----------------------------------------
//
// virtual
Ookala::StringDictItem::~StringDictItem()
{
    if (mStringDictItemData) {
        delete mStringDictItemData;
        mStringDictItemData = NULL;
    }
}      


// ----------------------------
//
Ookala::StringDictItem &
Ookala::StringDictItem::operator=(const StringDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        if (mStringDictItemData) {
            delete mStringDictItemData;
            mStringDictItemData = NULL;
        }

        if (src.mStringDictItemData) {
            mStringDictItemData  = new _StringDictItem();
            *mStringDictItemData = *(src.mStringDictItemData);
        }
    }

    return *this;
}



// ----------------------------------------
//
// root points to the place whose child should be a 
// <dictitem></dictitem> tag.
//
// virtual
bool 
Ookala::StringDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{

    xmlNodeAddContent(root, (const xmlChar *)(get().c_str()));

    return true;
}


// ----------------------------------------
//
// root should point to a <dictitem></dictitem> element.
// If we don't find a name, return false;
//
// Example:
//     <dictitem type="string" name="meaning of life">42.0001</dictitem>
//
// virtual
bool
Ookala::StringDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    std::string str;

    set("");

    xmlChar *value = xmlNodeListGetString(doc, root->xmlChildrenNode, 1);
    if (value != NULL) {
        str = ((char *)value);
        xmlFree(value);
    } 

    // Trim any leading + trailing space
    size_t start = str.find_first_not_of(" \t\n");
    size_t end   = str.find_last_not_of(" \t\n");

    if (start == std::string::npos) {
        str = "";
    } else {
        str = str.substr(start, end-start+1);
    }

    set(str);

    return true;
}

// ----------------------------------------

void
Ookala::StringDictItem::set(const std::string value)
{
    if (mStringDictItemData) {
        mStringDictItemData->mValue = value;
    }
}

// ----------------------------------------

std::string
Ookala::StringDictItem::get()
{
    if (mStringDictItemData) {
        return mStringDictItemData->mValue;
    }
    return std::string("");;
}

// ----------------------------------------
//
// virtual
void
Ookala::StringDictItem::debug()
{
    printf("%s\n", get().c_str());
}


// ========================================
//
// BlobDictItem
//
// ----------------------------------------


Ookala::BlobDictItem::BlobDictItem() :  
    DictItem()
{
    setItemType("blob");
    mValue          = NULL;
    mSize           = 0;
    mIsSerializable = false;

}

// ----------------------------------------
//
Ookala::BlobDictItem::BlobDictItem(const BlobDictItem &src):
    DictItem(src)
{
    mValue = src.mValue;
    mSize  = src.mSize;
}

// ----------------------------------------
//
// Blob's aren't serializable right now.
//
// virtual
bool 
Ookala::BlobDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    return true;
}

// ----------------------------
//
Ookala::BlobDictItem &
Ookala::BlobDictItem::operator=(const BlobDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        mValue = src.mValue;
        mSize  = src.mSize;
    }

    return *this;
}



// ----------------------------------------
//
// Blob's aren't deserializable right now.
//
// virtual
bool
Ookala::BlobDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    return true;
}

// ----------------------------------------

void
Ookala::BlobDictItem::set(uint8_t *value, uint32_t size)
{
    mValue = value;
    mSize  = size;
}

// ----------------------------------------

uint8_t *
Ookala::BlobDictItem::get(uint32_t &size)
{
    size = mSize;
    return mValue;
}

// ----------------------------------------
//
// virtual
void
Ookala::BlobDictItem::debug()
{
    printf("%d bytes\n",  mSize);
}


// ========================================
//
// IntArrayDictItem
//
// ----------------------------------------

Ookala::IntArrayDictItem::IntArrayDictItem() :  
    DictItem()
{
    setItemType("intArray");

    mIntArrayDictItemData = new _IntArrayDictItem;
}

// ----------------------------------------
//
Ookala::IntArrayDictItem::IntArrayDictItem(const IntArrayDictItem &src):
    DictItem(src)
{
    mIntArrayDictItemData = new _IntArrayDictItem;

    if (src.mIntArrayDictItemData) {
        mIntArrayDictItemData->mValue = src.mIntArrayDictItemData->mValue;
    }
}
      
// ----------------------------------------
//
// virtual
Ookala::IntArrayDictItem::~IntArrayDictItem()
{
    if (mIntArrayDictItemData) {
        delete mIntArrayDictItemData;
        mIntArrayDictItemData = NULL;
    }
}

// ----------------------------
//
Ookala::IntArrayDictItem &
Ookala::IntArrayDictItem::operator=(const IntArrayDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        if (mIntArrayDictItemData) {
            delete mIntArrayDictItemData;
            mIntArrayDictItemData = NULL;
        }

        if (src.mIntArrayDictItemData) {
            mIntArrayDictItemData  = new _IntArrayDictItem();
            *mIntArrayDictItemData = *(src.mIntArrayDictItemData);
        }
    }

    return *this;
}


// ----------------------------------------
//
// root points to the place whose child should be a 
// <dictitem></dictitem> tag.
//
// virtual
bool 
Ookala::IntArrayDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    for (std::vector<int32_t>::iterator i=mIntArrayDictItemData->mValue.begin();
                           i != mIntArrayDictItemData->mValue.end(); ++i) {

        std::ostringstream value;
        value << (*i);

        xmlNewTextChild(root, NULL, (const xmlChar *)"value", 
                                    (const xmlChar *)value.str().c_str());
    }

    return true;
}

// ----------------------------------------
//
// root should point to a <dictitem></dictitem> element.
// If we don't find a name, return false;
//
// Example:
//            <dictitem type="intArray" name="Countdown">
//                <value>4</value>
//                <value>3</value>
//                <value>2</value>
//                <value>1</value>
//            </dictitem>

// virtual
bool
Ookala::IntArrayDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    std::vector<std::string> names, values;

    mIntArrayDictItemData->mValue.clear();

    getXmlDirectChildren(doc, root, names, values);

    std::vector<std::string>::iterator theName  = names.begin();
    std::vector<std::string>::iterator theValue = values.begin();

    while ((theName != names.end()) && (theValue != values.end()))  {

        if ((*theName) == "value") {
            mIntArrayDictItemData->mValue.push_back(atoi((*theValue).c_str()));
        }

        ++theName;
        ++theValue;
    }

    return true;
}

// ----------------------------------------

void
Ookala::IntArrayDictItem::set(const std::vector<int32_t> value)
{
    mIntArrayDictItemData->mValue = value;
}

// ----------------------------------------


std::vector<int32_t>
Ookala::IntArrayDictItem::get()
{
    return mIntArrayDictItemData->mValue;
}

// ----------------------------------------
//
// virtual
void
Ookala::IntArrayDictItem::debug()
{
    int idx = 0;

    for (std::vector<int32_t>::iterator i = mIntArrayDictItemData->mValue.begin();
                i != mIntArrayDictItemData->mValue.end(); ++i) {
        printf("%d: %d\n", idx, (*i));
        idx++;
    }
}





// ========================================
//
// DoubleArrayDictItem
//
// ----------------------------------------

Ookala::DoubleArrayDictItem::DoubleArrayDictItem():  
    DictItem()
{
    setItemType("doubleArray");

    mDoubleArrayDictItemData = new _DoubleArrayDictItem;
}

// ----------------------------------------
//
Ookala::DoubleArrayDictItem::DoubleArrayDictItem(const DoubleArrayDictItem &src):  
    DictItem(src)
{
    mDoubleArrayDictItemData = new _DoubleArrayDictItem;    

    if (src.mDoubleArrayDictItemData) {
        mDoubleArrayDictItemData->mValue = src.mDoubleArrayDictItemData->mValue;
    }
}

// ----------------------------------------
//
// virtual
Ookala::DoubleArrayDictItem::~DoubleArrayDictItem()
{
    if (mDoubleArrayDictItemData) {
        delete mDoubleArrayDictItemData;
        mDoubleArrayDictItemData = NULL;
    }
}

// ----------------------------
//
Ookala::DoubleArrayDictItem &
Ookala::DoubleArrayDictItem::operator=(const DoubleArrayDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        if (mDoubleArrayDictItemData) {
            delete mDoubleArrayDictItemData;
            mDoubleArrayDictItemData = NULL;
        }

        if (src.mDoubleArrayDictItemData) {
            mDoubleArrayDictItemData  = new _DoubleArrayDictItem();
            *mDoubleArrayDictItemData = *(src.mDoubleArrayDictItemData);
        }
    }

    return *this;
}



// ----------------------------------------
//
// root points to the place whose child should be a 
// <dictitem></dictitem> tag.
//
// virtual
bool 
Ookala::DoubleArrayDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    for (std::vector<double>::iterator i=mDoubleArrayDictItemData->mValue.begin();
                           i != mDoubleArrayDictItemData->mValue.end(); ++i) {

        std::ostringstream value;
        value << (*i);

        xmlNewTextChild(root, NULL, (const xmlChar *)"value", 
                                    (const xmlChar *)value.str().c_str());
    }

    return true;
}

// ----------------------------------------
//
// root should point to a <dictitem></dictitem> element.
// If we don't find a name, return false;
//
// Example:
//            <dictitem type="doubleArray" name="Countdown">
//                <name>Countdown</name>
//                <value>4</value>
//                <value>3</value>
//                <value>2</value>
//                <value>1</value>
//            </dictitem>
//
// virtual
bool
Ookala::DoubleArrayDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    std::vector<std::string> names, values;

    mDoubleArrayDictItemData->mValue.clear();

    getXmlDirectChildren(doc, root, names, values);

    std::vector<std::string>::iterator theName  = names.begin();
    std::vector<std::string>::iterator theValue = values.begin();

    while ((theName != names.end()) && (theValue != values.end()))  {

        if ((*theName) == "value") {
            mDoubleArrayDictItemData->mValue.push_back(atof((*theValue).c_str()));
        }

        ++theName;
        ++theValue;
    }

    return true;
}

// ----------------------------------------

void
Ookala::DoubleArrayDictItem::set(const std::vector<double> value)
{
    mDoubleArrayDictItemData->mValue = value;
}

// ----------------------------------------

std::vector<double>
Ookala::DoubleArrayDictItem::get()
{
    return mDoubleArrayDictItemData->mValue;
}

// ----------------------------------------
//
// virtual
void
Ookala::DoubleArrayDictItem::debug()
{
    int idx = 0;

    for (std::vector<double>::iterator i = 
              mDoubleArrayDictItemData->mValue.begin();
                i != mDoubleArrayDictItemData->mValue.end(); ++i) {
        printf("%d: %f\n", idx, (*i));
        idx++;
    }   
}



// ========================================
//
// StringArrayDictItem
//
// ----------------------------------------


Ookala::StringArrayDictItem::StringArrayDictItem() :  
    DictItem()
{
    setItemType("stringArray");

    mStringArrayDictItemData = new _StringArrayDictItem;
}

// ----------------------------------------
//
Ookala::StringArrayDictItem::StringArrayDictItem(
                        const StringArrayDictItem &src):
    DictItem(src)
{
    mStringArrayDictItemData = new _StringArrayDictItem;

    if (src.mStringArrayDictItemData) {
        mStringArrayDictItemData->mValue = src.mStringArrayDictItemData->mValue;
    }
}

// ----------------------------------------
//
// virtual
Ookala::StringArrayDictItem::~StringArrayDictItem()
{
    if (mStringArrayDictItemData) {    
        delete mStringArrayDictItemData;
        mStringArrayDictItemData = NULL;
    }
}

// ----------------------------
//
Ookala::StringArrayDictItem &
Ookala::StringArrayDictItem::operator=(
                            const StringArrayDictItem &src)
{
    if (this != &src) {
        DictItem::operator=(src);

        if (mStringArrayDictItemData) {
            delete mStringArrayDictItemData;
            mStringArrayDictItemData = NULL;
        }

        if (src.mStringArrayDictItemData) {
            mStringArrayDictItemData  = new _StringArrayDictItem();
            *mStringArrayDictItemData = *(src.mStringArrayDictItemData);
        }
    }

    return *this;
}


// ----------------------------------------
//
// root points to the place whose child should be a 
// <dictitem></dictitem> tag.
//
// virtual
bool 
Ookala::StringArrayDictItem::serialize(xmlDocPtr doc, xmlNodePtr root)
{
    for (std::vector<std::string>::iterator i =
                      mStringArrayDictItemData->mValue.begin();
                 i != mStringArrayDictItemData->mValue.end(); ++i) {

        xmlNewTextChild(root, NULL, (const xmlChar *)"value", 
                                    (const xmlChar *)(*i).c_str());
    }

    return true;
}

// ----------------------------------------
//
// root should point to a <dictitem></dictitem> element.
// If we don't find a name, return false;
//
//    Example:
//        <dictitem type="stringArray" name="DataSavior Files">
//            <value>/usr/home/ookala/loadme.xml</value>
//            <value>~/loadme2.xml</value>
//            <value>~/loadme.%s.xml</value>
//            <value>/asdfa/vafas/foo.xml</value>
//        </dictitem>
//
// virtual
bool
Ookala::StringArrayDictItem::unserialize(xmlDocPtr doc, xmlNodePtr root) 
{
    std::vector<std::string> names, values;

    mStringArrayDictItemData->mValue.clear();

    getXmlDirectChildren(doc, root, names, values);

    std::vector<std::string>::iterator theName  = names.begin();
    std::vector<std::string>::iterator theValue = values.begin();

    while ((theName != names.end()) && (theValue != values.end()))  {

        if ((*theName) == "value") {

            // Trim any leading + trailing space
            size_t start = (*theValue).find_first_not_of(" \t\n");
            size_t end   = (*theValue).find_last_not_of(" \t\n");

            if (start == std::string::npos) {
                mStringArrayDictItemData->mValue.push_back("");
            } else {
                mStringArrayDictItemData->mValue.push_back(
                                (*theValue).substr(start, end-start+1));
            }
        }

        ++theName;
        ++theValue;
    }

    return true;
}

// ----------------------------------------

void
Ookala::StringArrayDictItem::set(const std::vector<std::string> value)
{
    mStringArrayDictItemData->mValue = value;
}

// ----------------------------------------

std::vector<std::string>
Ookala::StringArrayDictItem::get()
{
    return mStringArrayDictItemData->mValue;
}

// ----------------------------------------

// virtual
void
Ookala::StringArrayDictItem::debug()
{
    int idx = 0;

    for (std::vector<std::string>::iterator i = 
                     mStringArrayDictItemData->mValue.begin();
                i != mStringArrayDictItemData->mValue.end(); ++i) {
        printf("%d: %s\n", idx, (*i).c_str());
        idx++;
    }   
}



// ========================================
//
// _Dict
//
// ----------------------------------------

Ookala::Dict::Dict()
{
    mRegistry = NULL;    

    mDictData = new _Dict;
}

// ----------------------------------------
//

Ookala::Dict::Dict(PluginRegistry *registry)
{
    mRegistry = registry;   

    mDictData = new _Dict;
}

// ----------------------------------------
//
Ookala::Dict::Dict(const Dict &src)
{
    mRegistry = src.mRegistry;

    mDictData = new _Dict;
    if (src.mDictData) {
        mDictData->mItems = src.mDictData->mItems;
        mDictData->mName  = src.mDictData->mName;
    }
}

// ----------------------------------------
//
//virtual 
Ookala::Dict::~Dict()
{
    if (mDictData) {
        delete mDictData;
        mDictData = NULL;
    }
}

// ----------------------------
//
Ookala::Dict &
Ookala::Dict::operator=(const Dict &src)
{
    if (this != &src) {

        if (mDictData) {
            delete mDictData;
            mDictData = NULL;
        }

        if (src.mDictData) {
            mDictData  = new _Dict();
            *mDictData = *(src.mDictData);
        }

        mRegistry = src.mRegistry;
    }

    return *this;
}


// ----------------------------------------

void
Ookala::Dict::setName(const std::string &name)
{
    if (mDictData) {
        mDictData->mName = name;
    }
}

// ----------------------------------------

std::string
Ookala::Dict::getName()
{   
    if (mDictData) {
        return mDictData->mName;
    }

    return std::string("");
}

// ----------------------------------------

void 
Ookala::Dict::setRegistry(PluginRegistry *registry)
{
    mRegistry = registry;
}

// ----------------------------------------

bool
Ookala::Dict::set(const std::string &key, DictItem *value)
{
    if (mDictData) {
        mDictData->mItems[key] = value;
        return true;
    }

    return false;
}

// ----------------------------------------

bool
Ookala::Dict::get(const std::string &key, DictItem **value)
{
    
    if (!mDictData) return false;

    std::map<std::string, DictItem *>::iterator i = mDictData->mItems.find(key);
    
    if (i != mDictData->mItems.end()) {
        *value = (*i).second;
        return true;    
    }

    return false;
}

// ----------------------------------------

Ookala::DictItem *
Ookala::Dict::get(const std::string &key)
{
    if (!mDictData) return NULL;

    std::map<std::string, DictItem *>::iterator i = mDictData->mItems.find(key);
    
    if (i != mDictData->mItems.end()) {
        return (*i).second;    
    }

    return NULL;
}

// ----------------------------------------

bool
Ookala::Dict::remove(const std::string &key)
{
    if (!mDictData) return false;

    std::map<std::string, DictItem *>::iterator i = mDictData->mItems.find(key);
   
    if (i != mDictData->mItems.end()) {
        mDictData->mItems.erase(i);
        return true;    
    }

    return false;
}

// ----------------------------------------

bool
Ookala::Dict::clear()
{
    if (!mDictData) return false;

    mDictData->mItems.clear();

    return true;
}

// ----------------------------------------
//
// Returns all the keys that we're currently holding
std::vector<std::string>
Ookala::Dict::getKeys()
{
    std::vector<std::string> keys;

    if (!mDictData) return keys;

    for (std::map<std::string, DictItem *>::iterator i = 
                 mDictData->mItems.begin();
            i != mDictData->mItems.end(); ++i) {
        keys.push_back((*i).first);
    }

    return keys;
}

// ----------------------------------------

void
Ookala::Dict::debug()
{
    if (!mDictData) return;

    printf("%s: %d items\n", mDictData->mName.c_str(),  
                        (int)mDictData->mItems.size());
    for (std::map<std::string, DictItem *>::iterator i = 
                 mDictData->mItems.begin();
            i != mDictData->mItems.end(); ++i) {
        printf("---------------------------\n%s:\n", (*i).first.c_str());
        (*i).second->debug();
    }
}

// ----------------------------------------
//
// Insert a <dict> node as a child of parent

bool 
Ookala::Dict::serialize(xmlDocPtr doc, xmlNodePtr parent)
{
    xmlNodePtr dictNode, itemNode;

    if (!mDictData) return false;

    dictNode = xmlNewTextChild(parent, NULL, 
                    (const xmlChar *)"dict", NULL);
    if (mDictData->mName != "") {
        xmlSetProp(dictNode, (const xmlChar *)"name", 
                    (const xmlChar *)mDictData->mName.c_str());
    }

    for (std::map<std::string, DictItem *>::iterator i = 
                 mDictData->mItems.begin();
            i != mDictData->mItems.end(); ++i) {

        if ( (*i).second->serializable()) {
            itemNode = xmlNewTextChild(dictNode, NULL, 
                                (const xmlChar *)"dictitem", NULL);

            xmlSetProp(itemNode, (const xmlChar *)"name",
                        (const xmlChar *)(*i).first.c_str());

            xmlSetProp(itemNode, (const xmlChar *)"type",
                        (const xmlChar *)(*i).second->itemType().c_str());

            (*i).second->serialize(doc, itemNode);
        } 
    }

    return true;
}

// ----------------------------------------
//
// Assume that root is pointing to a <dict> node.

bool
Ookala::Dict::unserialize(xmlDocPtr doc, xmlNodePtr root)
{
    xmlNodePtr  dictNode;
    xmlNodePtr  itemNode;
    DictItem   *itemPtr;
    std::string itemName, dataType;
    xmlChar    *attrName;

    if (!mDictData) return false;

    if (root == NULL) {
        return false;
    }

    if (strcmp((char *)root->name, "dict")) {
        return false;
    } else {
        dictNode = root;
    }   

    printf("Found <dict> in stream\n");

    attrName = xmlGetProp(dictNode, (const xmlChar *)("name"));
    if (attrName != NULL) {
        mDictData->mName = (char *)(attrName);
        xmlFree(attrName);
    }

    // Here we've found a <dict></dict>. Walk all of its children
    // and pull out any <dictitem></dictitem> tags.

    itemNode = dictNode->xmlChildrenNode;
    while (itemNode) {

        if (strcmp((char *)itemNode->name, "dictitem")) {
            itemNode = itemNode->next;
            continue;
        }

        // <dictitem> must contain a type property that specifies
        // what sort of data it contains. For example,
        //   <dictitem name="foo" type="int"> ...
        attrName = xmlGetProp(itemNode, (const xmlChar *)("type"));
        if (attrName == NULL) {
            itemNode = itemNode->next;
            continue;
        }

        dataType.assign((char *)attrName);

        // <dictitem> also must have a name property, otherwise,
        // we'll skip it.
        attrName = xmlGetProp(itemNode, (const xmlChar *)("name"));
        if (attrName == NULL) {
            itemNode = itemNode->next;
            continue;
        }
        
        itemName.assign((char *)attrName);
        printf("Found <dictitem> \"%s\" of type %s\n",
                    itemName.c_str(), dataType.c_str());

        // Now we have a <dictitem></dictitem> tag. Decode it and
        // store the resulting values.
        if (mRegistry == NULL) {
            itemNode = itemNode->next;
            continue;
        }

        itemPtr = mRegistry->createDictItem(dataType);
        if (itemPtr == NULL) {
            fprintf(stderr, "WARNING: No DictItem found for type %s\n",
                                                    dataType.c_str());
            itemNode = itemNode->next;
            continue;
        }
        if (itemPtr->unserialize(doc, itemNode)) {
            set(itemName, itemPtr);
        } 
        itemNode = itemNode->next;
    }

    return true;
}





