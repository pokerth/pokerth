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
