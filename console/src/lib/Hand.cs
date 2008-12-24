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

namespace pokerth_lib
{
	public class Hand
	{
		public const int MaxPlayers = 7;

		public enum State
		{
			Preflop = 0,
			Flop,
			Turn,
			River
		}

		public enum Action
		{
			None = 0,
			Fold,
			Check,
			Call,
			Bet,
			Raise,
			AllIn
		}

		public Hand(Dictionary<uint, Player> players, uint myPlayerId, uint smallBlind)
		{
			m_mutex = new Object();
			m_state = State.Preflop;
			m_players = players;
			m_myPlayerId = myPlayerId;
			m_smallBlind = smallBlind;
		}

		public State CurState
		{
			get
			{
				lock (m_mutex)
				{
					return m_state;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_state = value;
				}
			}
		}

		public Dictionary<uint, Player> Players
		{
			get
			{
				lock (m_mutex)
				{
					// Should not be modified. This is actually a const return ;-).
					return m_players;
				}
			}
		}

		public uint MyPlayerId
		{
			get
			{
				// no need to lock a mutex, this is read only.
				return m_myPlayerId;
			}
		}

		public uint SmallBlind
		{
			get
			{
				// no need to lock a mutex, this is read only.
				return m_smallBlind;
			}
		}

		public uint HighestSet
		{
			get
			{
				lock (m_mutex)
				{
					return m_highestSet;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_highestSet = value;
				}
			}
		}

		public uint MinimumRaise
		{
			get
			{
				lock (m_mutex)
				{
					return m_minimumRaise;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_minimumRaise = value;
				}
			}
		}

		public int[] TableCards
		{
			get
			{
				lock (m_mutex)
				{
					return (int[])m_tableCards.Clone();
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_tableCards = value;
				}
			}
		}

		private Object m_mutex;
		private State m_state;
		private Dictionary<uint, Player> m_players;
		private int[] m_tableCards;
		private uint m_myPlayerId;
		private uint m_smallBlind;
		private uint m_highestSet;
		private uint m_minimumRaise;
	}
}
