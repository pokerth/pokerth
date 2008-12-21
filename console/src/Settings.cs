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
using System.Xml;
using System.Net;
using System.Net.Sockets;
using System.IO;

namespace pokerth_console
{
	class Settings
	{
		private const string ServerListUrl = "http://pokerth.net/serverlist.xml.z";

		public Settings()
		{
			m_serverSettings = RetrieveServerSettings();
		}

		public ServerSettings ServerSettings
		{
			get
			{
				return m_serverSettings;
			}
		}

		protected ServerSettings RetrieveServerSettings()
		{
			// Retrieve server list.
			string tmpFile = DownloadXmlServerList();
			// Parse Xml data;
			ServerSettings settings = ParseServerSettings(tmpFile);
			File.Delete(tmpFile);
			return settings;
		}

		protected string DownloadXmlServerList()
		{
			// Download the server list from the official site.
			WebClient webcl = new WebClient();
			string tmpFilePath = Path.GetTempPath();
			string tmpZipFile = tmpFilePath + "pokerth_serverlist.xml.z";
			string tmpXmlFile = tmpFilePath + "pokerth_serverlist.xml";
			webcl.DownloadFile(ServerListUrl, tmpZipFile);
			// The list is zlib compressed - uncompress.
			ZlibHelper.UncompressFile(tmpZipFile, tmpXmlFile);
			File.Delete(tmpZipFile);

			return tmpXmlFile;
		}

		protected ServerSettings ParseServerSettings(string file)
		{
			ServerSettings settings = new ServerSettings();
			FileStream f = new FileStream(file, FileMode.Open, FileAccess.Read);
			XmlReader x = XmlReader.Create(f);
			x.Read();
			x.ReadStartElement("ServerList");
			x.ReadStartElement("Server");
			x.ReadToFollowing("IPv4Address");
			x.MoveToFirstAttribute();
			settings.IPv4Address = x.Value;
			x.ReadToFollowing("IPv6Address");
			x.MoveToFirstAttribute();
			settings.IPv6Address = x.Value;
			x.ReadToFollowing("Port");
			x.MoveToFirstAttribute();
			settings.Port = Convert.ToInt32(x.Value);
			x.Close();
			f.Close();

			return settings;
		}

		private ServerSettings m_serverSettings;
	}
}
