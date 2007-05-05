#ifndef BOOST_TIMER_DEVICES_QPC_HPP
#define BOOST_TIMER_DEVICES_QPC_HPP

// (C) Copyright Vaucher Philippe 2006

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2006


// -------------------------- QueryPerformanceCounter() --------------------------

// Properties:
//   Win2k (PIT):
//     - 838 ns resolution.
//     - ~3000 ns overhead.
//   WinXP (PMT):
//     - 279 ns resolution.
//     - 700 ns overhead.
//
// Pros:
//   - High resolution (usually the best one).
//
// Cons:
//   - High overhead.
//   - Not thread safe.
//   - Multi core problems. To solve the issue you can #define BOOST_QPC_SET_AFFINITY which will call SetThreadAffinityMask() for you.
//
// Issues:
//   1) Q274323: may jump several seconds under heavy PCI bus load.
//      not a problem, because the older systems on which this occurs
//      have safe TSCs, so that is used instead.
//   2) "System clock problem can inflate benchmark scores":
//      incorrect value if not polled every 4.5 seconds?


// -------------------------- Includes --------------------------

#ifndef BOOST_WINDOWS
    #error "QueryPerformanceCounter() can only be used on windows"
#else

// Boost headers
#include <boost/throw_exception.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


// -------------------------- Code --------------------------

namespace boost
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
            uint64_t milliseconds = (current.QuadPart - m_start.QuadPart) / m_frequency.QuadPart;
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

} // namespace boost

#endif // BOOST_WINDOWS

#endif // BOOST_TIMER_DEVICES_QPC_HPP
