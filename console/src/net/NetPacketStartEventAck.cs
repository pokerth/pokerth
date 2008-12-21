using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.IO;

/*
struct GCC_PACKED NetPacketStartEventAckData
{
	NetPacketHeader		head;
	u_int32_t			reserved;
};
*/

namespace pokerth_console
{
	class NetPacketStartEventAck : NetPacket
	{
		public NetPacketStartEventAck()
			: base(NetPacket.NetTypeStartEventAck)
		{
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitStartEventAck(this);
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
