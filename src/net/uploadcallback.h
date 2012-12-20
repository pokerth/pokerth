/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/
/* Network file upload callback. */

#ifndef _UPLOADCALLBACK_H_
#define _UPLOADCALLBACK_H_

#include <string>

class UploadCallback
{
public:
	virtual ~UploadCallback();

	virtual void UploadCompleted(const std::string &filename, const std::string &returnMessage) = 0;
	virtual void UploadError(const std::string &filename, const std::string &errorMessage) = 0;
};

#endif // _UPLOADCALLBACK_H_
