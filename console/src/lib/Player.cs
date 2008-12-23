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

		private Object m_mutex;
		private int[] m_cards;
		private Hand.Action m_curAction;
		private uint m_money;
		private uint m_totalBet;
	}
}
