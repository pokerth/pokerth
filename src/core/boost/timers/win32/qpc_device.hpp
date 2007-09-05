#ifndef _BOOST_TIMERS_WIN32_QPC_DEVICE_HPP
#define _BOOST_TIMERS_WIN32_QPC_DEVICE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

// Boost headers
#include <boost/throw_exception.hpp>
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

class qpc_device
{

  public:

    typedef posix_time::ptime               time_type;
    typedef time_type::time_duration_type   time_duration_type;

    explicit qpc_device() : m_elapsed(0, 0, 0)
    {
        #ifdef BOOST_QPC_SET_AFFINITY
        m_affinity = SetThreadAffinityMask(GetCurrentThread(), 1);
        #endif

        m_start.QuadPart = 0;
        if(!QueryPerformanceFrequency(&m_frequency))
            throw_exception(std::runtime_error("qpc_device: QueryPerformanceFrequency() indicated there's no high-resolution performance counter"));

        // Make it so we have the future results directly in milliseconds
        m_frequency.QuadPart /= 1000;
    }

    ~qpc_device()
    {
        #ifdef BOOST_QPC_SET_AFFINITY
        SetThreadAffinityMask(GetCurrentThread(), m_affinity);
        #endif
    }

    void start() 
    {
        if(!m_start.QuadPart && !QueryPerformanceCounter(&m_start))
            throw_exception(std::runtime_error("qpc_device: QueryPerformanceCounter() failed"));
    }

    const time_duration_type& elapsed() const
    {
        if(m_start.QuadPart)
        {
            LARGE_INTEGER current;
            if(!QueryPerformanceCounter(&current))
                throw_exception(std::runtime_error("qpc_device: QueryPerformanceCounter() failed"));
            int64_t milliseconds = (current.QuadPart - m_start.QuadPart) / m_frequency.QuadPart;
            m_elapsed += posix_time::milliseconds(milliseconds);
            m_start = current;
        }
        return m_elapsed;
    }

    void reset()
    {
        m_elapsed = time_duration_type(0,0,0);
        m_start.QuadPart = 0;
    }


  private:

    LARGE_INTEGER               m_frequency;
    mutable LARGE_INTEGER       m_start;
    mutable time_duration_type  m_elapsed;

    #ifdef BOOST_QPC_SET_AFFINITY
    DWORD_PTR                   m_affinity;
    #endif

};

} // namespace win32
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_WIN32_QUERYPERFORMANCECOUNTER_HPP
