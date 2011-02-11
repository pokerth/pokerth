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

#include <vector>
#include <string>

#include <boost/thread.hpp>

enum ConfigState { NONEXISTING, OLD, OK };
enum ConfigType { CONFIG_TYPE_INT, CONFIG_TYPE_STRING, CONFIG_TYPE_INT_LIST, CONFIG_TYPE_STRING_LIST };

class QtToolsInterface;

class ConfigFile
{
public:
	ConfigFile(char *argv0, bool readonly);

	~ConfigFile();

	void fillBuffer();
	void writeBuffer() const;

	void updateConfig(ConfigState);
	ConfigState getConfigState() const {
		return myConfigState;
	}

	std::string readConfigString(std::string varName) const;
	std::list<std::string> readConfigStringList(std::string varName) const;
	void writeConfigString(std::string varName, std::string varCont);
	void writeConfigStringList(std::string varName, std::list<std::string> varCont);
	int readConfigInt(std::string varName) const;
	std::list<int> readConfigIntList(std::string varName) const;
	void writeConfigInt(std::string varName, int varCont);
	void writeConfigIntList(std::string varName, std::list<int> varCont);


private:

	mutable boost::recursive_mutex m_configMutex;

	struct ConfigInfo {
		ConfigInfo(const std::string &n, ConfigType t, const std::string &d, const std::list<std::string> &l =std::list<std::string>()) : name(n), type(t), defaultValue(d), defaultListValue(l) {}
		std::string name;
		ConfigType type;
		std::string defaultValue;
		std::list<std::string> defaultListValue;

	};

	std::vector<ConfigInfo> configList;
	std::vector<ConfigInfo> configBufferList;

	std::string configFileName;
	std::string logDir;
	std::string dataDir;
	std::string cacheDir;
	std::string defaultGameTableStyle;
	std::string defaultCardDeck;

	int configRev;
	bool noWriteAccess;

	std::string logOnOffDefault;

	ConfigState myConfigState;
	QtToolsInterface *myQtToolsInterface;

	char *myArgv0;
};

#endif
