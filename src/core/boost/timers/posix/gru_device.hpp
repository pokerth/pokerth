#ifndef _BOOST_TIMERS_POSIX_GRU_DEVICE_HPP
#define _BOOST_TIMERS_POSIX_GRU_DEVICE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

// Boost headers
#include <boost/date_time/posix_time/posix_time.hpp>

// Linux headers
#include <sys/times.h>
#include <sys/resource.h>


// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace posix
{

//! The gru_device class is based on getrusage() to compute the time
class gru_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit gru_device() : m_elapsed(0, 0, 0)
    {
        std::memset(&m_start, 0, sizeof(timeval));
    }

    void start() 
    {
        if(!is_time(m_start))
            getrusage(RUSAGE_SELF, &m_start);
    }

    const time_duration_type& elapsed() const
    {
        if(is_time(m_start))
        {
            rusage current;
            getrusage(RUSAGE_SELF, &current);
            int64_t seconds = current.ru_utime.tv_sec - m_start.ru_utime.tv_sec;
            int64_t microseconds = current.ru_utime.tv_usec - m_start.ru_utime.tv_usec;
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

    bool is_time(const rusage& value) const
    {
        return value.ru_utime.tv_sec || value.ru_utime.tv_usec;
    }

    mutable rusage              m_start;
    mutable time_duration_type  m_elapsed;

};

} // namespace posix
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_POSIX_GRU_DEVICE_HPP
