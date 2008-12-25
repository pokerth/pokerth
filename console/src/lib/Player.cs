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
	public class Player
	{
		public Player()
		{
			m_mutex = new Object();
			m_curAction = Hand.Action.None;
		}

		public int[] Cards
		{
			get
			{
				lock (m_mutex)
				{
					// Return a copy of the array.
					return (int[])m_cards.Clone();
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_cards = value;
				}
			}
		}

		public int[] BestHandPos
		{
			get
			{
				lock (m_mutex)
				{
					// Return a copy of the array.
					return (int[])m_bestHandPos.Clone();
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_bestHandPos = value;
				}
			}
		}

		public Hand.Action CurAction
		{
			get
			{
				lock (m_mutex)
				{
					return m_curAction;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_curAction = value;
				}
			}
		}

		public uint Money
		{
			get
			{
				lock (m_mutex)
				{
					return m_money;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_money = value;
				}
			}
		}

		public uint TotalBet
		{
			get
			{
				lock (m_mutex)
				{
					return m_totalBet;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_totalBet = value;
				}
			}
		}

		public int CardsValue
		{
			get
			{
				lock (m_mutex)
				{
					return m_cardsValue;
				}
			}
			set
			{
				lock (m_mutex)
				{
					m_cardsValue = value;
				}
			}
		}

		public void NewHand()
		{
			lock (m_mutex)
			{
				m_cards = null;
				m_curAction = Hand.Action.None;
				m_totalBet = 0;
			}
		}

		private Object m_mutex;
		private int[] m_cards;
		private int[] m_bestHandPos;
		private Hand.Action m_curAction;
		private uint m_money;
		private uint m_totalBet;
		private int m_cardsValue;
	}
}
