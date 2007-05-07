#ifndef BOOST_TIMER_HPP
#define BOOST_TIMER_HPP

// (C) Copyright Vaucher Philippe 2006

// Use, modification and distribution is subject to the 
// Boost Software License, Version 1.0. (See accompanying
// file LICENSE-1.0 or http://www.boost.org/LICENSE-1.0)
// Author: Vaucher Philippe 2006

// Speeds up compilation a bit on msvc
#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

// -------------------------- Includes --------------------------

#include "boost/timer/implementation.hpp"
#include "boost/timer/devices.hpp"
#include "boost/timer/typedefs.hpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // BOOST_TIMER_HPP
