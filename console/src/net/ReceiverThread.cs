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
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.IO;

namespace pokerth_console
{
	class ReceiverThread
	{
		const uint MaxPacketSize = 268;
		const uint MinPacketSize = 8;

		public ReceiverThread(NetworkStream stream, GameInfoList list)
		{
			m_recThread = new Thread(ThreadProc);
			m_terminateFlag = false;
			m_terminateFlagMutex = new System.Object();
			m_recStream = stream;
			m_recStream.ReadTimeout = 50;
			m_recBuf = new byte[8192];
			m_recBufOffset = 0;
			m_packetList = new List<NetPacket>();
			m_lobbyGameInfoList = list;
		}

		public static void Run(ReceiverThread me)
		{
			me.m_recThread.Start(me);
		}

		protected static void ThreadProc(object obj)
		{
			ReceiverThread me = (ReceiverThread)obj;
			me.Start();
		}

		protected void Start()
		{
			while (!IsTerminateFlagSet())
			{
				ReadFromStream();
				ScanPackets();
				ParsePackets();
			}
		}

		public void WaitTermination()
		{
			m_recThread.Join();
		}

		public void SetTerminateFlag()
		{
			lock (m_terminateFlagMutex)
			{
				m_terminateFlag = true;
			}
		}

		protected bool IsTerminateFlagSet()
		{
			lock (m_terminateFlagMutex)
			{
				return m_terminateFlag;
			}
		}

		protected void ReadFromStream()
		{
			if (m_recStream.DataAvailable)
				m_recBufOffset += m_recStream.Read(m_recBuf, m_recBufOffset, m_recBuf.Length - m_recBufOffset);
			Thread.Sleep(15);
		}

		protected void ScanPackets()
		{
			bool packetFound;

			do
			{
				packetFound = false;
				if (m_recBufOffset >= MinPacketSize)
				{
					// Treat input buffer as memory stream.
					MemoryStream memStream = new MemoryStream(m_recBuf);
					BinaryReader r = new BinaryReader(memStream);
					int type = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
					int size = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
					if (m_recBufOffset >= size)
					{
						packetFound = true;
						if (size > MaxPacketSize)
						{
							// Ignore packets which are too long.
							m_recBufOffset -= size;
						}
						else
						{
							// Scan Packet.
							NetPacket packet = NetPacket.Create(type, size, r);
							if (packet != null)
								m_packetList.Add(packet);
							// Advance within buf.
							if (m_recBufOffset > size)
							{
								for (int i = size, j = 0; i < m_recBufOffset; i++, j++)
								{
									m_recBuf[j] = m_recBuf[i];
								}
								m_recBufOffset -= size;
							}
							else
								m_recBufOffset = 0;
						}
					}
				}
			}
			while (packetFound);
		}

		protected void ParsePackets()
		{
			foreach (NetPacket p in m_packetList)
			{
				if (p.Type == NetPacket.NetTypeGameListNew)
				{
					m_lobbyGameInfoList.AddGameInfo(new GameInfo(
						p.Properties[NetPacket.PropertyType.PropGameName]));
				}
			}
			m_packetList.Clear();
		}

		private Thread			m_recThread;
		private bool			m_terminateFlag;
		private Object			m_terminateFlagMutex;
		private NetworkStream	m_recStream;
		private byte[]			m_recBuf;
		private int				m_recBufOffset;
		private List<NetPacket> m_packetList;
		private GameInfoList	m_lobbyGameInfoList;
	}
}
