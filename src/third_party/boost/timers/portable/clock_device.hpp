 #ifndef _BOOST_TIMERS_PORTABLE_CLOCK_DEVICE_HPP
#define _BOOST_TIMERS_PORTABLE_CLOCK_DEVICE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

// Standard headers
#include <ctime>

// Boost headers
#include <boost/date_time/posix_time/posix_time.hpp>


// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace portable
{

//! The clock_device class is based on std::clock() to compute the time
class clock_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit clock_device() : m_running(false), m_start(0), m_elapsed(0, 0, 0)
    {
    }

    void start() 
    {
        if(!m_running)
        {
            m_start     = std::clock();
            m_running   = true;
        }
    }

    const time_duration_type& elapsed() const
    {
        if(m_running)
        {
            std::clock_t current = std::clock();
            double seconds = (current - m_start) / static_cast<double>(CLOCKS_PER_SEC);
            m_elapsed += posix_time::milliseconds(static_cast<int64_t>(seconds * 1000.0));
            m_start = current;
        }
        return m_elapsed;
    }

    void reset()
    {
        m_elapsed   = time_duration_type(0,0,0);
        m_start     = 0;
        m_running   = false;
    }


  private:

    bool                        m_running;
    mutable std::clock_t        m_start;
    mutable time_duration_type  m_elapsed;

};

} // namespace portable
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_PORTABLE_CLOCK_DEVICE_HPP
