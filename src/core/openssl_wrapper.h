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
/* Wrapper for GnuTLS openssl include. Needed for Windows/MacOS compatibility. */

#ifndef _OPENSSL_WRAPPER_H_
#define _OPENSSL_WRAPPER_H_

#ifndef HAVE_SSIZE_T
#define HAVE_SSIZE_T
#include <sys/types.h>
#ifdef _WIN64 // This is only for Windows.
#ifndef _SSIZE_T_
typedef long long ssize_t;
#define _SSIZE_T_
#endif
#ifndef _PID_T_
typedef __int64 pid_t;
#define _PID_T_
#endif
#elif _WIN32
#ifndef _SSIZE_T_
typedef int ssize_t;
#define _SSIZE_T_
#endif
#ifndef _PID_T_
typedef int pid_t;
#define _PID_T_
#endif
#endif
#endif

#ifdef X509_NAME
#undef X509_NAME // Again for Windows - conflict with WinCrypt.h.
#endif

// For BSD-Systems, we assume that OpenSSL is part of the operating system.
//
// make sure you understood the following issue before defining this directive.
// "2. Can I use OpenSSL with GPL software?"
// http://www.openssl.org/support/faq.html#LEGAL2
//
#if defined(__APPLE__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
#define HAVE_OPENSSL
#endif

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#else
// For all other systems, we use gcrypt.
#include <gcrypt.h>
#endif

#endif
