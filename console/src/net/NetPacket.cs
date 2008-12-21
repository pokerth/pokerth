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

namespace pokerth_console
{
	abstract class NetPacket
	{
		public const int NetTypeInit							= 0x0001;
		public const int NetTypeInitAck							= 0x0002;
		public const int NetTypeRetrieveAvatar					= 0x0003;
		public const int NetTypeAvatarHeader					= 0x0004;
		public const int NetTypeAvatarFile						= 0x0005;
		public const int NetTypeAvatarEnd						= 0x0006;
		public const int NetTypeUnknownAvatar					= 0x0007;
		public const int NetTypeGameListNew						= 0x0010;
		public const int NetTypeGameListUpdate					= 0x0011;
		public const int NetTypeGameListPlayerJoined			= 0x0012;
		public const int NetTypeGameListPlayerLeft				= 0x0013;
		public const int NetTypeGameListAdminChanged			= 0x0014;
		public const int NetTypeRetrievePlayerInfo				= 0x0020;
		public const int NetTypePlayerInfo						= 0x0021;
		public const int NetTypeUnknownPlayerId					= 0x0022;
		public const int NetTypeUnsubscribeGameList				= 0x0023;
		public const int NetTypeResubscribeGameList				= 0x0024;
		public const int NetTypeCreateGame						= 0x0030;
		public const int NetTypeJoinGame						= 0x0031;
		public const int NetTypeJoinGameAck						= 0x0032;
		public const int NetTypeJoinGameFailed					= 0x0033;
		public const int NetTypePlayerJoined					= 0x0034;
		public const int NetTypePlayerLeft						= 0x0035;
		public const int NetTypeGameAdminChanged				= 0x0036;
		public const int NetTypeKickPlayer						= 0x0040;
		public const int NetTypeLeaveCurrentGame				= 0x0041;
		public const int NetTypeStartEvent						= 0x0042;
		public const int NetTypeStartEventAck					= 0x0043;
		public const int NetTypeGameStart						= 0x0050;
		public const int NetTypeHandStart						= 0x0051;
		public const int NetTypePlayersTurn						= 0x0052;
		public const int NetTypePlayersAction					= 0x0053;
		public const int NetTypePlayersActionDone				= 0x0054;
		public const int NetTypePlayersActionRejected			= 0x0055;
		public const int NetTypeDealFlopCards					= 0x0060;
		public const int NetTypeDealTurnCard					= 0x0061;
		public const int NetTypeDealRiverCard					= 0x0062;
		public const int NetTypeAllInShowCards					= 0x0063;
		public const int NetTypeEndOfHandShowCards				= 0x0064;
		public const int NetTypeEndOfHandHideCards				= 0x0065;
		public const int NetTypeEndOfGame						= 0x0070;
		public const int NetTypeAskKickPlayer					= 0x0071;
		public const int NetTypeAskKickPlayerDenied				= 0x0072;
		public const int NetTypeStartKickPlayerPetition			= 0x0073;
		public const int NetTypeVoteKickPlayer					= 0x0074;
		public const int NetTypeVoteKickPlayerAck				= 0x0075;
		public const int NetTypeVoteKickPlayerDenied			= 0x0076;
		public const int NetTypeKickPlayerPetitionUpdate		= 0x0077;
		public const int NetTypeEndKickPlayerPetition			= 0x0078;

		public const int NetTypeStatisticsChanged				= 0x0080;

		public const int NetTypeRemovedFromGame					= 0x0100;
		public const int NetTypeTimeoutWarning					= 0x0101;
		public const int NetTypeResetTimeout					= 0x0102;

		public const int NetTypeSendChatText					= 0x0200;
		public const int NetTypeChatText						= 0x0201;

		public const int NetTypeError							= 0x0400;


		public enum PropertyType
		{
			PropRequestedVersionMajor,
			PropRequestedVersionMinor,
			PropPlayerId,
			PropPlayerName,
			PropPlayerPassword,
			PropPlayerFlags,
			PropLatestGameVersion,
			PropLatestBetaRevision,
			PropSessionId,
			PropGameId,
			PropGameMode,
			PropGameName,
			PropGamePrivacyFlags,
			PropGamePassword,
			PropAdminPlayerId,
			PropCurNumPlayers
		}

		public enum ListPropertyType
		{
			PropPlayerSlots
		}

		public static NetPacket Create(int type, int size, BinaryReader reader)
		{
			NetPacket tmpPacket = null;
			switch (type)
			{
				// Only consider those packets which are sent by the server.
				case NetTypeInitAck:
					tmpPacket = new NetPacketInitAck(size, reader);
					break;
				case NetTypeGameListNew:
					tmpPacket = new NetPacketGameListNew(size, reader);
					break;
				default:
					break;
			}
			return tmpPacket;
		}

		public NetPacket(int type)
		{
			m_type = type;
			m_properties = new Dictionary<PropertyType, string>();
			m_listProperties = new Dictionary<ListPropertyType, List<string>>();
		}

		public Dictionary<PropertyType, string> Properties
		{
			set
			{
				m_properties = value;
			}
			get
			{
				return m_properties;
			}
		}

		public Dictionary<ListPropertyType, List<string>> ListProperties
		{
			set
			{
				m_listProperties = value;
			}
			get
			{
				return m_listProperties;
			}
		}

		public int Type
		{
			get
			{
				return m_type;
			}
		}

		public abstract byte[] ToByteArray();

		protected int AddPadding(int size)
		{
			return ((((size) + 3) / 4) * 4);
		}

		private int m_type;
		private Dictionary<PropertyType, string> m_properties;
		private Dictionary<ListPropertyType, List<string>> m_listProperties;
	}
}
