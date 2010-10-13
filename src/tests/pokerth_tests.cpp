#include <boost/shared_ptr.hpp>
#include <net/netpacket.h>
#include <game_defs.h>

int
main()
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_announceMessage;
	AnnounceMessage_t *netAnnounce = &packet->GetMsg()->choice.announceMessage;
	netAnnounce->protocolVersion.major = NET_VERSION_MAJOR;
	netAnnounce->protocolVersion.minor = NET_VERSION_MINOR;
	netAnnounce->latestGameVersion.major = POKERTH_VERSION_MAJOR;
	netAnnounce->latestGameVersion.minor = POKERTH_VERSION_MINOR;
	netAnnounce->latestBetaRevision = POKERTH_BETA_REVISION;

	return 0;
}
