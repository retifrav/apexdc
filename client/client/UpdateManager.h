/*
 * Copyright (C) 2006-2011 Crise, crise<at>mail.berlios.de
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

#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include "Speaker.h"
#include "Streams.h"
#include "HttpManager.h"
#include "TimerManager.h"
#include "Singleton.h"
#include "Util.h"
#include "version.h"

#define UPDATE_TEMP_DIR Util::getTempPath() + INST_NAME PATH_SEPARATOR_STR

namespace dcpp {

struct UpdateCheckData {
	UpdateCheckData() : isAutoUpdate(false), isForced(false) { }

	string title;
	string message;
	string version;
	string url;

	bool isAutoUpdate;
	string updateUrl;

	bool isForced;
	string forcedReason;
};

class UpdateListener
{
public:
	virtual ~UpdateListener() { }
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> UpdateAvailable;
	typedef X<1> UpdateFailed;
	typedef X<2> UpdateComplete;

	typedef X<3> AuthSuccess;
	typedef X<4> AuthFailure;

	typedef X<5> SettingUpdated;

	virtual void on(UpdateAvailable, const UpdateCheckData& /*ui*/) noexcept { }
	virtual void on(UpdateFailed, const string& /*line*/) noexcept { }
	virtual void on(UpdateComplete, const string& /*updater*/, const string& /*args*/) noexcept { }

	virtual void on(AuthSuccess, const string& /*title*/, const string& /*message*/) noexcept { }
	virtual void on(AuthFailure, const string& /*message*/) noexcept { }

	virtual void on(SettingUpdated, size_t /*key*/, const string& /*value*/) noexcept { }
};

class UpdateManager : public Singleton<UpdateManager>, public Speaker<UpdateListener>, private TimerManagerListener
{
public:
	UpdateManager() : updating(false), loggedIn(false), manualCheck(false) {
		TimerManager::getInstance()->addListener(this);
	}

	~UpdateManager() {
		TimerManager::getInstance()->removeListener(this);
	}

	void clientLogin(const string& url, const string& binaryHash);
	void setLogin(const string& username, const string& password);
	void resetLogin();

	void checkUpdates(const string& url, const string& binaryHash, bool bManual = false, int64_t delay = 5 * 60);
	void downloadUpdate(const string& url, const string& binaryPath);

	void updateGeoIP(const string& url, const string& binaryPath);

	void updateIP(string ipServer) {
		HttpManager::getInstance()->download(ipServer, [this](const HttpConnection* conn, OutputStream* os, Flags::MaskType flags) {
			updateIP(conn, static_cast<StringOutputStream*>(os)->getString(), flags);
		});
	}

	bool isUpdating() const { return updating; }
	bool isLoggedIn() const { return loggedIn; }

	static void signVersionFile(const string& file, const string& key, bool makeHeader = false);
	static bool verifyVersionData(const string& data, const ByteVector& singature);

	static bool applyUpdate(const string& sourcePath, const string& installPath);
	static void cleanTempFiles(const string& tmpPath = UPDATE_TEMP_DIR);

	static int64_t compareVersion(const string& otherVersion, const string& currentVersion = VERSIONSTRING);

private:
	static uint8_t publicKey[]; // see pubkey.h

	string currentBinary;
	string currentHash;
	string versionUrl;
	string updateTTH;

	bool updating;
	bool loggedIn;
	bool manualCheck;

	ByteVector versionSig;
	string versionCache;

	void updateDownload(const HttpConnection*, File* file, Flags::MaskType stFlags);
	void geoipDownload(const HttpConnection*, File* file, Flags::MaskType stFlags);

	void versionSignature(const HttpConnection* conn, const string& aData, Flags::MaskType stFlags);
	void versionCheck(const HttpConnection* conn, const string& versionInfo, Flags::MaskType stFlags);
	void parseVersion(const string& versionInfo, bool verified = true);

	void loginCheck(const HttpConnection* conn, const string& loginInfo, Flags::MaskType stFlagss);
	void failLogin(const string& line);

	void updateIP(const HttpConnection*, const string& ipData, Flags::MaskType stFlags);

	void on(TimerManagerListener::Minute, uint64_t /*aTick*/) noexcept;
};

} // namespace dcpp

#endif // UPDATE_MANAGER_H