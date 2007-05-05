#ifndef BOOST_TIMER_DEVICES_CLOCK_HPP
#define BOOST_TIMER_DEVICES_CLOCK_HPP

// (C) Copyright Vaucher Philippe 2006

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2006

// -------------------------- std::clock() --------------------------

// Properties:
//   - ??? ns resolution.
//   - ??? ns overhead.
//   - Is implemented with GetSystemTimeAsFileTime on windows.
//
// Pros:
//   - Portable.
//
// Cons:
//   - High resolution.


// -------------------------- Includes --------------------------

// Standard headers
#include <ctime>

// Boost headers
#include <boost/date_time/posix_time/posix_time.hpp>


// -------------------------- Code --------------------------

namespace boost
{

//! The clock_device class is based on std::clock() to compute the time
class clock_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit clock_device() : m_start(0), m_elapsed(0, 0, 0)
    {
    }

    void start() 
    {
        if(!m_start)
            m_start = std::clock();
    }

    const time_duration_type& elapsed() const
    {
        if(m_start)
        {
            std::clock_t current = std::clock();
            m_elapsed += posix_time::milliseconds(static_cast<double>(current-m_start) / CLOCKS_PER_SEC * 1000.0);
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

    mutable std::clock_t        m_start;
    mutable time_duration_type  m_elapsed;

};

} // namespace boost

#endif // BOOST_TIMER_DEVICES_CLOCK_HPP
