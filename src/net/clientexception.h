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
/* Exception class for client errors. */

#ifndef _CLIENTEXCEPTION_H_
#define _CLIENTEXCEPTION_H_


class ClientException
{
public:

	ClientException(int errorId, int osErrorCode)
		: m_errorId(errorId), m_osErrorCode(osErrorCode) {}

	int GetErrorId() const {return m_errorId;}
	int GetOsErrorCode() const {return m_osErrorCode;}

private:
	int m_errorId;
	int m_osErrorCode;
};

#endif
