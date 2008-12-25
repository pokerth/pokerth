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

namespace pokerth_lib
{
	class NetParser : INetPacketVisitor
	{
		public NetParser(PokerTHData data, SenderThread sender, ICallback callback)
		{
			m_data = data;
			m_sender = sender;
			m_callback = callback;
		}

		public void VisitInit(NetPacket p)
		{
			throw new NotImplementedException();
		}

		public void VisitInitAck(NetPacket p)
		{
			m_data.MyPlayerId =
				Convert.ToUInt32(p.Properties[NetPacket.PropType.PlayerId]);
			// Add self to list.
			m_data.PlayerList.AddPlayerInfo(new PlayerInfo(
				m_data.MyPlayerId,
				m_data.MyName));
			m_callback.InitDone();
		}

		public void VisitGameListNew(NetPacket p)
		{
			// Add game to list.
			m_data.GameList.AddGameInfo(new GameInfo(
				Convert.ToUInt32(p.Properties[NetPacket.PropType.GameId]),
				p.Properties[NetPacket.PropType.GameName],
				(GameInfo.Mode)Convert.ToInt32(p.Properties[NetPacket.PropType.GameMode]),
				p.ListProperties[NetPacket.ListPropType.PlayerSlots].
					ConvertAll<uint>(Convert.ToUInt32),
				Convert.ToUInt32(p.Properties[NetPacket.PropType.StartMoney])));
		}

		public void VisitGameListUpdate(NetPacket p)
		{
			GameInfo.Mode mode =
				(GameInfo.Mode)Convert.ToInt32(p.Properties[NetPacket.PropType.GameMode]);
			uint id = Convert.ToUInt32(p.Properties[NetPacket.PropType.GameId]);
			if (mode == GameInfo.Mode.Closed) // Remove game if it is has been closed.
				m_data.GameList.RemoveGameInfo(id);
			else
				m_data.GameList.GetGameInfo(id).CurrentMode = mode;
		}

		public void VisitRetrievePlayerInfo(NetPacket p)
		{
			throw new NotImplementedException();
		}

		public void VisitPlayerInfo(NetPacket p)
		{
			// Add player to list.
			m_data.PlayerList.AddPlayerInfo(new PlayerInfo(
				Convert.ToUInt32(p.Properties[NetPacket.PropType.PlayerId]),
				p.Properties[NetPacket.PropType.PlayerName]));
		}

		public void VisitCreateGame(NetPacket p)
		{
			throw new NotImplementedException();
		}

		public void VisitJoinGame(NetPacket p)
		{
			throw new NotImplementedException();
		}

		public void VisitJoinGameAck(NetPacket p)
		{
			m_data.MyGameId =
				Convert.ToUInt32(p.Properties[NetPacket.PropType.GameId]);
			m_callback.JoinedGame(m_data.GameList.GetGameInfo(m_data.MyGameId).Name);
		}

		public void VisitStartEvent(NetPacket p)
		{
			// Request player names for player ids.
			List<uint> playerSlots = m_data.GameList.GetGameInfo(m_data.MyGameId).PlayerSlots;
			foreach (uint id in playerSlots)
			{
				if (!m_data.PlayerList.HasPlayer(id))
				{
					NetPacketRetrievePlayerInfo request = new NetPacketRetrievePlayerInfo();
					request.Properties.Add(NetPacket.PropType.PlayerId, Convert.ToString(id));
					m_sender.Send(request);
				}
			}
			// Acknowledge start event.
			NetPacketStartEventAck ack = new NetPacketStartEventAck();
			m_sender.Send(ack);
		}

		public void VisitStartEventAck(NetPacket p)
		{
			throw new NotImplementedException();
		}

		public void VisitGameStart(NetPacket p)
		{
			// Generate player list, for gui and as hand data.
			List<string> strPlayers = new List<string>();

			List<uint> slots = p.ListProperties[NetPacket.ListPropType.PlayerSlots].
				ConvertAll<uint>(Convert.ToUInt32);
			m_players = new Dictionary<uint, Player>();

			foreach (uint i in slots)
			{
				if (m_data.PlayerList.HasPlayer(i))
					strPlayers.Add(m_data.PlayerList.GetPlayerInfo(i).Name);
				else
					strPlayers.Add(Convert.ToString(i));
				// Set player data.
				Player tmpPlayer = new Player();
				tmpPlayer.Money = m_data.GameList.GetGameInfo(m_data.MyGameId).StartMoney;
				m_players.Add(i, tmpPlayer);
			}

			m_callback.GameStarted(strPlayers);
		}

		public void VisitHandStart(NetPacket p)
		{
			// Reset players.
			foreach (KeyValuePair<uint, Player> player in m_players)
				player.Value.NewHand();

			// Assign own cards.
			int[] tmpCards = new int[2];
			tmpCards[0] = 
				Convert.ToInt32(p.Properties[NetPacket.PropType.FirstCard]);
			tmpCards[1] =
				Convert.ToInt32(p.Properties[NetPacket.PropType.SecondCard]);
			m_players[m_data.MyPlayerId].Cards = tmpCards;

			m_data.CurHand = new Hand(
				m_players,
				m_data.MyPlayerId,
				Convert.ToUInt32(p.Properties[NetPacket.PropType.SmallBlind]));
			m_callback.HandStarted(tmpCards);
		}

		public void VisitPlayersTurn(NetPacket p)
		{
			uint curPlayer = Convert.ToUInt32(p.Properties[NetPacket.PropType.PlayerId]);
			Hand.State state = (Hand.State)Convert.ToUInt16(p.Properties[NetPacket.PropType.GameState]);
			if (curPlayer == m_data.MyPlayerId)
				m_callback.MyTurn(state, m_data.CurHand.HighestSet,
					m_data.CurHand.MinimumRaise, m_players[curPlayer].Money);
			else
				m_callback.PlayersTurn(state, m_data.PlayerList.GetPlayerInfo(curPlayer).Name);
		}

		public void VisitPlayersAction(NetPacket p)
		{
			throw new NotImplementedException();
		}

		public void VisitPlayersActionDone(NetPacket p)
		{
			uint playerId = Convert.ToUInt32(p.Properties[NetPacket.PropType.PlayerId]);
			Player curPlayer = m_data.CurHand.Players[playerId];
			string name = m_data.PlayerList.GetPlayerInfo(playerId).Name;
			curPlayer.CurAction =
				(Hand.Action)Convert.ToUInt16(p.Properties[NetPacket.PropType.PlayerAction]);
			curPlayer.Money =
				Convert.ToUInt32(p.Properties[NetPacket.PropType.PlayerMoney]);
			uint curBet = Convert.ToUInt32(p.Properties[NetPacket.PropType.PlayerBetTotal])
				- curPlayer.TotalBet;
			curPlayer.TotalBet += curBet;

			m_data.CurHand.HighestSet = Convert.ToUInt32(p.Properties[NetPacket.PropType.HighestSet]);
			m_data.CurHand.MinimumRaise = Convert.ToUInt32(p.Properties[NetPacket.PropType.MinimumRaise]);

			if (curPlayer.CurAction == Hand.Action.None)
			{
				if (curBet == m_data.CurHand.SmallBlind)
					m_callback.SmallBlind(name, curBet);
				else
					m_callback.BigBlind(name, curBet);
			}
			else
			{
				m_callback.ActionDone(
					name,
					curPlayer.CurAction,
					curBet);
			}
		}

		public void VisitPlayersActionRejected(NetPacket p)
		{
			// TODO
		}

		public void VisitDealFlopCards(NetPacket p)
		{
			int[] tmpCards = new int[3];
			tmpCards[0] = Convert.ToInt32(p.Properties[NetPacket.PropType.FlopFirstCard]);
			tmpCards[1] = Convert.ToInt32(p.Properties[NetPacket.PropType.FlopSecondCard]);
			tmpCards[2] = Convert.ToInt32(p.Properties[NetPacket.PropType.FlopThirdCard]);
			m_data.CurHand.TableCards = tmpCards;
			m_callback.ShowFlopCards(tmpCards);
		}

		public void VisitDealTurnCard(NetPacket p)
		{
			int[] tmpCards = new int[4];
			m_data.CurHand.TableCards.CopyTo(tmpCards, 0);
			tmpCards[3] = Convert.ToInt32(p.Properties[NetPacket.PropType.TurnCard]);
			m_data.CurHand.TableCards = tmpCards;
			m_callback.ShowTurnCards(tmpCards);
		}

		public void VisitDealRiverCard(NetPacket p)
		{
			int[] tmpCards = new int[5];
			m_data.CurHand.TableCards.CopyTo(tmpCards, 0);
			tmpCards[4] = Convert.ToInt32(p.Properties[NetPacket.PropType.RiverCard]);
			m_data.CurHand.TableCards = tmpCards;
			m_callback.ShowRiverCards(tmpCards);
		}

		public void VisitEndOfHandShowCards(NetPacket p)
		{
			List<Dictionary<NetPacket.PropType, string>> tmpList
				= p.RecordProperties[NetPacket.RecordPropType.PlayerResult];
			foreach (Dictionary<NetPacket.PropType, string> i in tmpList)
			{
				uint playerId = Convert.ToUInt32(i[NetPacket.PropType.PlayerId]);
				string name = m_data.PlayerList.GetPlayerInfo(playerId).Name;
				Player curPlayer = m_data.CurHand.Players[playerId];
				int[] tmpCards = new int[2];
				tmpCards[0] = Convert.ToInt32(i[NetPacket.PropType.FirstCard]);
				tmpCards[1] = Convert.ToInt32(i[NetPacket.PropType.SecondCard]);
				curPlayer.Cards = tmpCards;
				int[] tmpBest = new int[5];
				tmpBest[0] = Convert.ToInt32(i[NetPacket.PropType.BestHandPos1]);
				tmpBest[1] = Convert.ToInt32(i[NetPacket.PropType.BestHandPos2]);
				tmpBest[2] = Convert.ToInt32(i[NetPacket.PropType.BestHandPos3]);
				tmpBest[3] = Convert.ToInt32(i[NetPacket.PropType.BestHandPos4]);
				tmpBest[4] = Convert.ToInt32(i[NetPacket.PropType.BestHandPos5]);
				curPlayer.BestHandPos = tmpBest;
				curPlayer.CardsValue = Convert.ToInt32(i[NetPacket.PropType.CardsValue]);
				curPlayer.Money = Convert.ToUInt32(i[NetPacket.PropType.PlayerMoney]);

				m_callback.EndOfHandShowCards(name, tmpCards, curPlayer.CardsValue, false);
			}
			foreach (Dictionary<NetPacket.PropType, string> i in tmpList)
			{
				uint moneyWon = Convert.ToUInt32(i[NetPacket.PropType.MoneyWon]);
				if (moneyWon > 0)
				{
					uint playerId = Convert.ToUInt32(i[NetPacket.PropType.PlayerId]);
					string name = m_data.PlayerList.GetPlayerInfo(playerId).Name;
					m_callback.PlayerWins(name, moneyWon);
				}
			}
		}

		public void VisitEndOfHandHideCards(NetPacket p)
		{
			uint playerId = Convert.ToUInt32(p.Properties[NetPacket.PropType.PlayerId]);
			string name = m_data.PlayerList.GetPlayerInfo(playerId).Name;
			Player curPlayer = m_data.CurHand.Players[playerId];
			curPlayer.Money = Convert.ToUInt32(p.Properties[NetPacket.PropType.PlayerMoney]);

			m_callback.PlayerWins(
				name,
				Convert.ToUInt32(p.Properties[NetPacket.PropType.MoneyWon]));
		}

		private PokerTHData m_data;
		private SenderThread m_sender;
		private ICallback m_callback;
		private Dictionary<uint, Player> m_players;
	}
}
