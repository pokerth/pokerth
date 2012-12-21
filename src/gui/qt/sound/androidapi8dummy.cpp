#include "configfile.h"
#include "androidapi8dummy.h"

AndroidApi8Dummy::AndroidApi8Dummy(ConfigFile *c, QObject *parent) :
	QObject(parent), myConfig(c)
{
}

AndroidApi8Dummy::~AndroidApi8Dummy()
{
}
