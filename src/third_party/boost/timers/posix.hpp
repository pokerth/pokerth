#ifndef _BOOST_TIMERS_POSIX_HPP
#define _BOOST_TIMERS_POSIX_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

#include "timer.hpp"
#include "posix/ftime_device.hpp"
#include "posix/gru_device.hpp"
#include "posix/gtod_device.hpp"


// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace posix
{

    // ftime() device
    typedef timer<ftime_device>     ftime_timer;

    // getcputime() device
    typedef timer<gru_device>       gru_timer;

    // gettimeofday() device
    typedef timer<gtod_device>      gtod_timer;

    // Typedef which timer is the best
    typedef gtod_timer              best_timer;

} // namespace posix
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_POSIX_HPP
