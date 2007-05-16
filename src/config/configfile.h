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
#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include "tinyxml.h"
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

#include <boost/shared_ptr.hpp>


using namespace std;

enum ConfigState { NONEXISTING, OLD };
enum ConfigType { CONFIG_TYPE_INT, CONFIG_TYPE_STRING };


class ConfigFile{
public:
    ConfigFile(int, char **);

    ~ConfigFile();
	
	void fillBuffer();

	void updateConfig(ConfigState);

	std::string readConfigString(std::string varName);
    	void writeConfigString(std::string varName, std::string varCont);
    	
	int readConfigInt(std::string varName);
	void writeConfigInt(std::string varName, int varCont);
    	

private:
	

	struct ConfigInfo
	{
		ConfigInfo(const std::string &n, ConfigType t, const std::string &d) : name(n), type(t), defaultValue(d) {}
		std::string name;
		ConfigType type;
		std::string defaultValue;
	};
	
	vector<ConfigInfo> configList;
	vector<ConfigInfo> configBufferList;

	std::string configFileName;
	std::string logDir;
	std::string dataDir;
	int configRev;
	bool noWriteAccess;

	std::string logOnOffDefault;
	std::string claNoWriteAccess;

	ConfigState myConfigState;
};

#endif
