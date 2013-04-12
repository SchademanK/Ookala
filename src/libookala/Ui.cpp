// --------------------------------------------------------------------------
// $Id: Ui.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include "Ui.h"

// -------------------------------
//
Ookala::Ui::Ui():
    Plugin()
{
    setName("Ui");

    addAttribute("ui");
    addAttribute("Ui");
    addAttribute("UI");

    addAttribute("gui");
    addAttribute("Gui");
    addAttribute("GUI");
}

// -------------------------------
//
Ookala::Ui::Ui(const Ui &src):
    Plugin(src)
{

}

// ----------------------------
//
Ookala::Ui &
Ookala::Ui::operator=(const Ui &src)
{
    if (this != &src) {
        Plugin::operator=(src);
    }

    return *this;
}


// -------------------------------
//
// virtual 
bool 
Ookala::Ui::setBool(const std::string &key, bool value)
{
    return false;
}

// -------------------------------
//
// virtual 
bool 
Ookala::Ui::setString(const std::string &key, const std::string &value)
{
    return false;
} 

// -------------------------------
//
// virtual 
bool 
Ookala::Ui::setInt(const std::string &key, int32_t value)
{
    return false;
}

// -------------------------------
//
// virtual 
bool 
Ookala::Ui::setDouble(const std::string &key, double value)
{
    return false;
}

// -------------------------------
//
// virtual 
bool 
Ookala::Ui::setRgb(const std::string &key, Rgb value)
{
    return false;
}

// -------------------------------
//
// virtual 
bool 
Ookala::Ui::setYxy(const std::string &key, Yxy value)
{
    return false;
}


