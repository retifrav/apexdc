/* 
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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
#include "LogManager.h"

#include "LogManagerListener.h"

#include "File.h"
#include "TimerManager.h"

namespace dcpp {

using std::make_pair;

void LogManager::log(Area area, StringMap& params) noexcept {
	log(getPath(area, params), Util::formatParams(getSetting(area, FORMAT), params, false));
}

void LogManager::message(const string& msg, Severity severity) {
	if(BOOLSETTING(LOG_SYSTEM)) {
		StringMap params;
		params["message"] = msg;
		log(SYSTEM, params);
	}

	{
		Lock l(cs);
		// Keep the last 200 messages (completely arbitrary number...)
		// we just need it to display logs before GUI opens
		while(lastLogs.size() > 200)
			lastLogs.pop_front();
		lastLogs.push_back(make_pair(msg, MessageData(GET_TIME(), severity)));
	}

	fire(LogManagerListener::Message(), msg, severity);
}

const LogManager::List& LogManager::getLastLogs() {
	Lock l(cs); return lastLogs;
}

string LogManager::getPath(Area area, StringMap& params) const {
	// Disable filtering of formatParam in lieu the one provided by validateFileName
	return SETTING(LOG_DIRECTORY) + Util::formatParams(getSetting(area, FILE), params, false);
}

string LogManager::getPath(Area area) const {
	StringMap params;
	return getPath(area, params);
}

const string& LogManager::getSetting(int area, int sel) const {
	return SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(options[area][sel]), true);
}

void LogManager::saveSetting(int area, int sel, const string& setting) {
	SettingsManager::getInstance()->set(static_cast<SettingsManager::StrSetting>(options[area][sel]), setting);
}

void LogManager::saveCache() noexcept {
	Lock l(cs);
	for(StringMapIter i = logCache.begin(); i != logCache.end(); ++i) {
		try {
			string file = Util::validateFileName(i->first);
			File::ensureDirectory(file);
			File f(file, File::WRITE, File::OPEN | File::CREATE);
			f.setEndPos(0);
			if(f.getPos() == 0) {
				f.write("\xef\xbb\xbf");
			}
			f.write(i->second);
		} catch (const FileException&) {
			// ...
		}
	}

	// reset cache
	logCache.clear();
}

LogManager::LogManager() {
	options[UPLOAD][FILE]		= SettingsManager::LOG_FILE_UPLOAD;
	options[UPLOAD][FORMAT]		= SettingsManager::LOG_FORMAT_POST_UPLOAD;
    options[DOWNLOAD][FILE]		= SettingsManager::LOG_FILE_DOWNLOAD;
	options[DOWNLOAD][FORMAT]	= SettingsManager::LOG_FORMAT_POST_DOWNLOAD;
	options[CHAT][FILE]			= SettingsManager::LOG_FILE_MAIN_CHAT;
	options[CHAT][FORMAT]		= SettingsManager::LOG_FORMAT_MAIN_CHAT;
	options[PM][FILE]			= SettingsManager::LOG_FILE_PRIVATE_CHAT;
	options[PM][FORMAT]			= SettingsManager::LOG_FORMAT_PRIVATE_CHAT;
	options[SYSTEM][FILE]		= SettingsManager::LOG_FILE_SYSTEM;
	options[SYSTEM][FORMAT]		= SettingsManager::LOG_FORMAT_SYSTEM;
	options[STATUS][FILE]		= SettingsManager::LOG_FILE_STATUS;
	options[STATUS][FORMAT]		= SettingsManager::LOG_FORMAT_STATUS;
	options[WEBSERVER][FILE]	= SettingsManager::LOG_FILE_WEBSERVER;
	options[WEBSERVER][FORMAT]	= SettingsManager::WEBSERVER_FORMAT;

	TimerManager::getInstance()->addListener(this);
}

LogManager::~LogManager() {
	TimerManager::getInstance()->removeListener(this);
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
