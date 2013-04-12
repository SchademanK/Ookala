// --------------------------------------------------------------------------
// $Id: Mutex.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef MUTEX_H_HAS_BEEN_INCLUDED
#define MUTEX_H_HAS_BEEN_INCLUDED

//
// A little wrapper for dealing with cross-platform mutexes.
//


#ifdef __linux__
#include <pthread.h>
#endif

#include "Types.h"
#include "Plugin.h"

namespace Ookala {

class EXIMPORT Mutex
{
    public:
        Mutex();
        ~Mutex();

        void lock();
        void unlock();

    protected:
#ifdef WIN32
        HANDLE          mMutexWin;
#else
        pthread_mutex_t mMutexPthread;        
#endif
};

}; // namespace Ookala

#endif

