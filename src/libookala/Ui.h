// --------------------------------------------------------------------------
// $Id: Ui.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef UI_H_HAS_BEEN_INCLUDED
#define UI_H_HAS_BEEN_INCLUDED

#include <string>

#include "Types.h"
#include "Plugin.h"

namespace Ookala {

// Base class for implementing Gui plugins. It's pretty
// simple, only allowing for other folks to update values
// that we might care about.
//
// Settable values are distingished by keys, some of which
// are described below:
//
//   Ui::visible             - bool
//   Ui::status_string_major - string
//   Ui::status_string_minor - string
//   Ui::measured_Yxy        - Yxy   [Measured in cd/m^2]
//   Ui::target_red_Yxy      - Yxy
//   Ui::target_green_Yxy    - Yxy
//   Ui::target_blue_Yxy     - Yxy
//   Ui::target_white_Yxy    - Yxy
//

class EXIMPORT Ui: public Plugin
{
    public:
        Ui();
        Ui(const Ui &src);
        virtual ~Ui() {}
        Ui & operator=(const Ui &src);

        virtual bool setBool(  const std::string &key, bool        value);
        virtual bool setString(const std::string &key,  
                                                const std::string &value);     
        virtual bool setInt(   const std::string &key, int32_t     value);
        virtual bool setDouble(const std::string &key, double      value);
        virtual bool setRgb(   const std::string &key, Rgb         value);
        virtual bool setYxy   (const std::string &key, Yxy         value);
};

}; // namespace Ookala

#endif


