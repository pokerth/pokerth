# QMake pro-file for PokerTH common library
TEMPLATE = lib
CODECFORSRC = UTF-8
CONFIG += staticlib \
	thread \
	exceptions \
	rtti \
	stl \
	warn_on
UI_DIR = uics
TARGET = lib/pokerth_protocol
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += ENABLE_IPV6
QT -= core \
	gui
# PRECOMPILED_HEADER = src/pch_lib.h

INCLUDEPATH += . \
	src \
	src/third_party/asn1
DEPENDPATH += . \
	src \
	src/third_party/asn1

# Input
HEADERS += src/third_party/asn1/AllInShowCardsMessage.h \
	src/third_party/asn1/ChatCleanerMessage.h \
	src/third_party/asn1/CleanerInitMessage.h \
	src/third_party/asn1/CleanerInitAckMessage.h \
	src/third_party/asn1/CleanerChatRequestMessage.h \
	src/third_party/asn1/CleanerChatReplyMessage.h \
	src/third_party/asn1/GuestLogin.h \
	src/third_party/asn1/AskKickDeniedMessage.h \
	src/third_party/asn1/AskKickPlayerMessage.h \
	src/third_party/asn1/AuthMessage.h \
	src/third_party/asn1/AuthClientResponse.h \
	src/third_party/asn1/AuthServerChallenge.h \
	src/third_party/asn1/AuthServerVerification.h \
	src/third_party/asn1/asn_application.h \
	src/third_party/asn1/asn_codecs.h \
	src/third_party/asn1/asn_codecs_prim.h \
	src/third_party/asn1/asn_internal.h \
	src/third_party/asn1/asn_SEQUENCE_OF.h \
	src/third_party/asn1/asn_SET_OF.h \
	src/third_party/asn1/asn_system.h \
	src/third_party/asn1/AuthenticatedLogin.h \
	src/third_party/asn1/AvatarData.h \
	src/third_party/asn1/AvatarEnd.h \
	src/third_party/asn1/AvatarHash.h \
	src/third_party/asn1/AvatarHeader.h \
	src/third_party/asn1/AvatarReplyMessage.h \
	src/third_party/asn1/AvatarRequestMessage.h \
	src/third_party/asn1/ber_decoder.h \
	src/third_party/asn1/ber_tlv_length.h \
	src/third_party/asn1/ber_tlv_tag.h \
	src/third_party/asn1/BIT_STRING.h \
	src/third_party/asn1/BOOLEAN.h \
	src/third_party/asn1/Card.h \
	src/third_party/asn1/ChatMessage.h \
	src/third_party/asn1/ChatTypeGame.h \
	src/third_party/asn1/ChatTypeLobby.h \
	src/third_party/asn1/ChatTypeBot.h \
	src/third_party/asn1/ChatTypeBroadcast.h \
	src/third_party/asn1/ChatRequestMessage.h \
	src/third_party/asn1/ChatRequestTypeLobby.h \
	src/third_party/asn1/ChatRequestTypeGame.h \
	src/third_party/asn1/constraints.h \
	src/third_party/asn1/constr_CHOICE.h \
	src/third_party/asn1/constr_SEQUENCE.h \
	src/third_party/asn1/constr_SEQUENCE_OF.h \
	src/third_party/asn1/constr_SET_OF.h \
	src/third_party/asn1/constr_TYPE.h \
	src/third_party/asn1/DealFlopCardsMessage.h \
	src/third_party/asn1/DealRiverCardMessage.h \
	src/third_party/asn1/DealTurnCardMessage.h \
	src/third_party/asn1/der_encoder.h \
	src/third_party/asn1/DialogMessage.h \
	src/third_party/asn1/EndKickPetitionMessage.h \
	src/third_party/asn1/EndOfGameMessage.h \
	src/third_party/asn1/EndOfHandHideCards.h \
	src/third_party/asn1/EndOfHandMessage.h \
	src/third_party/asn1/EndOfHandShowCards.h \
	src/third_party/asn1/ErrorMessage.h \
	src/third_party/asn1/GameAdminChanged.h \
	src/third_party/asn1/GameListAdminChanged.h \
	src/third_party/asn1/GameListMessage.h \
	src/third_party/asn1/GameListNew.h \
	src/third_party/asn1/GameListPlayerJoined.h \
	src/third_party/asn1/GameListPlayerLeft.h \
	src/third_party/asn1/GameListUpdate.h \
	src/third_party/asn1/GamePlayerJoined.h \
	src/third_party/asn1/GamePlayerLeft.h \
	src/third_party/asn1/GamePlayerMessage.h \
	src/third_party/asn1/GameStartMessage.h \
	src/third_party/asn1/Guid.h \
	src/third_party/asn1/HandStartMessage.h \
	src/third_party/asn1/Id.h \
	src/third_party/asn1/InitAckMessage.h \
	src/third_party/asn1/InitMessage.h \
	src/third_party/asn1/INTEGER.h \
	src/third_party/asn1/InviteNotifyMessage.h \
	src/third_party/asn1/InvitePlayerToGameMessage.h \
	src/third_party/asn1/JoinExistingGame.h \
	src/third_party/asn1/JoinGameAck.h \
	src/third_party/asn1/JoinGameFailed.h \
	src/third_party/asn1/JoinGameReplyMessage.h \
	src/third_party/asn1/JoinGameRequestMessage.h \
	src/third_party/asn1/JoinNewGame.h \
	src/third_party/asn1/KickPetitionUpdateMessage.h \
	src/third_party/asn1/KickPlayerRequestMessage.h \
	src/third_party/asn1/LeaveGameRequestMessage.h \
	src/third_party/asn1/MyActionRequestMessage.h \
	src/third_party/asn1/NativeEnumerated.h \
	src/third_party/asn1/NativeInteger.h \
	src/third_party/asn1/NetAvatarType.h \
	src/third_party/asn1/NetGameInfo.h \
	src/third_party/asn1/NetGameMode.h \
	src/third_party/asn1/NetGameState.h \
	src/third_party/asn1/NetPlayerAction.h \
	src/third_party/asn1/NonZeroId.h \
	src/third_party/asn1/OCTET_STRING.h \
	src/third_party/asn1/per_decoder.h \
	src/third_party/asn1/per_encoder.h \
	src/third_party/asn1/per_opentype.h \
	src/third_party/asn1/per_support.h \
	src/third_party/asn1/PlayerAllIn.h \
	src/third_party/asn1/PlayerInfoData.h \
	src/third_party/asn1/PlayerInfoRights.h \
	src/third_party/asn1/PlayerInfoReplyMessage.h \
	src/third_party/asn1/PlayerInfoRequestMessage.h \
	src/third_party/asn1/PlayerListMessage.h \
	src/third_party/asn1/PlayerResult.h \
	src/third_party/asn1/PlayersActionDoneMessage.h \
	src/third_party/asn1/PlayersTurnMessage.h \
	src/third_party/asn1/PokerTHMessage.h \
	src/third_party/asn1/RejectGameInvitationMessage.h \
	src/third_party/asn1/RejectGameInvReason.h \
	src/third_party/asn1/RejectInvNotifyMessage.h \
	src/third_party/asn1/RemovedFromGame.h \
	src/third_party/asn1/ResetTimeoutMessage.h \
	src/third_party/asn1/StartEventAckMessage.h \
	src/third_party/asn1/StartEventMessage.h \
	src/third_party/asn1/StartKickPetitionMessage.h \
	src/third_party/asn1/StatisticsData.h \
	src/third_party/asn1/StatisticsMessage.h \
	src/third_party/asn1/SubscriptionRequestMessage.h \
	src/third_party/asn1/TimeoutWarningMessage.h \
	src/third_party/asn1/UnknownAvatar.h \
	src/third_party/asn1/UnknownPlayerInfo.h \
	src/third_party/asn1/UTF8String.h \
	src/third_party/asn1/Version.h \
	src/third_party/asn1/VoteKickAck.h \
	src/third_party/asn1/VoteKickDenied.h \
	src/third_party/asn1/VoteKickReplyMessage.h \
	src/third_party/asn1/VoteKickRequestMessage.h \
	src/third_party/asn1/xer_decoder.h \
	src/third_party/asn1/xer_encoder.h \
	src/third_party/asn1/xer_support.h \
	src/third_party/asn1/YourActionRejectedMessage.h \
	src/third_party/asn1/AnnounceMessage.h \
	src/third_party/asn1/UnauthenticatedLogin.h \
	src/third_party/asn1/AfterHandShowCardsMessage.h \
	src/third_party/asn1/ShowMyCardsRequestMessage.h \
	src/third_party/asn1/PlainCards.h \
	src/third_party/asn1/EncryptedCards.h \
	src/third_party/asn1/AfkWarningMessage.h \
    src/third_party/asn1/ChatRequestTypePrivate.h \
    src/third_party/asn1/ReportAvatarMessage.h \
    src/third_party/asn1/ReportAvatarAckMessage.h \
    src/third_party/asn1/CleanerChatTypeLobby.h \
    src/third_party/asn1/CleanerChatTypeGame.h \
    src/third_party/asn1/CleanerChatType.h
SOURCES += src/third_party/asn1/ChatCleanerMessage.c \
	src/third_party/asn1/CleanerInitMessage.c \
	src/third_party/asn1/CleanerInitAckMessage.c \
	src/third_party/asn1/CleanerChatRequestMessage.c \
	src/third_party/asn1/CleanerChatReplyMessage.c \
	src/third_party/asn1/AuthMessage.c \
	src/third_party/asn1/AuthClientResponse.c \
	src/third_party/asn1/AuthServerChallenge.c \
	src/third_party/asn1/AuthServerVerification.c \
	src/third_party/asn1/AllInShowCardsMessage.c \
	src/third_party/asn1/YourActionRejectedMessage.c \
	src/third_party/asn1/xer_support.c \
	src/third_party/asn1/xer_encoder.c \
	src/third_party/asn1/xer_decoder.c \
	src/third_party/asn1/VoteKickRequestMessage.c \
	src/third_party/asn1/VoteKickReplyMessage.c \
	src/third_party/asn1/VoteKickDenied.c \
	src/third_party/asn1/VoteKickAck.c \
	src/third_party/asn1/Version.c \
	src/third_party/asn1/UTF8String.c \
	src/third_party/asn1/UnknownPlayerInfo.c \
	src/third_party/asn1/UnknownAvatar.c \
	src/third_party/asn1/TimeoutWarningMessage.c \
	src/third_party/asn1/SubscriptionRequestMessage.c \
	src/third_party/asn1/StatisticsMessage.c \
	src/third_party/asn1/StatisticsData.c \
	src/third_party/asn1/StartKickPetitionMessage.c \
	src/third_party/asn1/StartEventMessage.c \
	src/third_party/asn1/StartEventAckMessage.c \
	src/third_party/asn1/ResetTimeoutMessage.c \
	src/third_party/asn1/RemovedFromGame.c \
	src/third_party/asn1/PokerTHMessage.c \
	src/third_party/asn1/PlayersTurnMessage.c \
	src/third_party/asn1/PlayersActionDoneMessage.c \
	src/third_party/asn1/PlayerResult.c \
	src/third_party/asn1/PlayerInfoRequestMessage.c \
	src/third_party/asn1/PlayerInfoReplyMessage.c \
	src/third_party/asn1/PlayerInfoData.c \
	src/third_party/asn1/PlayerInfoRights.c \
	src/third_party/asn1/PlayerAllIn.c \
	src/third_party/asn1/PlayerListMessage.c \
	src/third_party/asn1/per_support.c \
	src/third_party/asn1/per_opentype.c \
	src/third_party/asn1/per_encoder.c \
	src/third_party/asn1/per_decoder.c \
	src/third_party/asn1/OCTET_STRING.c \
	src/third_party/asn1/NonZeroId.c \
	src/third_party/asn1/NetPlayerAction.c \
	src/third_party/asn1/NetGameState.c \
	src/third_party/asn1/NetGameMode.c \
	src/third_party/asn1/NetGameInfo.c \
	src/third_party/asn1/NetAvatarType.c \
	src/third_party/asn1/NativeInteger.c \
	src/third_party/asn1/NativeEnumerated.c \
	src/third_party/asn1/MyActionRequestMessage.c \
	src/third_party/asn1/LeaveGameRequestMessage.c \
	src/third_party/asn1/KickPlayerRequestMessage.c \
	src/third_party/asn1/KickPetitionUpdateMessage.c \
	src/third_party/asn1/JoinNewGame.c \
	src/third_party/asn1/JoinGameRequestMessage.c \
	src/third_party/asn1/JoinGameReplyMessage.c \
	src/third_party/asn1/JoinGameFailed.c \
	src/third_party/asn1/JoinGameAck.c \
	src/third_party/asn1/JoinExistingGame.c \
	src/third_party/asn1/INTEGER.c \
	src/third_party/asn1/InitMessage.c \
	src/third_party/asn1/InitAckMessage.c \
	src/third_party/asn1/Id.c \
	src/third_party/asn1/HandStartMessage.c \
	src/third_party/asn1/Guid.c \
	src/third_party/asn1/GameStartMessage.c \
	src/third_party/asn1/GamePlayerMessage.c \
	src/third_party/asn1/GamePlayerLeft.c \
	src/third_party/asn1/GamePlayerJoined.c \
	src/third_party/asn1/GameListUpdate.c \
	src/third_party/asn1/GameListPlayerLeft.c \
	src/third_party/asn1/GameListPlayerJoined.c \
	src/third_party/asn1/GameListNew.c \
	src/third_party/asn1/GameListMessage.c \
	src/third_party/asn1/GameListAdminChanged.c \
	src/third_party/asn1/GameAdminChanged.c \
	src/third_party/asn1/ErrorMessage.c \
	src/third_party/asn1/EndOfHandShowCards.c \
	src/third_party/asn1/EndOfHandMessage.c \
	src/third_party/asn1/EndOfHandHideCards.c \
	src/third_party/asn1/EndOfGameMessage.c \
	src/third_party/asn1/EndKickPetitionMessage.c \
	src/third_party/asn1/DialogMessage.c \
	src/third_party/asn1/der_encoder.c \
	src/third_party/asn1/DealTurnCardMessage.c \
	src/third_party/asn1/DealRiverCardMessage.c \
	src/third_party/asn1/DealFlopCardsMessage.c \
	src/third_party/asn1/constraints.c \
	src/third_party/asn1/constr_TYPE.c \
	src/third_party/asn1/constr_SET_OF.c \
	src/third_party/asn1/constr_SEQUENCE_OF.c \
	src/third_party/asn1/constr_SEQUENCE.c \
	src/third_party/asn1/constr_CHOICE.c \
	src/third_party/asn1/ChatRequestMessage.c \
	src/third_party/asn1/ChatRequestTypeLobby.c \
	src/third_party/asn1/ChatRequestTypeGame.c \
	src/third_party/asn1/ChatMessage.c \
	src/third_party/asn1/ChatTypeGame.c \
	src/third_party/asn1/ChatTypeLobby.c \
	src/third_party/asn1/ChatTypeBot.c \
	src/third_party/asn1/ChatTypeBroadcast.c \
	src/third_party/asn1/Card.c \
	src/third_party/asn1/BOOLEAN.c \
	src/third_party/asn1/BIT_STRING.c \
	src/third_party/asn1/ber_tlv_tag.c \
	src/third_party/asn1/ber_tlv_length.c \
	src/third_party/asn1/ber_decoder.c \
	src/third_party/asn1/AvatarRequestMessage.c \
	src/third_party/asn1/AvatarReplyMessage.c \
	src/third_party/asn1/AvatarHeader.c \
	src/third_party/asn1/AvatarHash.c \
	src/third_party/asn1/AvatarEnd.c \
	src/third_party/asn1/AvatarData.c \
	src/third_party/asn1/AuthenticatedLogin.c \
	src/third_party/asn1/asn_SET_OF.c \
	src/third_party/asn1/asn_SEQUENCE_OF.c \
	src/third_party/asn1/asn_codecs_prim.c \
	src/third_party/asn1/AskKickPlayerMessage.c \
	src/third_party/asn1/AskKickDeniedMessage.c \
	src/third_party/asn1/GuestLogin.c \
	src/third_party/asn1/AnnounceMessage.c \
	src/third_party/asn1/UnauthenticatedLogin.c \
	src/third_party/asn1/InvitePlayerToGameMessage.c \
	src/third_party/asn1/InviteNotifyMessage.c \
	src/third_party/asn1/RejectGameInvitationMessage.c \
	src/third_party/asn1/RejectInvNotifyMessage.c \
	src/third_party/asn1/RejectGameInvReason.c \
	src/third_party/asn1/AfterHandShowCardsMessage.c \
	src/third_party/asn1/ShowMyCardsRequestMessage.c \
	src/third_party/asn1/EncryptedCards.c \
	src/third_party/asn1/PlainCards.c \
	src/third_party/asn1/AfkWarningMessage.c \
    src/third_party/asn1/ChatRequestTypePrivate.c \
    src/third_party/asn1/ReportAvatarMessage.c \
    src/third_party/asn1/ReportAvatarAckMessage.c \
    src/third_party/asn1/CleanerChatTypeLobby.c \
    src/third_party/asn1/CleanerChatTypeGame.c \
    src/third_party/asn1/CleanerChatType.c
win32 { 
	DEFINES += CURL_STATICLIB
	DEFINES += _WIN32_WINNT=0x0501
}
mac { 
	# make it universal
	CONFIG += x86
	CONFIG -= ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	# QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/
	INCLUDEPATH += /Developer/SDKs/MacOSX10.5.sdk/usr/include/
	INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
	INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers
}
