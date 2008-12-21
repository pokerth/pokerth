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
	class SenderThread : NetThread
	{
		public SenderThread(NetworkStream stream)
			: base(stream)
		{
			m_packetQueue = new Queue<NetPacket>();
		}

		public void Send(NetPacket p)
		{
			lock (m_packetQueue)
			{
				m_packetQueue.Enqueue(p);
			}
		}

		protected override void Start()
		{
			while (!IsTerminateFlagSet())
			{
				bool sleep = false;
				lock (m_packetQueue)
				{
					if (m_packetQueue.Count > 0)
					{
						byte[] outBuf = m_packetQueue.Dequeue().ToByteArray();
						NetStream.Write(outBuf, 0, outBuf.Length);
					}
					else
						sleep = true;
				}
				if (sleep)
					Thread.Sleep(15);
			}
		}

		private Queue<NetPacket> m_packetQueue;
	}
}
