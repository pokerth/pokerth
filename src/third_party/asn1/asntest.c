#include <stdlib.h>
#include <stdio.h>

#include "PokerTHMessage.h"

ssize_t
simple_serializer(PokerTHMessage_t *msg, unsigned char *buf, size_t size) {
	asn_enc_rval_t er; /* Encoder return value */
	er = der_encode_to_buffer(&asn_DEF_PokerTHMessage, msg, buf, size);
	if(er.encoded == -1) {
		/*
		* Failed to encode the rectangle data.
		*/
		fprintf(stderr, "Cannot encode %s\n", er.failed_type->name);
		return -1;
	} else {
		/* Return the number of bytes */
		return er.encoded;
	}
}

PokerTHMessage_t *
simple_deserializer(const unsigned char *buf, size_t size) {
	PokerTHMessage_t *msg = 0; /* Note this 0! */
	asn_dec_rval_t rval;
	rval = ber_decode(0, &asn_DEF_PokerTHMessage, (void **)&msg, buf, size);
	if(rval.code == RC_OK) {
		return msg; /* Decoding succeeded */
	} else {
		/* Free partially decoded message */
		ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
		return 0;
	}
}

int
main()
{
	PokerTHMessage_t *msg;
	unsigned char buf[256];
	ssize_t outSize, i;
	msg = calloc(1, sizeof(PokerTHMessage_t));
	memset(msg, 0, sizeof(PokerTHMessage_t));
	msg->present = PokerTHMessage_PR_initMessage;
	msg->choice.initMessage.requestedVersion.major = 1;
	msg->choice.initMessage.login.present = login_PR_anonymousLogin;
	OCTET_STRING_fromString(&msg->choice.initMessage.login.choice.anonymousLogin.playerName, "Hallo");
	outSize = simple_serializer(msg, buf, sizeof(buf));
	ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
	for (i = 0; i < outSize; i++)
		printf("%02x ", buf[i]);
	printf("\n");
	msg = simple_deserializer(buf, outSize - 1);
	if (msg)
		printf("error\n");
	msg = simple_deserializer(buf, outSize+1);
	xer_fprint(stdout, &asn_DEF_PokerTHMessage, msg);
	return 0;
}

