// --------------------------------------------------------------------------
// $Id: Dict.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DICT_H_HAS_BEEN_INCLUDED
#define DICT_H_HAS_BEEN_INCLUDED

#include <vector>
#include <string>
#include <map>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "Types.h"
#include "Plugin.h"

namespace Ookala {

class EXIMPORT DictItem
{
    public:
        DictItem();

        // Derived classes should also take care to have a copy 
        // constructor which hits the base-class copy constructor
        // thanks to windows + stl.
        DictItem(const DictItem &src);
        virtual ~DictItem();
        DictItem & operator=(const DictItem &src);
        
        virtual const std::string itemType() const;
        
        virtual bool serializable() { return mIsSerializable; }

        // root points to the place whose child should be a 
        // <dictitem></dictitem> tag.
        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);

        // root should be pointing to a <dictitem></dictitem> element.
        // This may return false if we don't find a name.
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

        // NOTE: Derived classes should implement get and set methods, 
        //        to access their particulars. Unfortunaly, we can't 
        //        force them to do so with pure-virtuals, because we
        //        don't have consistant signatures of the methods.
        
        virtual void debug();

    protected:        
        bool        mIsSerializable;

        void        setItemType(const std::string &type);        

        // root should be pointing to a <dictitem></dictitem> element.
        // (but this isn't enforced)
        //
        // tags will contain the XXX of all <XXX>YYY</XXX> that follow,
        // but will only descend one level.
        //
        // contents will contain the YYY of all <XXX>YYY</XXX> that
        // follow, also only descending one level.
        //
        // Both vectors should come back the same length.
        virtual bool getXmlDirectChildren(
                                   xmlDocPtr doc, xmlNodePtr root,
                                   std::vector<std::string> &tags,
                                   std::vector<std::string> &contents);

    private:
        struct _DictItem {
            std::string mItemType;
        };

        _DictItem *mDictItemData;
};


// 
// Boolean dictionary item
//
class EXIMPORT BoolDictItem: public DictItem 
{
    public:
        BoolDictItem();
        BoolDictItem(const BoolDictItem &src);
        BoolDictItem & operator=(const BoolDictItem &src);

        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

        void set(const bool value);
        bool get();

        virtual void debug();

    protected:
        bool mValue;
};

// 
// Integer dictionary item
//
class EXIMPORT IntDictItem: public DictItem
{
    public:
        IntDictItem();
        IntDictItem(const IntDictItem &src);
        IntDictItem & operator=(const IntDictItem &src);

        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

        void set(const int32_t value);
        int32_t get();

        virtual void debug();

    protected:
        int32_t mValue;
};

// 
// Double dictionary item
//
class EXIMPORT DoubleDictItem: public DictItem
{
    public:
        DoubleDictItem();
        DoubleDictItem(const DoubleDictItem &src);
        DoubleDictItem & operator=(const DoubleDictItem &src);

        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

        void set(const double value);
        double get();

        virtual void debug();

    protected:
        double mValue;
};

// 
// String dictionary item
//
class EXIMPORT StringDictItem: public DictItem
{
    public:
        StringDictItem();
        StringDictItem(const StringDictItem &src);
        virtual ~StringDictItem();
        StringDictItem & operator=(const StringDictItem &src);

        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

        void set(const std::string value);
        std::string get();

        virtual void debug();

    private:
        struct _StringDictItem {
            std::string mValue;
        };
        
        _StringDictItem *mStringDictItemData;
};

// 
// Memory blob dictionary item
//
class EXIMPORT BlobDictItem: public DictItem
{
    public:
        BlobDictItem();
        BlobDictItem(const BlobDictItem &src);
        BlobDictItem & operator=(const BlobDictItem &src);

        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);

        void set(uint8_t *value, uint32_t size);
        uint8_t *get(uint32_t &size);

        virtual void debug();

    protected:
        uint8_t *mValue;
        uint32_t mSize;
};


// 
// Integer array dictionary item
//
class EXIMPORT IntArrayDictItem: public DictItem
{
    public:
        IntArrayDictItem();
        IntArrayDictItem(const IntArrayDictItem &src);
        virtual ~IntArrayDictItem(); 
        IntArrayDictItem & operator=(const IntArrayDictItem &src);
        
        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root); 
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);       

        void set(const std::vector<int32_t> value);
        
        std::vector<int32_t> get();

        virtual void debug();       

    private:
        struct _IntArrayDictItem {
            std::vector<int32_t> mValue;
        };

        _IntArrayDictItem *mIntArrayDictItemData;
};

// 
// Double array dictionary item
//
class EXIMPORT DoubleArrayDictItem: public DictItem
{
    public:
        DoubleArrayDictItem();
        DoubleArrayDictItem(const DoubleArrayDictItem &src);
        virtual ~DoubleArrayDictItem();
        DoubleArrayDictItem & operator=(const DoubleArrayDictItem &src);
    
        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root);
        
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);
        
        void set(const std::vector<double> value);
        
        std::vector<double> get();
        
        virtual void debug();

    private:
        struct _DoubleArrayDictItem {
            std::vector<double> mValue;
        };

        _DoubleArrayDictItem *mDoubleArrayDictItemData;
};


// 
// String array dictionary item
//
class EXIMPORT StringArrayDictItem: public DictItem
{
    public:
        StringArrayDictItem();  
        StringArrayDictItem(const StringArrayDictItem &src);    
        virtual ~StringArrayDictItem();
        StringArrayDictItem & operator=(const StringArrayDictItem &src);
        
        virtual bool serialize(xmlDocPtr doc, xmlNodePtr root); 
        virtual bool unserialize(xmlDocPtr doc, xmlNodePtr root);
        
        void set(const std::vector<std::string> value);
        
        std::vector<std::string> get();
        
        virtual void debug();
        
    private:
        struct _StringArrayDictItem {
            std::vector<std::string> mValue;
        };

        _StringArrayDictItem  *mStringArrayDictItemData;
};


//
// Given a type, figure out what sort of dictionary item to alloc
//
// We need this fellow so we can deserialize DictItems that are 
// user defined in plugins.
//
class PluginRegistry;
class EXIMPORT Dict
{
    public:
        Dict();
        Dict(PluginRegistry *registry);
        Dict(const Dict &src);
        Dict & operator=(const Dict &src);

        virtual ~Dict();

        void        setName(const std::string &name);
        
        std::string getName();
        
        void        setRegistry(PluginRegistry *registry);
        
        // Store a value with a key. The data stored should be
        // allocated dynamically, so it can stay around. Note, 
        // you'll have to free it yourself, sorry.
        bool        set(const std::string &key, DictItem *value);
        

        // Return the value stored with the given key. Returns
        // false if the key isn't found.
        bool        get(const std::string &key, DictItem **value);
        
        // Similar, but returns NULL if we don't find anything.
        DictItem *  get(const std::string &key);
        
        // Remove a certain key from the dictionary. Returns
        // false if we didn't find the key.
        bool        remove(const std::string &key);
        
        // Remove everything from the dictionary.
        bool        clear();
        
        // Returns all the keys that we're currently holding
        std::vector<std::string> getKeys();
        
        void        debug();
        
        // Add a <dict>...</dict> element into the xml stream at the
        // perscribed point, as a child of the node that is passed in.
        // 
        // If you want to use this to save out data, you first want to
        // do something like:
        //  
        //      xmlDocPtr doc = xmlNewDoc((const xmlChar *)("1.0"));
        //      if (doc == NULL) {
        //          printf("Error creating xml doc\n");
        //          ...
        //      }
        //
        //      xmlNodePtr cur = xmlNewNode(NULL,(const xmlChar *)("root"));
        //      xmlDocSetRootElement(doc, cur);
        //
        //      ... Add some stuff to the Dict ...
        //
        //      dict->serialize(doc, cur);
        //
        //      xmlSaveFormatFile("new.xml", doc, 1);
        //
        //      xmlFreeDoc(doc);
        //
        // The parent parameter should be where we insert our children.
        bool serialize(xmlDocPtr doc, xmlNodePtr parent);
        
        //  xmlDocPtr doc = xmlParseFile(filename.c_str());
        //  if (doc == NULL) {
        //     printf("Can't parse file\n");
        //     return false;
        //  }
        //
        //  xmlNodePtr root = xmlDocGetRootElement(doc);
        //  if (root == NULL) {
        //     printf("no root!\n");
        //     return false;
        //  }
        //
        //  xmlNodePtr child = root->xmlChildrenNode;
        //  while (child != NULL) {
        //
        //       /* If we like child, read in it's goodies. */
        //       if (...) {
        //          dict.unserialize(doc, child);
        //       }
        //       child = child->next;
        //  }
        //
        // When unserializing, Assume that root is pointing to a <dict>
        // node and unserialize that.
        bool unserialize(xmlDocPtr doc, xmlNodePtr root);
        

    protected:
        PluginRegistry  *mRegistry;

    private:
        struct _Dict {
            std::map<std::string, DictItem *>  mItems;   

            std::string                        mName;            
        };

        _Dict           *mDictData;
};

}; // namespace Ookala


#endif

