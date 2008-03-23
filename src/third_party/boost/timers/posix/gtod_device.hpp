#ifndef _BOOST_TIMERS_POSIX_GTOD_DEVICE_HPP
#define _BOOST_TIMERS_POSIX_GTOD_DEVICE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

// Boost headers
#include <boost/date_time/posix_time/posix_time.hpp>

// Linux headers
#include <sys/time.h>

// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace posix
{

//! The gtod_device class is based on gettimeofday() to compute the time
class gtod_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit gtod_device() : m_elapsed(0, 0, 0)
    {
        std::memset(&m_start, 0, sizeof(timeval));
    }

    void start() 
    {
        if(!is_time(m_start))
            gettimeofday(&m_start, 0);
    }

    const time_duration_type& elapsed() const
    {
        if(is_time(m_start))
        {
            timeval current;
            gettimeofday(&current, 0);
            int64_t seconds = current.tv_sec - m_start.tv_sec;
            int64_t microseconds = current.tv_usec - m_start.tv_usec;
            m_elapsed += posix_time::microseconds(seconds * 1000000 + microseconds);
            m_start = current;
        }
        return m_elapsed;
    }

    void reset()
    {
        m_elapsed = time_duration_type(0,0,0);
        std::memset(&m_start, 0, sizeof(timeval));
    }


  private:

    bool is_time(const timeval& value) const
    {
        return value.tv_sec || value.tv_usec;
    }

    mutable timeval             m_start;
    mutable time_duration_type  m_elapsed;

};

} // namespace posix
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_POSIX_GTOD_DEVICE_HPP
