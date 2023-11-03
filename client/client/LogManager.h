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

#ifndef DCPLUSPLUS_DCPP_LOG_MANAGER_H
#define DCPLUSPLUS_DCPP_LOG_MANAGER_H

#include <deque>
#include <utility>

#include "Util.h"
#include "File.h"
#include "Singleton.h"
#include "Speaker.h"
#include "LogManagerListener.h"
#include "TimerManager.h"

namespace dcpp {

using std::deque;
using std::pair;

class LogManager : public Singleton<LogManager>, public Speaker<LogManagerListener>, private TimerManagerListener
{
public:
	enum Severity { LOG_INFO, LOG_WARNING, LOG_ERROR };
	enum Area { CHAT, PM, DOWNLOAD, UPLOAD, SYSTEM, STATUS, WEBSERVER, LAST };
	enum { FILE, FORMAT };

	struct MessageData {
		MessageData(time_t t, Severity sev) : time(t), severity(sev) { }

		time_t time;
		Severity severity;
	};

	typedef deque<pair<string, MessageData> > List;

	void log(Area area, StringMap& params) noexcept;
	void message(const string& msg, Severity severity);

	const List& getLastLogs();
	string getPath(Area area, StringMap& params) const;
	string getPath(Area area) const;
 
	const string& getSetting(int area, int sel) const;
	void saveSetting(int area, int sel, const string& setting);

	void saveCache() noexcept;

	void clearLastLogs() { Lock l(cs); lastLogs.clear(); }

private:
	void log(const string& area, const string& msg) noexcept {
		Lock l(cs);
		logCache[area] += msg + NATIVE_NL;
	}

	void on(TimerManagerListener::Minute, uint64_t /*aTick*/) noexcept {
		saveCache();
	}

	friend class Singleton<LogManager>;
	CriticalSection cs;
	List lastLogs;
	StringMap logCache;
	
	int options[LAST][2];

	LogManager();
	~LogManager();

};

#define LOG(area, msg) LogManager::getInstance()->log(area, msg)

} // namespace dcpp

#endif // !defined(LOG_MANAGER_H)

/**
 * @file
 * $Id$
 */
