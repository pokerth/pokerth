#ifndef BOOST_TIMER_DEVICES_TGT_HPP
#define BOOST_TIMER_DEVICES_TGT_HPP

// (C) Copyright Vaucher Philippe 2006

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2006

// -------------------------- std::clock() --------------------------

// Properties:
//   - 1 ms resolution.
//   - ??? ns overhead.
//
// Pros:
//   - Low overhead.
//
// Cons:
//   - Low resolution.

// -------------------------- Includes --------------------------

#ifndef BOOST_WINDOWS
    #error "timeGetTime() can only be used on windows"
#else

// Boost headers
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Automagically link winmm.lib if we can
#pragma comment(lib, "winmm.lib")


// -------------------------- Code --------------------------

namespace boost
{

//! The tgt_device class is based on timeGetTime() to compute the time
class tgt_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit tgt_device() : m_start(0), m_elapsed(0, 0, 0)
    {
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
            m_elapsed += posix_time::milliseconds(current - m_start);
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

    mutable uint32_t m_start;
    mutable time_duration_type m_elapsed;

};

} // namespace boost

#endif // BOOST_WINDOWS

#endif // BOOST_TIMER_DEVICES_TGT_HPP
