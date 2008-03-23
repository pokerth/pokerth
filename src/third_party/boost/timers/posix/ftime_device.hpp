#ifndef _BOOST_TIMERS_POSIX_FTIME_DEVICE_HPP
#define _BOOST_TIMERS_POSIX_FTIME_DEVICE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

// Boost headers
#include <boost/date_time/posix_time/posix_time.hpp>

// Linux headers
#include <sys/timeb.h>

// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace posix
{

//! The gtod_device class is based on gettimeofday() to compute the time
class ftime_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit ftime_device() : m_elapsed(0, 0, 0)
    {
        std::memset(&m_start, 0, sizeof(timeb));
    }

    void start() 
    {
        if(!is_time(m_start))
            ftime(&m_start);
    }

    const time_duration_type& elapsed() const
    {
        if(is_time(m_start))
        {
            timeb current;
            ftime(&current);
            int64_t seconds = current.time - m_start.time;
            int64_t milliseconds = current.millitm - m_start.millitm;
            m_elapsed += posix_time::milliseconds(seconds * 1000 + milliseconds);
            m_start = current;
        }
        return m_elapsed;
    }

    void reset()
    {
        m_elapsed = time_duration_type(0,0,0);
        std::memset(&m_start, 0, sizeof(timeb));
    }


  private:

    bool is_time(const timeb& value) const
    {
        return value.time != 0;
    }

    mutable timeb               m_start;
    mutable time_duration_type  m_elapsed;

};

} // namespace posix
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_POSIX_FTIME_DEVICE_HPP
