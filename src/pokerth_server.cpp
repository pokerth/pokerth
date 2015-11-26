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

#include <net/netpacket.h>
#include "session.h"
#include "configfile.h"
#include <qttoolsinterface.h>
#include <gui/generic/serverguiwrapper.h>
#include <net/socket_startup.h>
#include <core/loghelper.h>
#include <core/thread.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <csignal>

#ifdef _MSC_VER
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define ENABLE_LEAK_CHECK() \
			{ \
				int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); \
				tmpFlag |= _CRTDBG_LEAK_CHECK_DF; \
				_CrtSetDbgFlag(tmpFlag); \
			}
#endif
#endif

#ifndef ENABLE_LEAK_CHECK
#define ENABLE_LEAK_CHECK()
#endif

using namespace std;
namespace po = boost::program_options;
using namespace boost::filesystem;

volatile int g_pokerthTerminate = 0;

void
TerminateHandler(int /*signal*/)
{
	g_pokerthTerminate = 1;
}

// TODO: Hack
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#ifndef daemon
int daemon(int, int);
#endif
#endif

int
main(int argc, char *argv[])
{
	ENABLE_LEAK_CHECK();

	//_CrtSetBreakAlloc(10260);

	bool readonlyConfig = false;
	string pidFile;
	int logLevel = 1;
	{
		// Check command line options.
		po::options_description desc("Allowed options");
		desc.add_options()
		("help,h", "produce help message")
		("version,v", "print version string")
		("log-level,l", po::value<int>(), "set log level (0=minimal, 1=default, 2=verbose)")
		("pid-file,p", po::value<string>(), "create pid-file in different location")
		("readonly-config", "treat config file as read-only")
		;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << desc << endl;
			return 1;
		}
		if (vm.count("version")) {
			cout << "PokerTH server version   " << POKERTH_BETA_RELEASE_STRING << endl
				 << "Network protocol version " << NET_VERSION_MAJOR << "." << NET_VERSION_MINOR << endl;
			return 1;
		}
		if (vm.count("log-level")) {
			logLevel = vm["log-level"].as<int>();
			if (logLevel < 0 || logLevel > 2) {
				cout << "Invalid log-level: \"" << logLevel << "\", allowed range 0-2." << endl;
				return 1;
			}
		}
		if (vm.count("pid-file"))
			pidFile = vm["pid-file"].as<string>();
		if (vm.count("readonly-config"))
			readonlyConfig = true;
	}

	boost::shared_ptr<QtToolsInterface> myQtToolsInterface(CreateQtToolsWrapper());
	//create defaultconfig
	boost::shared_ptr<ConfigFile> myConfig(new ConfigFile(argv[0], readonlyConfig));
	loghelper_init(myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("LogDir")), logLevel);

	// TODO: Hack
#ifndef _WIN32
#ifdef QT_NO_DEBUG
	if (daemon(0, 0) != 0) {
		cout << "Failed to start daemon." << endl;
		return 1;
	}
#endif
#endif

	signal(SIGTERM, TerminateHandler);
	signal(SIGINT, TerminateHandler);

	socket_startup();

	LOG_MSG("Starting PokerTH dedicated server. Availability: IPv6 "
			<< socket_has_ipv6() << ", SCTP " << socket_has_sctp() << ", Dual Stack " << socket_has_dual_stack() << ".");

	// Store pid in file.
	if (pidFile.empty()) {
		path tmpPidPath(myConfig->readConfigString("LogDir"));
		tmpPidPath /= "pokerth.pid";
		pidFile = tmpPidPath.directory_string();
	}
	{
		std::ofstream pidStream(pidFile.c_str(), ios_base::out | ios_base::trunc);
		if (!pidStream.fail())
			pidStream << getpid();
		else
			LOG_ERROR("Could not create process id file \"" << pidFile << "\"!");
	}

	// Create pseudo Gui Wrapper for the server.
	boost::shared_ptr<GuiInterface> myServerGuiInterface(new ServerGuiWrapper(myConfig.get(), NULL, NULL, NULL));
	boost::shared_ptr<Session> session(new Session(myServerGuiInterface.get(), myConfig.get(), NULL));
	if (!session->init())
		LOG_ERROR("Missing files - please check your directory settings!");
	myServerGuiInterface->setSession(session);

	myServerGuiInterface->getSession()->startNetworkServer(true);
	while (!g_pokerthTerminate) {
		Thread::Msleep(100);
		if (myServerGuiInterface->getSession()->pollNetworkServerTerminated())
			g_pokerthTerminate = true;
	}
	myServerGuiInterface->getSession()->terminateNetworkServer();
	session.reset();
	myServerGuiInterface.reset();
	myConfig.reset();

	LOG_MSG("Terminating PokerTH dedicated server." << endl);
	socket_cleanup();
	return 0;
}

