#ifndef BOOST_TIMER_DEVICES_GSTAFT_HPP
#define BOOST_TIMER_DEVICES_GSTAFT_HPP

// (C) Copyright Vaucher Philippe 2006

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2006


// -------------------------- GetSystemTimeAsFileTime() --------------------------

// Properties:
//   - ??? ns resolution.
//   - ??? ns overhead (6 clocks (64-bit load/store + synchronization (4))).
//
// Pros:
//   - Low overhead.
//
// Cons:
//   - Low resolution.


// -------------------------- Includes --------------------------

#ifndef BOOST_WINDOWS
    #error "GetSystemTimeAsFileTime() can only be used on windows"
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

//! The gstaft_device class is based on GetSystemTimeAsFileTime() to compute the time
class gstaft_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit gstaft_device() : m_elapsed(0, 0, 0)
    {
        std::memset(&m_start, 0, sizeof(ULARGE_INTEGER));
    }

    void start() 
    {
        if(!m_start.QuadPart)
            GetSystemTimeAsFileTime(reinterpret_cast<LPFILETIME>(&m_start));
    }

    const time_duration_type& elapsed() const
    {
        if(m_start.QuadPart)
        {
            ULARGE_INTEGER current;
            GetSystemTimeAsFileTime(reinterpret_cast<LPFILETIME>(&current));
            uint64_t tmp = current.QuadPart - m_start.QuadPart;
            m_elapsed += posix_time::milliseconds(tmp / 10000.0);
            m_start = current;
        }
        return m_elapsed;
    }

    void reset()
    {
        m_elapsed = time_duration_type(0,0,0);
        std::memset(&m_start, 0, sizeof(ULARGE_INTEGER));
    }


  private:

    mutable ULARGE_INTEGER     m_start;
    mutable time_duration_type m_elapsed;

};

} // namespace boost

#endif // BOOST_WINDOWS

#endif // BOOST_TIMER_DEVICES_GSTAFT_HPP
