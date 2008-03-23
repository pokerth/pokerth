#ifndef _BOOST_TIMERS_PORTABLE_TIME_DEVICE_HPP
#define _BOOST_TIMERS_PORTABLE_TIME_DEVICE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

// Standard headers
#include <ctime>


// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace portable
{

//! The time_device class is based on std::time() to compute the time
class time_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit time_device() : m_start(0), m_elapsed(0, 0, 0)
    {
    }

    void start() 
    {
        if(!m_start)
            m_start = std::time(0);
    }

    const time_duration_type& elapsed() const
    {
        if(m_start)
        {
            std::time_t current = std::time(0);
            long seconds = static_cast<long>(current - m_start);
            m_elapsed += posix_time::seconds(seconds);
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

    mutable std::time_t         m_start;
    mutable time_duration_type  m_elapsed;

};

} // namespace portable
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_PORTABLE_TIME_DEVICE_HPP
