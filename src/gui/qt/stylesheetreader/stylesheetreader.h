/***************************************************************************
 *   Copyright (C) 2006 by Felix Hammer   *
 *   f.hammer@web.de   *
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
#ifndef STYLESHEETREADER_H
#define STYLESHEETREADER_H

#include "tinyxml.h"
#include <vector>
#include <string>

#include <boost/thread.hpp>


class StyleSheetReader{
public:
	StyleSheetReader(std::string f);
	~StyleSheetReader();
	
	void fillBuffer();

	std::string readColorString(std::string varName) const;

private:

	mutable boost::recursive_mutex m_configMutex;

	struct Colors
	{
		Colors(const std::string &n, const std::string &v) : name(n), value(v) {}
		std::string name;
		std::string value;
	};
	
	std::vector<Colors> colorsBufferList;

	std::string styleSheetFileName;
};

#endif
