// --------------------------------------------------------------------------
// $Id: WsLut.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef WSLUT_H_HAS_BEEN_INCLUDED
#define WSLUT_H_HAS_BEEN_INCLUDED


#ifdef EXIMPORT
    #undef EXIMPORT
#endif

#ifdef _WIN32
#ifdef WSLUT_EXPORTS
#define EXIMPORT _declspec(dllexport)
#else
#define EXIMPORT _declspec(dllimport)
#endif

#else
#define EXIMPORT
#endif


#include <vector>

#include "Plugin.h"

namespace Ookala {

//
// Simple interface for handing window-system lookup tables. These 
// are host side, not monitor side.
//
// Currently, there isn't a great way to handle a case where we want
// multiple luts spanning a multi-head display. 
//

class EXIMPORT WsLut: public Plugin
{
    public:
        WsLut();
        WsLut(const WsLut &src);
        virtual ~WsLut();
        WsLut & operator=(const WsLut &src);

    
        PLUGIN_ALLOC_FUNCS(WsLut)

        // Return keys to the Luts that we can access
        virtual std::vector<uint32_t> luts();

        // Query the size of the Lut that the windowing system supports
        virtual uint32_t getSize(const uint32_t lutId = 0);
    
        // Returns the max output value for the Lut.
        virtual uint32_t getMax(const uint32_t lutId = 0);

        // Set the LUT with either raw or normalized values.
        virtual bool set(const std::vector<uint32_t> &redLut,
                         const std::vector<uint32_t> &greenLut,
                         const std::vector<uint32_t> &blueLut,
                         const uint32_t               lutId = 0);

        virtual bool set(const std::vector<double> &redLut,
                         const std::vector<double> &greenLut,
                         const std::vector<double> &blueLut,
                         const uint32_t             lutId = 0);


        // Retrieve the current LUT as either raw or normalized values.
        virtual bool get(std::vector<uint32_t> &redLut,
                         std::vector<uint32_t> &greenLut,
                         std::vector<uint32_t> &blueLut,
                         const uint32_t         lutId = 0);

        virtual bool get(std::vector<double> &redLut,
                         std::vector<double> &greenLut,
                         std::vector<double> &blueLut,
                         const uint32_t       lutId = 0);


        // Reset to an identity Lut - uploads lut and make it current
        virtual bool reset(const uint32_t lutId = 0);

        // Reset to randomized values - uploads lut and make it current
        virtual bool randomize(const uint32_t lutId = 0);
        
    protected:

        // The size supported. This should be queried and set
        // in   _wsGetLut().
        uint32_t              mLutSize;

        // This is the max value of the Lut output. This should
        // also be queried and set in _wsGetLut().
        uint32_t              mLutMax;

        // Window-system specific goodness for setting or loading luts
        virtual bool _wsSetLut(const std::vector<uint32_t> &redLut,
                               const std::vector<uint32_t> &greenLut,
                               const std::vector<uint32_t> &blueLut);

        virtual bool _wsGetLut(std::vector<uint32_t> &redLut,
                               std::vector<uint32_t> &greenLut,
                               std::vector<uint32_t> &blueLut);

        virtual bool _postLoad();

        // Look in the chain dict for:
        //     WsLut::randomize <bool>
        //     WsLut::reset     <bool>
        //     WsLut::pow       <double>
        virtual bool _run(PluginChain *chain);

};

}; // namespace Ookala 

#endif

