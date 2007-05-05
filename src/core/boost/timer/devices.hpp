#ifndef BOOST_TIMER_DEVICES_HPP
#define BOOST_TIMER_DEVICES_HPP

// (C) Copyright Vaucher Philippe 2006

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2006


// -------------------------- Devices --------------------------

//! A device has 3 main member functions: start(), elapsed() & reset().


// -------------------------- Includes --------------------------

#include "boost/timer/devices/clock.hpp"
#include "boost/timer/devices/date_time.hpp"

#ifndef BOOST_WINDOWS
    #include "boost/timer/devices/gettimeofday.hpp"
#else
    #include "boost/timer/devices/QueryPerformanceCounter.hpp"
    #include "boost/timer/devices/timeGetTime.hpp"
    #include "boost/timer/devices/GetSystemTimeAsFileTime.hpp"
    #include "boost/timer/devices/GetTickCount.hpp"
#endif

#endif // BOOST_TIMER_DEVICES_HPP
