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
/* Wrapper for GnuTLS openssl include. Needed for Windows compatibility. */

#ifndef _OPENSSL_WRAPPER_H_
#define _OPENSSL_WRAPPER_H_

#ifndef HAVE_SSIZE_T
# define HAVE_SSIZE_T
#include <sys/types.h>
#ifdef _WIN32
	#ifndef ssize_t
		typedef long ssize_t; // This is only for Windows. Supports only Win32.
	#endif
	#ifndef pid_t
		typedef unsigned pid_t;
	#endif
#endif
#endif

#ifdef X509_NAME
	#undef X509_NAME // Again for Windows - conflict with WinCrypt.h.
#endif

#include <gnutls/openssl.h>

#endif
