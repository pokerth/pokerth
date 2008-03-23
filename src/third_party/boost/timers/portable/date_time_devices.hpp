#ifndef _BOOST_TIMERS_PORTABLE_DATE_TIME_DEVICES_HPP
#define _BOOST_TIMERS_PORTABLE_DATE_TIME_DEVICES_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

#include <boost/date_time/posix_time/posix_time.hpp>


// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace portable
{

//! The date_time device
template <class Clock, class Time = posix_time::ptime>
class date_time_device
{

  public:

    typedef Clock clock_type;
    typedef Time  time_type;
    typedef typename clock_type::time_duration_type time_duration_type;

    explicit date_time_device() : m_start(date_time::not_a_date_time), m_elapsed(0, 0, 0)
    {
    }

    void start()
    {
        if(m_start.is_not_a_date_time())
            m_start = clock_type::local_time();
    }

    const time_duration_type& elapsed() const
    {
        if(!m_start.is_not_a_date_time())
        {
            time_type current(clock_type::local_time());
            m_elapsed += (current - m_start);
            m_start = current;
        }
        return m_elapsed;
    }

    void reset()
    {
        m_start   = date_time::not_a_date_time;
        m_elapsed = time_duration_type(0,0,0);
    }


  private:

    mutable time_type           m_start;
    mutable time_duration_type  m_elapsed;

};

// Convenience typedefs
typedef date_time_device<posix_time::microsec_clock>    microsec_device;
typedef date_time_device<posix_time::second_clock>      second_device;

} // namespace portable
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_PORTABLE_DATE_TIME_DEVICES_HPP
