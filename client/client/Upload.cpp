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

#include "stdinc.h"
#include "Upload.h"

#include "UserConnection.h"
#include "Streams.h"

namespace dcpp {

Upload::Upload(UserConnection& conn, const string& path, const TTHValue& tth) : Transfer(conn, path, tth), stream(0), fileSize(-1), downloadedBytes(-1), delayTime(0) { 
	conn.setUpload(this);
}

Upload::~Upload() { 
	getUserConnection().setUpload(0);
	delete stream; 
}

int64_t Upload::getPos(bool fullFile /*= false*/) const {
	if (!fullFile)
		return Transfer::getPos();
	return (getDownloadedBytes() == -1 ? getStartPos() : getDownloadedBytes()) + Transfer::getPos();
}

int64_t Upload::getSize(bool fullFile /*= false*/) const {
	if (!fullFile)
		return Transfer::getSize();
	return getFileSize() == -1 ? Transfer::getSize() : getFileSize();
}

int64_t Upload::getActual(bool fullFile /*= false*/) const {
	if (!fullFile)
		return Transfer::getActual();
	return (getDownloadedBytes() == -1 ? getStartPos() : getDownloadedBytes()) + Transfer::getActual();
}

int64_t Upload::getSecondsLeft(bool fullFile /*= false*/) const {
	double avg = getAverageSpeed();
	int64_t bytesLeft =  getSize(fullFile) - getPos(fullFile);
	return (avg > 0) ? static_cast<int64_t>(bytesLeft / avg) : 0;
}

void Upload::getParams(const UserConnection& aSource, StringMap& params) const {
	Transfer::getParams(aSource, params);
	params["source"] = getPath();
}

} // namespace dcpp
