#ifndef _BOOST_TIMERS_WIN32_GPT_DEVICE_HPP
#define _BOOST_TIMERS_WIN32_GPT_DEVICE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

// Standard headers
#include <utility>

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

//! The gtt_device class is based on GetThreadTimes() to compute the time
class gpt_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit gpt_device() : m_start(0, 0), m_elapsed(0, 0, 0)
    {
    }

    void start() 
    {
        if(!is_time(m_start))
        {
            FILETIME tmp1, tmp2;
            GetProcessTimes(GetCurrentProcess(), &tmp1, &tmp2, reinterpret_cast<LPFILETIME>(&m_start.first), reinterpret_cast<LPFILETIME>(&m_start.second));
        }
    }

    const time_duration_type& elapsed() const
    {
        if(is_time(m_start))
        {
            std::pair<uint64_t, uint64_t> current;
            FILETIME tmp1, tmp2;
            GetProcessTimes(GetCurrentProcess(), &tmp1, &tmp2, reinterpret_cast<LPFILETIME>(&current.first), reinterpret_cast<LPFILETIME>(&current.second));
            // diff is measured in 100ns intervals...
            uint64_t diff = (current.first - m_start.first) + (current.second - m_start.second);
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
        m_start.first = m_start.second = 0;
    }


  private:

    bool is_time(const std::pair<uint64_t, uint64_t>& time) const
    {
        return (time.first != 0 || time.second != 0);
    }

    mutable std::pair<uint64_t, uint64_t>   m_start;
    mutable time_duration_type              m_elapsed;

};

} // namespace win32
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_WIN32_GTT_DEVICE_HPP
