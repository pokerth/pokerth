#ifndef BOOST_TIMER_TYPEDEFS_HPP
#define BOOST_TIMER_TYPEDEFS_HPP

// (C) Copyright Vaucher Philippe 2006

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2006


// -------------------------- Includes --------------------------

#include <boost/date_time/posix_time/posix_time.hpp>

#include "boost/timer/implementation.hpp"
#include "boost/timer/devices.hpp"


// -------------------------- Devices --------------------------

namespace boost
{

// boost::date_time devices
typedef timer<microsec_device>   microsec_timer;
typedef timer<second_device>     second_timer;

// std::clock() device
typedef timer<clock_device> clock_timer;

#ifndef BOOST_WINDOWS

    // gettimeofday() device
    typedef timer<gtod_device> gtod_timer;

#else

    // QueryPerformanceCounter() device
    typedef timer<qpc_device> qpc_timer;

    // timeGetTime() device
    typedef timer<tgt_device> tgt_timer;

    // GetSystemTimeAsFileTime() device
    typedef timer<gstaft_device> gstaft_timer;

    // GetTickCount() device
    typedef timer<gtc_device> gtc_timer;

#endif

} // namespace boost

#endif // BOOST_TIMER_TYPEDEFS_HPP
