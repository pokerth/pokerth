#ifndef BOOST_TIMER_DEVICES_GTC_HPP
#define BOOST_TIMER_DEVICES_GTC_HPP

// (C) Copyright Vaucher Philippe 2006

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2006

// -------------------------- GetTickCount() --------------------------

// Properties:
//   - 5 to 55 ms resolution.
//   - ??? ns overhead (8 clock (load, mul, shift)).
//
// Pros:
//   - Very low overhead.
//
// Cons:
//   - Very low resolution.


// -------------------------- Includes --------------------------

#ifndef BOOST_WINDOWS
    #error "GetTickCount() can only be used on windows"
#else

// Boost headers
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


// -------------------------- Code --------------------------

namespace boost
{

//! The gtc_device class is based on GetTickCount() to compute the time
class gtc_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit gtc_device() : m_start(0), m_elapsed(0, 0, 0)
    {
    }

    void start() 
    {
        if(!m_start)
            m_start = GetTickCount();
    }

    const time_duration_type& elapsed() const
    {
        if(m_start)
        {
            uint32_t current = GetTickCount();
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

#endif // BOOST_TIMER_DEVICES_GTC_HPP
