/***************************************************************************
 *   Copyright (C) 2008 by Lothar May                                      *
 *                                                                         *
 *   This file is part of pokerth_console.                                 *
 *   pokerth_console is free software: you can redistribute it and/or      *
 *   modify it under the terms of the GNU Affero General Public License    *
 *   as published by the Free Software Foundation, either version 3 of     *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   pokerth_console is distributed in the hope that it will be useful,    *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the                                *
 *   GNU Affero General Public License along with pokerth_console.         *
 *   If not, see <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/

using System;
using System.Collections.Generic;
using System.Text;

namespace pokerth_console
{
	class PokerTHData
	{
		public PokerTHData(string name)
		{
			m_gameInfoList = new GameInfoList();
			m_playerInfoList = new PlayerInfoList();
			m_mutex = new Object();
			m_myPlayerId = 0;
			m_myGameId = 0;
			m_myName = name;
		}

		public GameInfoList GameList
		{
			get
			{
				return m_gameInfoList;
			}
		}

		public PlayerInfoList PlayerList
		{
			get
			{
				return m_playerInfoList;
			}
		}

		public Hand CurHand
		{
			get
			{
				lock (m_mutex)
				{
					return m_curHand;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_curHand = value;
				}
			}
		}

		public string MyName
		{
			get
			{
				return m_myName;
			}
		}

		public uint MyPlayerId
		{
			get
			{
				lock (m_mutex)
				{
					return m_myPlayerId;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_myPlayerId = value;
				}
			}
		}

		public uint MyGameId
		{
			get
			{
				lock (m_mutex)
				{
					return m_myGameId;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_myGameId = value;
				}
			}
		}

		private GameInfoList m_gameInfoList;
		private PlayerInfoList m_playerInfoList;
		private Hand m_curHand;
		private Object m_mutex;
		private uint m_myPlayerId;
		private uint m_myGameId;
		private string m_myName;
	}
}
