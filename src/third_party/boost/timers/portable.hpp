#ifndef _BOOST_TIMERS_PORTABLE_HPP
#define _BOOST_TIMERS_PORTABLE_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

#include "timer.hpp"
#include "portable/clock_device.hpp"
#include "portable/date_time_devices.hpp"
#include "portable/time_device.hpp"


// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace portable
{

    // boost::date_time devices
    typedef timer<microsec_device>      microsec_timer;
    typedef timer<second_device>        second_timer;

    // std::clock() device
    typedef timer<clock_device>         clock_timer;

    // std::time() device
    typedef timer<time_device>          time_timer;

    // Typedef which timer is the best
    typedef microsec_timer              best_timer;

} // namespace portable
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_PORTABLE_HPP
