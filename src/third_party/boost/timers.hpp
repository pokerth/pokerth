#ifndef _BOOST_TIMERS_HPP
#define _BOOST_TIMERS_HPP

// (C) Copyright Vaucher Philippe 2007

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2007

// -------------------------- Includes --------------------------

// Speeds up compilation a bit on msvc
#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#define _WINSOCKAPI_
#endif

#include <boost/config.hpp>

#include "timers/timer.hpp"
#include "timers/portable.hpp"

/*#ifdef BOOST_WINDOWS
    #include "timers/win32.hpp"
#else
    #include "timers/posix.hpp"
#endif*/ // Modified by Lothar May: We only need the portable timers.

#endif // _BOOST_TIMERS_HPP
