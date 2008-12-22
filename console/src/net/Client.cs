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
using System.Net;
using System.Net.Sockets;

namespace pokerth_console
{
	class Client
	{
		public Client(Settings settings, PokerTHData data)
		{
			m_tcpClient = new TcpClient();
			m_settings = settings;
			m_data = data;
		}

		public void Connect()
		{
			// Connect to the server.
			// Try IPv6 first.
			IPAddress[] addresses = new IPAddress[2];
			addresses[0] = IPAddress.Parse(m_settings.ServerSettings.IPv6Address);
			addresses[1] = IPAddress.Parse(m_settings.ServerSettings.IPv4Address);
			m_tcpClient.Connect(addresses, m_settings.ServerSettings.Port);
			//m_tcpClient.Connect("localhost", 7234);
		}

		public void Start()
		{
			StartSendThread();
			StartReceiveThread();
			SendInit();
		}

		public void JoinGame(uint gameId)
		{
			SendJoinGame(gameId);
		}

		public void SetTerminateFlag()
		{
			m_sender.SetTerminateFlag();
			m_receiver.SetTerminateFlag();
		}

		public void WaitTermination()
		{
			m_sender.WaitTermination();
			m_receiver.WaitTermination();
		}

		protected void StartReceiveThread()
		{
			m_receiver = new ReceiverThread(m_tcpClient.GetStream(), m_sender, m_data);
			m_receiver.Run();
		}

		protected void StartSendThread()
		{
			m_sender = new SenderThread(m_tcpClient.GetStream());
			m_sender.Run();
		}

		protected void SendInit()
		{
			NetPacket init = new NetPacketInit();
			init.Properties.Add(NetPacket.PropertyType.RequestedVersionMajor, "5");
			init.Properties.Add(NetPacket.PropertyType.RequestedVersionMinor, "0");
			init.Properties.Add(NetPacket.PropertyType.PlayerName, "Testuser1");
			init.Properties.Add(NetPacket.PropertyType.PlayerPassword, "");
			m_sender.Send(init);
		}

		protected void SendJoinGame(uint gameId)
		{
			NetPacket join = new NetPacketJoinGame();
			join.Properties.Add(NetPacket.PropertyType.GameId, Convert.ToString(gameId));
			join.Properties.Add(NetPacket.PropertyType.GamePassword, ""); // no password for now
			m_sender.Send(join);
		}

		private TcpClient m_tcpClient;
		private ReceiverThread m_receiver;
		private SenderThread m_sender;
		private Settings m_settings;
		private PokerTHData m_data;
	}
}
