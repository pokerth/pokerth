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
#ifdef _WIN32
#error This source code is not for Win32.
#endif

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>


#ifndef daemon

int
daemon(int nochdir, int noclose)
{
	int maxfd, fd = 0;

	if (getppid() == 1)
		return 0;		/* run_as_daemon has already been called. */

	switch (fork())
	{
		case 0: /* The child process. */
			break;
		case -1: /* Fork failed. */
			return -1;
		default: /* Exit the parent. */
			_exit(0);
	}
	/* Now running the child (daemon). */

	if (setsid() < 0) /* Obtain a new process group. */
		return -1;

	switch (fork())
	{
		case 0:
			break;
		case -1:
			return -1;
		default:
			_exit(0);
	}


	if (!nochdir)
		chdir("/"); /* Change working directory. */

	if (!noclose)
	{
		/* Close all open handles. */
		maxfd = sysconf(_SC_OPEN_MAX);
		while (fd < maxfd)
			close(fd++);

		/* Use /dev/null as stdin/stdout/stderr. */
		open("/dev/null", O_RDWR);
		dup(0);
		dup(0);
	}
	
	return 0;
}

#endif
