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
	public class ServerSettings
	{
		public ServerSettings()
		{
		}

		public string IPv4Address
		{
			get
			{
				return m_ipv4Address;
			}
			set
			{
				m_ipv4Address = value;
			}
		}

		public string IPv6Address
		{
			get
			{
				return m_ipv6Address;
			}
			set
			{
				m_ipv6Address = value;
			}
		}

		public int Port
		{
			get
			{
				return m_port;
			}
			set
			{
				m_port = value;
			}
		}

		public string Server
		{
			get
			{
				return m_server;
			}
			set
			{
				m_server = value;
			}
		}

		private string m_ipv4Address = "";
		private string m_ipv6Address = "";
		private string m_server = "";
		private int m_port = 0;
	}
}
