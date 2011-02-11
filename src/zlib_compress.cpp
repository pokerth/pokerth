/***************************************************************************
 *   Copyright (C) 2008 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <iostream>
#include <fstream>
#include <string>


using namespace std;
using namespace boost::filesystem;


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		cout << "Usage: zlib_compress <text input file>" << endl;
		return 1;
	}
	// Use input file name, and write to input file with additional ".z".
	path inputFilePath(argv[1]);
	path outputFilePath(inputFilePath);
	outputFilePath = change_extension(outputFilePath, extension(outputFilePath) + ".z");
	// Check whether file exists.
	if (!exists(inputFilePath)) {
		cerr << "Input file does not exist." << endl;
		return 2;
	}
	try {
		ifstream inFile(inputFilePath.directory_string().c_str(), ios_base::in);
		ofstream outFile(outputFilePath.directory_string().c_str(), ios_base::out | ios_base::binary);
		boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
		out.push(boost::iostreams::zlib_compressor());
		out.push(outFile);
		boost::iostreams::copy(inFile, out);
		cout << "Compression of \"" << inputFilePath.directory_string() << "\" to \"" << outputFilePath.directory_string() << "\" was successful." << endl;
	} catch (...) {
		cerr << "Compression failed." << endl;
		return 3;
	}

	return 0;
}

