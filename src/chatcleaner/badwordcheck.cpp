#include "badwordcheck.h"

#include <QtCore>

BadWordCheck::BadWordCheck()
{
	badWords << "arsch" << "asshole" << "bastard" << "bitch" << "cunt" << " dick " << "drecksau" << " fag " << "fagget" << "fotze" << "fuker" 
			<< "fuck" << " fuk " << "gay" << "horny" << "hure" << "mistgeburt" << "missgeburt" << "motherfucker" << "nazi" << "nigga" 
			<< "nigger" << "nutte" << "ommak" << "penis" << "pussy" << "schlampe" << "schwanz" << "sex" << "shit" << "sieg heil" << "slut" 
			<< "suck" << "whore";
}

bool BadWordCheck::run(QString msg) 
{
	QStringListIterator it(badWords);
	while (it.hasNext()) {
		if(msg.toLower().contains(it.next()))
			return true;
	}
	return false;
}
