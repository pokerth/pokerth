using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

namespace pokerth_lib
{
	public abstract class BasicThread
	{
		public BasicThread()
		{
			m_thread = new Thread(ThreadProc);
			m_terminateFlag = false;
			m_terminateFlagMutex = new System.Object();
		}

		public void Run()
		{
			m_thread.Start(this);
		}

		protected static void ThreadProc(object obj)
		{
			BasicThread me = (BasicThread)obj;
			me.Main();
		}

		protected abstract void Main();

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
	}
}
