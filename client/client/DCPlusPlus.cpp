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
#include "DCPlusPlus.h"

#include "ConnectionManager.h"
#include "DownloadManager.h"
#include "UploadManager.h"
#include "CryptoManager.h"
#include "ShareManager.h"
#include "SearchManager.h"
#include "QueueManager.h"
#include "ClientManager.h"
#include "HashManager.h"
#include "LogManager.h"
#include "FavoriteManager.h"
#include "SettingsManager.h"
#include "FinishedManager.h"
#include "ADLSearch.h"
#include "MappingManager.h"
#include "ConnectivityManager.h"

#include "DebugManager.h"
#include "DetectionManager.h"
#include "WebServerManager.h"
#include "ThrottleManager.h"
#include "File.h"

#include "RawManager.h"
#include "PluginManager.h"
#include "HttpManager.h"
#include "UpdateManager.h"

#include <dht/DHT.h>

namespace dcpp {

using dht::DHT;

void startup() {
	// "Dedicated to the near-memory of Nev. Let's start remembering people while they're still alive."
	// Nev's great contribution to dc++
	while(1) break;


#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	Util::initialize();

	ResourceManager::newInstance();
	SettingsManager::newInstance();

	TimerManager::newInstance();
	LogManager::newInstance();
	HashManager::newInstance();
	CryptoManager::newInstance();
	SearchManager::newInstance();
	ClientManager::newInstance();
	ConnectionManager::newInstance();
	DownloadManager::newInstance();
	UploadManager::newInstance();
	ThrottleManager::newInstance();
	QueueManager::newInstance();
	ShareManager::newInstance();
	HttpManager::newInstance();
	UpdateManager::newInstance();
	FavoriteManager::newInstance();
	FinishedManager::newInstance();
	RawManager::newInstance();
	ADLSearchManager::newInstance();
	ConnectivityManager::newInstance();
	MappingManager::newInstance();
	DebugManager::newInstance();
	WebServerManager::newInstance();
	PluginManager::newInstance();
	PluginApiImpl::init();

	DetectionManager::newInstance();
}

void load(function<void (const string&)> stepF, function<void (float)> progressF) {
	SettingsManager::getInstance()->load();	

	if(!SETTING(LANGUAGE_FILE).empty()) {
		string languageFile = SETTING(LANGUAGE_FILE);
		if(!File::isAbsolute(languageFile))
			languageFile = Util::getPath(Util::PATH_LOCALE) + languageFile;
		ResourceManager::getInstance()->loadLanguage(languageFile);
	}

	auto announce = [&stepF](const string& str) {
		if(stepF) {
			stepF(_("Loading: ") + str);
		}
	};

	announce(STRING(FAVORITE_HUBS) + " & " + STRING(FAVORITE_USERS));
	FavoriteManager::getInstance()->load();

	PluginManager::getInstance()->loadPlugins(announce);

	announce(STRING(SECURITY_CERTIFICATES));
	CryptoManager::getInstance()->loadCertificates();

	// Deprecated, these should be removed (DHT has no IPv6 support and is no longer maintained)
	DetectionManager::getInstance()->load();
	DHT::newInstance();

	announce(STRING(HASH_DATABASE));
	HashManager::getInstance()->startup(progressF);

	announce(STRING(SHARED_FILES));
	ShareManager::getInstance()->refresh(true, false, true, progressF);

	announce(STRING(DOWNLOAD_QUEUE));
	QueueManager::getInstance()->loadQueue(progressF);

	if(stepF && Util::fileExists(UPDATE_TEMP_DIR)) {
		stepF(_("Removing temporary updater files..."));
		UpdateManager::cleanTempFiles();
	}
}

void shutdown() {
	PluginManager::getInstance()->unloadPlugins();
	HashManager::getInstance()->shutdown();
	ThrottleManager::getInstance()->shutdown();
	TimerManager::getInstance()->shutdown();
	ConnectionManager::getInstance()->shutdown();
	WebServerManager::getInstance()->shutdown();
	HttpManager::getInstance()->shutdown();
	MappingManager::getInstance()->close();
	BufferedSocket::waitShutdown();
	
	QueueManager::getInstance()->saveQueue(true);
	SettingsManager::getInstance()->save();
	FavoriteManager::getInstance()->shutdown();

	DHT::deleteInstance();
	DetectionManager::deleteInstance();

	PluginApiImpl::shutdown();
	PluginManager::deleteInstance();
	WebServerManager::deleteInstance();
	DebugManager::deleteInstance();
	MappingManager::deleteInstance();
	ConnectivityManager::deleteInstance();
	ADLSearchManager::deleteInstance();
	RawManager::deleteInstance();
	FinishedManager::deleteInstance();
	HttpManager::deleteInstance();
	UpdateManager::deleteInstance();
	ShareManager::deleteInstance();
	CryptoManager::deleteInstance();
	ThrottleManager::deleteInstance();
	DownloadManager::deleteInstance();
	UploadManager::deleteInstance();
	QueueManager::deleteInstance();
	ConnectionManager::deleteInstance();
	SearchManager::deleteInstance();
	FavoriteManager::deleteInstance();
	ClientManager::deleteInstance();
	HashManager::deleteInstance();
	LogManager::deleteInstance();
	TimerManager::deleteInstance();

	SettingsManager::deleteInstance();
	ResourceManager::deleteInstance();

#ifdef _WIN32
	::WSACleanup();
#endif
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
