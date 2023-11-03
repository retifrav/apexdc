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

#ifndef DCPLUSPLUS_DCPP_FINISHED_MANAGER_H
#define DCPLUSPLUS_DCPP_FINISHED_MANAGER_H

#include "QueueManagerListener.h"
#include "UploadManagerListener.h"
#include "FinishedManagerListener.h"

#include "Speaker.h"
#include "Singleton.h"
#include "User.h"
#include "MerkleTree.h"
#include "FinishedItem.h"
#include "HintedUser.h"
#include "SettingsManager.h"

namespace dcpp {

class FinishedManager : public Singleton<FinishedManager>,
	public Speaker<FinishedManagerListener>, private QueueManagerListener, private UploadManagerListener, private SettingsManagerListener
{
public:
	const FinishedItem::List& lockList(bool upload = false) { cs.lock(); return upload ? uploads : downloads; }
	void unlockList() { cs.unlock(); }

	void remove(FinishedItemPtr item, bool upload = false);
	void removeAll(bool upload = false);

private:
	friend class Singleton<FinishedManager>;
	
	FinishedManager();
	~FinishedManager();

	void on(QueueManagerListener::Finished, const QueueItem*, const string&, const Download*) noexcept;
	void on(UploadManagerListener::Complete, const Upload*) noexcept;

	void on(SettingsManagerListener::Load, SimpleXML& /*xml*/) noexcept;
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	CriticalSection cs;
	FinishedItem::List downloads, uploads;
};

} // namespace dcpp

#endif // !defined(FINISHED_MANAGER_H)

/**
 * @file
 * $Id$
 */
