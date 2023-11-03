/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_FINISHED_ITEM_H
#define DCPLUSPLUS_DCPP_FINISHED_ITEM_H

#include <vector>

#include "forward.h"
#include "typedefs.h"

#include "Util.h"
#include "ClientManager.h"
#include "HintedUser.h"

namespace dcpp {

using std::vector;

class FinishedItem
{
public:
	typedef vector<FinishedItemPtr> List;

	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_DONE,
		COLUMN_PATH,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_SIZE,
		COLUMN_SPEED,
		COLUMN_LAST
	};

	FinishedItem(string const& aTarget, const HintedUser& aUser,  int64_t aSize, int64_t aSpeed, time_t aTime, const string& aTTH = Util::emptyString) : 
		target(aTarget), user(aUser), size(aSize), avgSpeed(aSpeed), time(aTime), tth(aTTH)
	{
	}

	const tstring getText(uint8_t col) const {
		dcassert(col >= 0 && col < COLUMN_LAST);
		switch(col) {
			case COLUMN_FILE: return Text::toT(Util::getFileName(getTarget()));
			case COLUMN_DONE: return Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", getTime()));
			case COLUMN_PATH: return Text::toT(Util::getFilePath(getTarget()));
			case COLUMN_NICK: return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(getUser())));
			case COLUMN_HUB: return Text::toT(Util::toString(ClientManager::getInstance()->getHubNames(getUser()))); 
			case COLUMN_SIZE: return Util::formatBytesT(getSize());
			case COLUMN_SPEED: return Text::toT(Util::formatBytes(getAvgSpeed()) + "/s");
			default: return Util::emptyStringT;
		}
	}

	static int compareItems(const FinishedItem* a, const FinishedItem* b, uint8_t col) {
		switch(col) {
			case COLUMN_SPEED:	return compare(a->getAvgSpeed(), b->getAvgSpeed());
			case COLUMN_SIZE:	return compare(a->getSize(), b->getSize());
			default:			return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
		}
	}
	int getImageIndex() const;

	GETSET(string, target, Target);
	GETSET(string, tth, TTH);

	GETSET(int64_t, size, Size);
	GETSET(int64_t, avgSpeed, AvgSpeed);
	GETSET(time_t, time, Time);
	GETSET(HintedUser, user, User);

private:
	friend class FinishedManager;

};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_FINISHED_ITEM_H)
