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
using System.Net;
using System.Net.Sockets;
using System.IO;

namespace pokerth_lib
{
	public abstract class NetPacket
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

		public enum PropType
		{
			RequestedVersionMajor,
			RequestedVersionMinor,
			PlayerId,
			PlayerName,
			ServerPassword,
			PlayerFlags,
			PlayerRights,
			PlayerAction,
			PlayerBet,
			PlayerBetTotal,
			PlayerMoney,
			LatestGameVersion,
			LatestBetaRevision,
			SessionId,
			GameId,
			GameMode,
			GameName,
			GamePrivacyFlags,
			GamePassword,
			GameState,
			AdminPlayerId,
			MaxNumPlayers,
			RaiseIntervalMode,
			RaiseSmallBlindInterval,
			RaiseMode,
			EndRaiseMode,
			ProposedGuiSpeed,
			PlayerActionTimeout,
			FirstSmallBlind,
			EndRaiseSmallBlindValue,
			StartMoney,
			CurNumPlayers,
			StartFlags,
			StartDealerPlayerId,
			FirstCard,
			SecondCard,
			SmallBlind,
			HighestSet,
			MinimumRaise,
			ActionRejectReason,
			FlopFirstCard,
			FlopSecondCard,
			FlopThirdCard,
			TurnCard,
			RiverCard,
			MoneyWon,
			BestHandPos1,
			BestHandPos2,
			BestHandPos3,
			BestHandPos4,
			BestHandPos5,
			CardsValue,
			ChatText,
			ErrorReason,
			RemoveReason,
		}

		public enum ListPropType
		{
			PlayerSlots,
			ManualBlindSlots,
		}

		public enum RecordPropType
		{
			PlayerResults,
			PlayerCards,
		}

		public static NetPacket Create(int type)
		{
			NetPacket tmpPacket = null;
			switch (type)
			{
				// Virtual constructor.
				case NetTypeInit :
					tmpPacket = new NetPacketInit();
					break;
				case NetTypeInitAck :
					tmpPacket = new NetPacketInitAck();
					break;
				//case NetTypeRetrieveAvatar :
				//case NetTypeAvatarHeader :
				//case NetTypeAvatarFile :
				//case NetTypeAvatarEnd :
				//case NetTypeUnknownAvatar :
				case NetTypeGameListNew :
					tmpPacket = new NetPacketGameListNew();
					break;
				case NetTypeGameListUpdate :
					tmpPacket = new NetPacketGameListUpdate();
					break;
				case NetTypeGameListPlayerJoined :
					tmpPacket = new NetPacketGameListPlayerJoined();
					break;
				case NetTypeGameListPlayerLeft :
					tmpPacket = new NetPacketGameListPlayerLeft();
					break;
				//case NetTypeGameListAdminChanged :
				//	tmpPacket = new NetPacketGameListAdminChanged();
				//	break;
				case NetTypeRetrievePlayerInfo :
					tmpPacket = new NetPacketRetrievePlayerInfo();
					break;
				case NetTypePlayerInfo :
					tmpPacket = new NetPacketPlayerInfo();
					break;
				//case NetTypeUnknownPlayerId :
				case NetTypeUnsubscribeGameList :
					tmpPacket = new NetPacketUnsubscribeGameList();
					break;
				//case NetTypeResubscribeGameList :
				case NetTypeCreateGame :
					tmpPacket = new NetPacketCreateGame();
					break;
				case NetTypeJoinGame :
					tmpPacket = new NetPacketJoinGame();
					break;
				case NetTypeJoinGameAck :
					tmpPacket = new NetPacketJoinGameAck();
					break;
				//case NetTypeJoinGameFailed :
				//	tmpPacket = new NetPacketJoinGameFailed();
				//	break;
				//case NetTypePlayerJoined :
				//case NetTypePlayerLeft :
				//case NetTypeGameAdminChanged :
				//case NetTypeKickPlayer :
				case NetTypeLeaveCurrentGame :
					tmpPacket = new NetPacketLeaveCurrentGame();
					break;
				case NetTypeStartEvent :
					tmpPacket = new NetPacketStartEvent();
					break;
				case NetTypeStartEventAck :
					tmpPacket = new NetPacketStartEventAck();
					break;
				case NetTypeGameStart :
					tmpPacket = new NetPacketGameStart();
					break;
				case NetTypeHandStart :
					tmpPacket = new NetPacketHandStart();
					break;
				case NetTypePlayersTurn :
					tmpPacket = new NetPacketPlayersTurn();
					break;
				case NetTypePlayersAction :
					tmpPacket = new NetPacketPlayersAction();
					break;
				case NetTypePlayersActionDone :
					tmpPacket = new NetPacketPlayersActionDone();
					break;
				case NetTypePlayersActionRejected :
					tmpPacket = new NetPacketPlayersActionRejected();
					break;
				case NetTypeDealFlopCards :
					tmpPacket = new NetPacketDealFlopCards();
					break;
				case NetTypeDealTurnCard :
					tmpPacket = new NetPacketDealTurnCard();
					break;
				case NetTypeDealRiverCard :
					tmpPacket = new NetPacketDealRiverCard();
					break;
				case NetTypeAllInShowCards :
					tmpPacket = new NetPacketAllInShowCards();
					break;
				case NetTypeEndOfHandShowCards :
					tmpPacket = new NetPacketEndOfHandShowCards();
					break;
				case NetTypeEndOfHandHideCards :
					tmpPacket = new NetPacketEndOfHandHideCards();
					break;
				case NetTypeEndOfGame :
					tmpPacket = new NetPacketEndOfGame();
					break;
				//case NetTypeAskKickPlayer :
				//case NetTypeAskKickPlayerDenied :
				//case NetTypeStartKickPlayerPetition :
				//case NetTypeVoteKickPlayer :
				//case NetTypeVoteKickPlayerAck :
				//case NetTypeVoteKickPlayerDenied :
				//case NetTypeKickPlayerPetitionUpdate :
				//case NetTypeEndKickPlayerPetition :
				//case NetTypeStatisticsChanged :
				case NetTypeRemovedFromGame :
					tmpPacket = new NetPacketRemovedFromGame();
					break;
				//case NetTypeTimeoutWarning :
				//case NetTypeResetTimeout :
				//case NetTypeSendChatText :
				case NetTypeChatText :
					tmpPacket = new NetPacketChatText();
					break;
				case NetTypeError :
					tmpPacket = new NetPacketError();
					break;
			}
			return tmpPacket;
		}

		public static NetPacket Create(int type, int size, BinaryReader reader)
		{
			NetPacket tmpPacket = null;
			switch (type)
			{
				// Only consider those packets which are sent by the server.
				case NetTypeInitAck :
					tmpPacket = new NetPacketInitAck(size, reader);
					break;
				case NetTypeGameListNew :
					tmpPacket = new NetPacketGameListNew(size, reader);
					break;
				case NetTypeGameListUpdate :
					tmpPacket = new NetPacketGameListUpdate(size, reader);
					break;
				case NetTypeGameListPlayerJoined :
					tmpPacket = new NetPacketGameListPlayerJoined(size, reader);
					break;
				case NetTypeGameListPlayerLeft :
					tmpPacket = new NetPacketGameListPlayerLeft(size, reader);
					break;
				case NetTypeJoinGameAck :
					tmpPacket = new NetPacketJoinGameAck(size, reader);
					break;
				case NetTypePlayerInfo :
					tmpPacket = new NetPacketPlayerInfo(size, reader);
					break;
				case NetTypeStartEvent :
					tmpPacket = new NetPacketStartEvent(size, reader);
					break;
				case NetTypeGameStart :
					tmpPacket = new NetPacketGameStart(size, reader);
					break;
				case NetTypeHandStart :
					tmpPacket = new NetPacketHandStart(size, reader);
					break;
				case NetTypePlayersTurn :
					tmpPacket = new NetPacketPlayersTurn(size, reader);
					break;
				case NetTypePlayersActionDone :
					tmpPacket = new NetPacketPlayersActionDone(size, reader);
					break;
				case NetTypePlayersActionRejected :
					tmpPacket = new NetPacketPlayersActionRejected(size, reader);
					break;
				case NetTypeDealFlopCards :
					tmpPacket = new NetPacketDealFlopCards(size, reader);
					break;
				case NetTypeDealTurnCard :
					tmpPacket = new NetPacketDealTurnCard(size, reader);
					break;
				case NetTypeDealRiverCard :
					tmpPacket = new NetPacketDealRiverCard(size, reader);
					break;
				case NetTypeAllInShowCards :
					tmpPacket = new NetPacketAllInShowCards(size, reader);
					break;
				case NetTypeEndOfHandShowCards :
					tmpPacket = new NetPacketEndOfHandShowCards(size, reader);
					break;
				case NetTypeEndOfHandHideCards :
					tmpPacket = new NetPacketEndOfHandHideCards(size, reader);
					break;
				case NetTypeEndOfGame :
					tmpPacket = new NetPacketEndOfGame(size, reader);
					break;
				case NetTypeRemovedFromGame :
					tmpPacket = new NetPacketRemovedFromGame(size, reader);
					break;
				case NetTypeChatText :
					tmpPacket = new NetPacketChatText(size, reader);
					break;
				case NetTypeError :
					tmpPacket = new NetPacketError(size, reader);
					break;
				default:
					break;
			}
			return tmpPacket;
		}

		public NetPacket(int type)
		{
			m_type = type;
			m_properties = new Dictionary<PropType, string>();
			m_listProperties = new Dictionary<ListPropType, List<string>>();
			m_recordProperties = new Dictionary<RecordPropType, List<Dictionary<PropType, string>>>();
		}

		public Dictionary<PropType, string> Properties
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

		public Dictionary<ListPropType, List<string>> ListProperties
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

		public Dictionary<RecordPropType, List<Dictionary<PropType, string>>> RecordProperties
		{
			set
			{
				m_recordProperties = value;
			}
			get
			{
				return m_recordProperties;
			}
		}

		public int Type
		{
			get
			{
				return m_type;
			}
		}

		public abstract void Accept(INetPacketVisitor visitor);

		public abstract byte[] ToByteArray();

		static protected int AddPadding(int size)
		{
			return ((((size) + 3) / 4) * 4);
		}

		protected void ScanGameInfoBlock(BinaryReader r)
		{
			Properties.Add(PropType.MaxNumPlayers,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropType.RaiseIntervalMode,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropType.RaiseSmallBlindInterval,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropType.RaiseMode,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropType.EndRaiseMode,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			int numManualBlinds = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
			Properties.Add(PropType.ProposedGuiSpeed,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropType.PlayerActionTimeout,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropType.FirstSmallBlind,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			Properties.Add(PropType.EndRaiseSmallBlindValue,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			Properties.Add(PropType.StartMoney,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));

			// Scan manual blinds.
			List<string> blindSlots = new List<string>();
			for (int i = 0; i < numManualBlinds; i++)
				blindSlots.Add(Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			ListProperties.Add(ListPropType.ManualBlindSlots, blindSlots);
		}

		protected void WriteGameInfoBlock(BinaryWriter w)
		{
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.MaxNumPlayers])));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.RaiseIntervalMode])));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.RaiseSmallBlindInterval])));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.RaiseMode])));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.EndRaiseMode])));
			w.Write(IPAddress.HostToNetworkOrder((short)
				ListProperties[ListPropType.ManualBlindSlots].Count));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.ProposedGuiSpeed])));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.PlayerActionTimeout])));
			w.Write(IPAddress.HostToNetworkOrder((int)
				Convert.ToUInt32(Properties[PropType.FirstSmallBlind])));
			w.Write(IPAddress.HostToNetworkOrder((int)
				Convert.ToUInt32(Properties[PropType.EndRaiseSmallBlindValue])));
			w.Write(IPAddress.HostToNetworkOrder((int)
				Convert.ToUInt32(Properties[PropType.StartMoney])));

			foreach (string s in ListProperties[ListPropType.ManualBlindSlots])
			{
				w.Write(IPAddress.HostToNetworkOrder((int)
					Convert.ToUInt32(s)));
			}
		}

		private int m_type;
		private Dictionary<PropType, string> m_properties;
		private Dictionary<ListPropType, List<string>> m_listProperties;
		private Dictionary<RecordPropType, List<Dictionary<PropType, string>>> m_recordProperties;
	}
}
