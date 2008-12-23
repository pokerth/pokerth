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

namespace pokerth_lib
{
	abstract class NetThread
	{
		public NetThread(NetworkStream stream)
		{
			m_thread = new Thread(ThreadProc);
			m_terminateFlag = false;
			m_terminateFlagMutex = new System.Object();
			m_netStream = stream;
		}

		public void Run()
		{
			m_thread.Start(this);
		}

		protected NetworkStream NetStream
		{
			get
			{
				return m_netStream;
			}
		}

		protected static void ThreadProc(object obj)
		{
			NetThread me = (NetThread)obj;
			me.Start();
		}

		protected abstract void Start();

		public void WaitTermination()
		{
			m_thread.Join();
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

		private Thread m_thread;
		private bool m_terminateFlag;
		private Object m_terminateFlagMutex;
		private NetworkStream m_netStream;
	}
}
