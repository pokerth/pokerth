/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#define NOMINMAX // for Windows

#ifdef ANDROID
#error This file is not for android.
#endif

#include "tools.h"
#include <core/loghelper.h>
#include <core/openssl_wrapper.h>

#include <boost/thread.hpp>
#include <boost/nondet_random.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>


using namespace std;

boost::thread_specific_ptr<boost::random_device> g_rand_state;

struct nondet_rng : std::unary_function<unsigned, unsigned> {
	boost::random_device &_state;
	unsigned operator()(unsigned i) {
		boost::uniform_int<> rng(0, i - 1);
		return rng(_state);
	}
	nondet_rng(boost::random_device &state) : _state(state) {}
};

static inline void InitRandState()
{
	if (!g_rand_state.get()) {
		g_rand_state.reset(new boost::random_device);
	}
}

void Tools::ShuffleArrayNonDeterministic(int *inout, unsigned count)
{
	InitRandState();
	nondet_rng rand(*g_rand_state);
	random_shuffle(&inout[0], &inout[count], rand);
}

void Tools::GetRand(int minValue, int maxValue, unsigned count, int *out)
{
	InitRandState();
	boost::uniform_int<> dist(minValue, maxValue);
	boost::variate_generator<boost::random_device&, boost::uniform_int<> > gen(*g_rand_state, dist);
	int *startPtr = out;
	for (unsigned i = 0; i < count; i++) {
		*startPtr++ = gen();
	}
}

