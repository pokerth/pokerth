#ifndef _BOOST_TIMERS_WIN32_TGT_DEVICE_HPP
#define _BOOST_TIMERS_WIN32_TGT_DEVICE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

// Boost headers
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

// Automagically link winmm.lib if we can
#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#pragma comment(lib, "winmm.lib")
#endif

// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace win32
{

//! The tgt_device class is based on timeGetTime() to compute the time
class tgt_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit tgt_device() : m_start(0), m_elapsed(0, 0, 0)
    {
        #ifndef BOOST_TGT_DEFAULT_RES
        timeBeginPeriod(1);
        #endif
    }

    ~tgt_device()
    {
        #ifndef BOOST_TGT_DEFAULT_RES
        timeEndPeriod(1);
        #endif
    }

    void start() 
    {
        if(!m_start)
            m_start = timeGetTime();
    }

    const time_duration_type& elapsed() const
    {
        if(m_start)
        {
            uint32_t current = timeGetTime();
            int64_t milliseconds = current - m_start;
            m_elapsed += posix_time::milliseconds(milliseconds);
            m_start = current;
        }
        return m_elapsed;
    }

    void reset()
    {
        m_elapsed = time_duration_type(0,0,0);
        m_start = 0;
    }


  private:

    mutable uint32_t            m_start;
    mutable time_duration_type  m_elapsed;

};

} // namespace win32
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_WIN32_TGT_DEVICE_HPP
