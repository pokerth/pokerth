using System;
using System.Collections.Generic;
using System.Text;

namespace pokerth_console
{
	class ServerSettings
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

		private string m_ipv4Address = "";
		private string m_ipv6Address = "";
		private int m_port = 0;
	}
}
