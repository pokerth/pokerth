/***************************************************************************
 *   Copyright (C) 2009 by Lothar May                                      *
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
/* PokerTH encoded packet. */

#ifndef _ENCODEDPACKET_H_
#define _ENCODEDPACKET_H_


class EncodedPacket
{
public:
	EncodedPacket(const unsigned char *data, unsigned size);
	~EncodedPacket();

	unsigned GetSize() const {
		return m_size;
	}

	const unsigned char *GetData() const {
		return m_data;
	}

private:
	unsigned m_size;
	unsigned char *m_data;
};

#endif

