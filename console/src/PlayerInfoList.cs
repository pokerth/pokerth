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
using System.IO;

namespace pokerth_console
{
	class PlayerInfoList
	{
		public PlayerInfoList()
		{
			m_list = new Dictionary<uint, PlayerInfo>();
		}

		public void AddPlayerInfo(PlayerInfo info)
		{
			lock (m_list)
			{
				if (m_list.ContainsKey(info.Id))
					m_list[info.Id] = info;
				else
					m_list.Add(info.Id, info);
			}
		}

		public PlayerInfo GetPlayerInfo(uint id)
		{
			lock (m_list)
			{
				return m_list[id];
			}
		}

		public bool HasPlayer(uint id)
		{
			lock (m_list)
			{
				return m_list.ContainsKey(id);
			}
		}

		public override string ToString()
		{
			string outString = "";
			lock (m_list)
			{
				foreach (KeyValuePair<uint, PlayerInfo> i in m_list)
				{
					outString += i.Value.Name;
					outString += '\n';
				}
			}
			return outString;
		}

		private Dictionary<uint, PlayerInfo> m_list;
	}
}
