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
using System.Xml;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace pokerth_console
{
	class Program
	{
		static int Main(string[] args)
		{
			// Retrieve server list.
			WebClient webcl = new WebClient();
			string tmpFilePath = Path.GetTempPath();
			string tmpZipFileName = tmpFilePath + "pokerth_serverlist.xml.z";
			string tmpXmlFileName = tmpFilePath + "pokerth_serverlist.xml";
			webcl.DownloadFile("http://pokerth.net/serverlist.xml.z", tmpZipFileName);
			ZlibHelper.UncompressFile(tmpZipFileName, tmpXmlFileName);
			FileStream f = new FileStream(tmpXmlFileName, FileMode.Open, FileAccess.Read);
			XmlReader x = XmlReader.Create(f);
			x.Read();
			x.ReadStartElement("ServerList");
			x.ReadStartElement("Server");
			x.ReadToFollowing("IPv4Address");
			x.MoveToFirstAttribute();
			string ipAddress = x.Value;
			x.ReadToFollowing("Port");
			x.MoveToFirstAttribute();
			int port = Convert.ToInt32(x.Value);
			x.Close();
			f.Close();
			File.Delete(tmpZipFileName);
			File.Delete(tmpXmlFileName);
			// Connect to the server.
			TcpClient client = new TcpClient(ipAddress, port);
			NetPacket test = new NetPacketInit();
			test.Properties.Add(NetPacket.PropertyType.PropRequestedVersionMajor, "4");
			test.Properties.Add(NetPacket.PropertyType.PropRequestedVersionMinor, "2");
			test.Properties.Add(NetPacket.PropertyType.PropPlayerName, "Loto");
			test.Properties.Add(NetPacket.PropertyType.PropPlayerPassword, "");
			byte[] buf = test.ToByteArray();
			client.GetStream().Write(buf, 0, buf.Length);
			GameInfoList list = new GameInfoList();
			ReceiverThread t = new ReceiverThread(client.GetStream(), list);
			ReceiverThread.Run(t);
			Thread.Sleep(5000);
			t.SetTerminateFlag();
			Console.Write(list.ToString());
			t.WaitTermination();
			return 0;
		}
	}
}
