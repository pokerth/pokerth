#ifndef _BOOST_TIMERS_WIN32_GSTAFT_DEVICE_HPP
#define _BOOST_TIMERS_WIN32_GSTAFT_DEVICE_HPP

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


// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace win32
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
            // diff is measured in 100ns intervals...
            uint64_t diff = current.QuadPart - m_start.QuadPart;
            // ... so we must divide by 10 to get nanoseconds
            uint64_t nanoseconds = static_cast<uint64_t>(diff / 10.0);
            m_elapsed += posix_time::milliseconds(static_cast<int64_t>(nanoseconds / 1000.0));
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

} // namespace win32
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_WIN32_GSTAFT_DEVICE_HPP
