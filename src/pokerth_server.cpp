/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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

#include <iostream>

#include "session.h"
#include "configfile.h"
#include <qttoolsinterface.h>
#include <gui/generic/serverguiwrapper.h>
#include <net/socket_startup.h>
#include <net/netpacket.h>
#include <core/loghelper.h>
#include <core/thread.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <memory>
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

//	_CrtSetBreakAlloc(4772);

	bool readonlyConfig = false;
	string pidFile;
	{
		// Check command line options.
		po::options_description desc("Allowed options");
		desc.add_options()
			("help,h", "produce help message")
			("version,v", "print version string")
			("pid-file,p", po::value<string>(), "create pid-file in different location")
			("readonly-config", "treat config file as read-only")
			;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			cout << desc << endl;
			return 1;
		}
		if (vm.count("version"))
		{
			cout << "PokerTH server version   " << POKERTH_BETA_RELEASE_STRING << endl
				 << "Network protocol version " << NET_VERSION_MAJOR << "." << NET_VERSION_MINOR << endl;
			return 1;
		}
		if (vm.count("pid-file"))
			pidFile = vm["pid-file"].as<string>();
		if (vm.count("readonly-config"))
			readonlyConfig = true;
	}

	auto_ptr<QtToolsInterface> myQtToolsInterface(CreateQtToolsWrapper());
	//create defaultconfig
	ConfigFile *myConfig = new ConfigFile(argv[0], readonlyConfig);
	loghelper_init(myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("LogDir")));

	// TODO: Hack
#ifndef _WIN32
	daemon(0, 0);
#endif

	signal(SIGTERM, TerminateHandler);
	signal(SIGINT, TerminateHandler);

	socket_startup();

	LOG_MSG("Starting PokerTH dedicated server. Availability: IPv6 "
		<< socket_has_ipv6() << ", SCTP " << socket_has_sctp() << ", Dual Stack " << socket_has_dual_stack() << ".");

	// Store pid in file.
	if (pidFile.empty())
	{
		path tmpPidPath(myConfig->readConfigString("LogDir"));
		tmpPidPath /= "pokerth.pid";
		pidFile = tmpPidPath.directory_string();
	}
	{
		ofstream pidStream(pidFile.c_str(), ios_base::out | ios_base::trunc);
		if (!pidStream.fail())
			pidStream << getpid();
		else
			LOG_ERROR("Could not create process id file \"" << pidFile << "\"!");
	}

	// Create pseudo Gui Wrapper for the server.
	boost::shared_ptr<GuiInterface> myServerGuiInterface(new ServerGuiWrapper(myConfig, NULL, NULL, NULL));
	boost::shared_ptr<Session> session(new Session(myServerGuiInterface.get(), myConfig));
	if (!session->init())
		LOG_ERROR("Missing files - please check your directory settings!");
	myServerGuiInterface->setSession(session);

	myServerGuiInterface->getSession().startNetworkServer();
	while (!g_pokerthTerminate)
	{
		Thread::Msleep(100);
		if (myServerGuiInterface->getSession().pollNetworkServerTerminated())
			g_pokerthTerminate = true;
	}
	myServerGuiInterface->getSession().terminateNetworkServer();

	LOG_MSG("Terminating PokerTH dedicated server." << endl);
	socket_cleanup();
	return 0;
}

