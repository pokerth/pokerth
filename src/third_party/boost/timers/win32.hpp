#ifndef _BOOST_TIMERS_WIN32_HPP
#define _BOOST_TIMERS_WIN32_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007


// -------------------------- Includes --------------------------

#include "timer.hpp"
#include "win32/gpt_device.hpp"
#include "win32/gtc_device.hpp"
#include "win32/gtt_device.hpp"
#include "win32/gstaft_device.hpp"
#include "win32/qpc_device.hpp"
#include "win32/tgt_device.hpp"


// -------------------------- Code --------------------------

namespace boost
{
namespace timers
{
namespace win32
{

    // GetProcessTimes() device
    typedef timer<gpt_device>       gpt_timer;

    // GetThreadTimes() device
    typedef timer<gtt_device>       gtt_timer;

    // GetTickCount() device
    typedef timer<gtc_device>       gtc_timer;

    // GetSystemTimeAsFileTime() device
    typedef timer<gstaft_device>    gstaft_timer;

    // QueryPerformanceCounter() device
    typedef timer<qpc_device>       qpc_timer;

    // timeGetTime() device
    typedef timer<tgt_device>       tgt_timer;

    // Typedef which timer is the best
    typedef qpc_timer               best_timer;

} // namespace win32
} // namespace timers
} // namespace boost

#endif // _BOOST_TIMERS_WIN32_HPP
