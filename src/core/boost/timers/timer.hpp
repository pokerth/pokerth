#ifndef _BOOST_TIMERS_TIMER_HPP
#define _BOOST_TIMERS_TIMER_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Code --------------------------

namespace boost
{

namespace timers
{

//! The timer class takes a device type that's used for the elapsed time computation.
//! Calling start(), pause() or resume() twice has no special effects.
template<class Device>
class timer
{

  public:

    typedef Device                                      device_type;
    typedef typename device_type::time_duration_type    time_duration_type;

    enum start_options { auto_start = 1, manual_start };

    //! By default, start the timer without any time offset
    explicit timer(time_duration_type initial_duration = time_duration_type(0, 0, 0), start_options start_op = auto_start)
    : m_elapsed(initial_duration), m_running(false)
    {
        if(start_op == auto_start)
            start();
    }

    //! Starts the timer
    void start() 
    {
        if(!m_running)
        {
            m_device.start();
            m_running = true;
        }
    }

    //! Restarts the timer
    void restart()
    {
        reset();
        start();
    }

    //! Returns the elapsed time
    time_duration_type elapsed() const
    {
        return m_running ? (m_device.elapsed() + m_elapsed) : m_elapsed;
    }

    //! Pauses the timer
    void pause()
    {
        if(m_running)
        {
            m_elapsed += m_device.elapsed();
            m_device.reset();
            m_running = false;
        }
    }

    //! Resumes the timer
    void resume()
    {
        if(!m_running)
        {
            m_device.start();
            m_running = true;
        }
    }

    //! Resets the timer
    void reset()
    {
        m_elapsed = time_duration_type(0,0,0);
        m_device.reset();
        m_running = false;
    }

    //! Tells wether the timer is currently running or not
    bool is_running() const
    {
        return m_running;
    }


  private:

    mutable device_type         m_device;
    mutable time_duration_type  m_elapsed;
    bool                        m_running;

};

} // namespace timers

} // namespace boost

#endif // _BOOST_TIMERS_TIMER_HPP
