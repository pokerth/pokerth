using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.IO;

namespace pokerth_lib
{
	public abstract class TestThread : BasicThread
	{
		public TestThread(string serverAddress, uint id)
		{
			m_id = id;
			m_serverAddress = serverAddress;
			m_client = new TcpClient();
		}

		public void Connect()
		{
            bool isConnected = false;
            do
            {
                try
                {
                    m_client.Connect(m_serverAddress, 7234);
                    isConnected = true;
                }
                catch (SocketException)
                {
                    Thread.Sleep(100);
                }
            } while (!isConnected);
			m_client.NoDelay = true;
		}

		public void SendInit(string userName)
		{
			// Send init.
			NetPacket init = NetPacket.Create(NetPacket.NetTypeInit);
			init.Properties.Add(NetPacket.PropType.RequestedVersionMajor, "5");
			init.Properties.Add(NetPacket.PropType.RequestedVersionMinor, "2");
			init.Properties.Add(NetPacket.PropType.PlayerName, userName);
			init.Properties.Add(NetPacket.PropType.ServerPassword, "");
			byte[] outBuf = init.ToByteArray();
			m_client.GetStream().Write(outBuf, 0, outBuf.Length);
		}

		public static void ReadFromStream(NetworkStream stream, byte[] buf, int bufOffset, int numBytes)
		{
			int bytesRead = 0;
			do
			{
				while (!stream.DataAvailable)
					Thread.Sleep(20);
				bytesRead += stream.Read(buf, bufOffset + bytesRead, numBytes - bytesRead);
			} while (bytesRead != numBytes);
		}

		public int ReadNextPacket(out byte[] packet)
		{
			byte[] header = new byte[4];
			NetworkStream stream = m_client.GetStream();
			ReadFromStream(stream, header, 0, 4);
			MemoryStream memStream = new MemoryStream(header);
			BinaryReader r = new BinaryReader(memStream);
			int type = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
			int size = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
			if (size < 4)
			{
				throw new InvalidDataException("Invalid packet size.");
			}
			packet = new byte[size];
			header.CopyTo(packet, 0);
			ReadFromStream(stream, packet, 4, size - 4);
			return type;
		}

		public void WaitForPacket(int waitType)
		{
			int type;
			byte[] packet = null;
			do
			{
				type = ReadNextPacket(out packet);
			} while (type != waitType);
		}

		protected uint m_id;
		protected TcpClient m_client;
		protected string m_serverAddress;
	}
}
