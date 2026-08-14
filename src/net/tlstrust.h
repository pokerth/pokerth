/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *****************************************************************************/
/* TLS trust setup for Qt network requests. */

#ifndef _TLSTRUST_H_
#define _TLSTRUST_H_

#include <QNetworkRequest>

namespace TlsTrust
{

// Configure platform trust roots for a request. Desktop Qt uses the platform
// defaults; Android needs an explicit root list.
void ConfigureRequest(QNetworkRequest &request);

}

#endif
