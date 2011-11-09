/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/

#define NOMINMAX // for Windows

#include "tools.h"
#include <core/loghelper.h>
#include <core/openssl_wrapper.h>

#include <boost/thread.hpp>
#include <boost/nondet_random.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <set>
#include <limits>


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

void Tools::GetRandUnique(int minValue, int maxValue, unsigned count, int *out)
{
	InitRandState();
	unsigned numCreated = 0;
	int *startPtr = out;
	set<int> tmpSet;
	for (unsigned i = 0; i < count; i++) {
		boost::uniform_int<> dist(minValue, maxValue - numCreated);
		boost::variate_generator<boost::random_device&, boost::uniform_int<> > gen(*g_rand_state, dist);
		*startPtr = gen();
		set<int>::const_iterator v = tmpSet.begin();
		set<int>::const_iterator end = tmpSet.end();
		while (v != end) {
			if (*v <= *startPtr) {
				(*startPtr)++;
			}
			++v;
		}
		tmpSet.insert(*startPtr);
		startPtr++;
		numCreated++;
	}
}

/*
void Tools::getRandNumber(int start, int end, int howMany, int* randArray, bool different, int* bad, int countBad)
{

	int r = end-start+1;
	unsigned char rand_buf[4];
	unsigned int randNumber;

	int i;

	if (!different) {

		int counter(0);
		while(counter < howMany) {

#ifdef HAVE_OPENSSL
			if(!RAND_bytes(rand_buf, 4)) {
				LOG_MSG("RAND_bytes failed!");
			}
#else
			gcry_randomize(rand_buf, 4, GCRY_VERY_STRONG_RANDOM);
#endif

			randNumber = 0;
			for(i=0; i<4; i++) {
				randNumber += (rand_buf[i] << 8*i);
			}

			if(randNumber < ( (unsigned int) ( ((double)numeric_limits<unsigned int>::max()) / r ) ) * r) {
				randArray[counter] = start + (randNumber % r);
				counter++;
			}

		}
	} else {

		int *tempArray = new int[end-start+1];
		for (i=0; i<(end-start+1); i++) tempArray[i]=1;

		if(bad) {
			for(i=0; i<countBad; i++) {
				tempArray[bad[i]]=0;
			}
		}

		int counter(0);
		while (counter < howMany) {

#ifdef HAVE_OPENSSL
			if(!RAND_bytes(rand_buf, 4)) {
				LOG_MSG("RAND_bytes failed!");
			}
#else
			gcry_randomize(rand_buf, 4, GCRY_VERY_STRONG_RANDOM);
#endif

			randNumber = 0;
			for(i=0; i<4; i++) {
				randNumber += (rand_buf[i] << 8*i);
			}

			if(randNumber < ( (unsigned int) ( ((double)numeric_limits<unsigned int>::max()) / r ) ) * r) {
				randNumber = randNumber % r;

				if (tempArray[randNumber] == 1) {

					randArray[counter] = start + randNumber;
					tempArray[randNumber] = 0;
					counter++;
				}
			}

		}

		delete[] tempArray;
	}

}*/

