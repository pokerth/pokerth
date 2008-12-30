using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.IO;


namespace pokerth_lib
{
	class NetPacketUnsubscribeGameList : NetPacket
	{
		public NetPacketUnsubscribeGameList()
			: base(NetPacket.NetTypeUnsubscribeGameList)
		{
		}

		public NetPacketUnsubscribeGameList(int size, BinaryReader r)
			: base(NetPacket.NetTypeUnsubscribeGameList)
		{
			if (size != 8)
				throw new NetPacketException("NetPacketUnsubscribeGameList invalid size.");
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitJoinGameAck(this);
		}

		public override byte[] ToByteArray()
		{
			MemoryStream memStream = new MemoryStream();
			BinaryWriter w = new BinaryWriter(memStream);

			w.Write(IPAddress.HostToNetworkOrder((short)Type));
			w.Write(IPAddress.HostToNetworkOrder((short)8));
			w.Write(IPAddress.HostToNetworkOrder((int)0)); // reserved

			return memStream.ToArray();
		}
	}
}
