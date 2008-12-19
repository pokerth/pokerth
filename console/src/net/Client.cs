using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace pokerth_console
{
	class Client
	{
		public Client(Settings settings)
		{
			m_tcpClient = new TcpClient();
			m_settings = settings;
		}

		public void Connect()
		{
			// Connect to the server.
			// Try IPv6 first.
			IPAddress[] addresses = new IPAddress[2];
			addresses[0] = IPAddress.Parse(m_settings.ServerSettings.IPv6Address);
			addresses[1] = IPAddress.Parse(m_settings.ServerSettings.IPv4Address);
			m_tcpClient.Connect(addresses, m_settings.ServerSettings.Port);
		}

		public void Start(GameInfoList list)
		{
			StartReceiveThread(list);
			SendInit();
		}

		public void SetTerminateFlag()
		{
			m_receiver.SetTerminateFlag();
		}

		public void WaitTermination()
		{
			m_receiver.WaitTermination();
		}

		protected void StartReceiveThread(GameInfoList list)
		{
			m_receiver = new ReceiverThread(m_tcpClient.GetStream(), list);
			m_receiver.Run();
		}

		protected void SendInit()
		{
			NetPacket test = new NetPacketInit();
			test.Properties.Add(NetPacket.PropertyType.PropRequestedVersionMajor, "4");
			test.Properties.Add(NetPacket.PropertyType.PropRequestedVersionMinor, "2");
			test.Properties.Add(NetPacket.PropertyType.PropPlayerName, "Loto");
			test.Properties.Add(NetPacket.PropertyType.PropPlayerPassword, "");
			byte[] buf = test.ToByteArray();
			m_tcpClient.GetStream().Write(buf, 0, buf.Length);
		}

		private TcpClient m_tcpClient;
		private ReceiverThread m_receiver;
		private Settings m_settings;
	}
}
