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

// Connectivity test program for PokerTH

#include <iostream>
#include <boost/thread.hpp>
#include <boost/nondet_random.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>


using namespace std;
namespace po = boost::program_options;
boost::thread_specific_ptr<boost::random_device> g_rand_state;

struct nondet_rng : std::unary_function<unsigned, unsigned> {
	boost::random_device &_state;
	unsigned operator()(unsigned i)
	{
		boost::uniform_int<> rng(0, i - 1);
		return rng(_state);
	}
	nondet_rng(boost::random_device &state) : _state(state) {}
};


int
main(int argc, char *argv[])
{
	try {
		// Check command line options.
		po::options_description desc("Allowed options");
		desc.add_options()
		("help,h", "produce help message")
		("num,n", po::value<int>(), "set num of deals")
		;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << desc << endl;
			return 1;
		}
		if (!vm.count("mode")) {
			cout << "Missing option!" << endl << desc << endl;
			return 1;
		}

		int num = vm["num"].as<int>();
        const int NumCards = 52;

        for(int i=0; i<=num;i++){
            // shuffle cards
            int cardsArray[NumCards] = {
                0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                50, 51
            };
            if (!g_rand_state.get()) {
                g_rand_state.reset(new boost::random_device);
            }
            nondet_rng rand(*g_rand_state);
            random_shuffle(&cardsArray[0], &cardsArray[NumCards], rand);

            ostringstream oss("");
            for (int temp = 0; temp < size_of_array; temp++)
                oss << int_array[temp];
            cout << oss << endl;

	} catch (...) {
		cout << "Exception caught" << endl;
		return 1;
	}

	return 0;
}

