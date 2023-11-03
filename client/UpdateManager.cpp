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

#include "stdinc.h"
#include "UpdateManager.h"

#include <boost/shared_array.hpp>
#include <boost/regex.hpp>

#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/pem.h>

#include "TimerManager.h"
#include "ResourceManager.h"
#include "SettingsManager.h"
#include "LogManager.h"
#include "FavoriteManager.h"
#include "ClientManager.h"

#include "ZipFile.h"
#include "SimpleXML.h"
#include "HashCalc.h"
#include "HttpConnection.h"

#include "pubkey.h"

#ifdef _WIN64
# define UPGRADE_TAG "UpdateURLx64"
#else
# define UPGRADE_TAG "UpdateURL"
#endif

namespace dcpp {

void UpdateManager::signVersionFile(const string& file, const string& key, bool makeHeader) {
	string versionData;
	unsigned int sig_len = 0;

	RSA* rsa = RSA_new();

	try {
		// Read All Data from files
		File versionFile(file, File::READ,  File::OPEN);
		versionData = versionFile.read();
		versionFile.close();

		FILE* f = fopen(key.c_str(), "r");
		PEM_read_RSAPrivateKey(f, &rsa, NULL, NULL);
		fclose(f);
	} catch(const FileException&) { return; }

	// Make SHA hash
	int res = -1;
	SHA_CTX sha_ctx = { 0 };
	uint8_t digest[SHA_DIGEST_LENGTH];

	res = SHA1_Init(&sha_ctx);
	if(res != 1)
		return;
	res = SHA1_Update(&sha_ctx, versionData.c_str(), versionData.size());
	if(res != 1)
		return;
	res = SHA1_Final(digest, &sha_ctx);
	if(res != 1)
		return;

	// sign hash
	boost::shared_array<uint8_t> sig = boost::shared_array<uint8_t>(new uint8_t[RSA_size(rsa)]);
	RSA_sign(NID_sha1, digest, sizeof(digest), sig.get(), &sig_len, rsa);

	if(sig_len > 0) {
		string c_key = Util::emptyString;

		if(makeHeader) {
			int buf_size = i2d_RSAPublicKey(rsa, 0);
			boost::shared_array<uint8_t> buf = boost::shared_array<uint8_t>(new uint8_t[buf_size]);

			{
				uint8_t* buf_ptr = buf.get();
				i2d_RSAPublicKey(rsa, &buf_ptr);
			}

			c_key = "// Automatically generated file, DO NOT EDIT!" NATIVE_NL NATIVE_NL;
			c_key += "#ifndef PUBKEY_H" NATIVE_NL "#define PUBKEY_H" NATIVE_NL NATIVE_NL;

			c_key += "uint8_t dcpp::UpdateManager::publicKey[] = { " NATIVE_NL "\t";
			for(int i = 0; i < buf_size; ++i) {
				c_key += (dcpp_fmt("0x%02X") % (unsigned int)buf[i]).str();
				if(i < buf_size - 1) {
					c_key += ", ";
					if((i+1) % 15 == 0) c_key += NATIVE_NL "\t";
				} else c_key += " " NATIVE_NL "};" NATIVE_NL NATIVE_NL;	
			}

			c_key += "#endif // PUBKEY_H" NATIVE_NL;
		}

		try {
			// Write signature file
			File outSig(file + ".sign", File::WRITE, File::TRUNCATE | File::CREATE);
			outSig.write(sig.get(), sig_len);
			outSig.close();

			if(!c_key.empty()) {
				// Write the public key header (openssl probably has something to generate similar file, but couldn't locate it)
				File pubKey(Util::getFilePath(file) + "pubkey.h", File::WRITE, File::TRUNCATE | File::CREATE);
				pubKey.write(c_key);
				pubKey.close();
			}
		} catch(const FileException&) { }
	}

	if(rsa) {
		RSA_free(rsa);
		rsa = NULL;
	}
}

bool UpdateManager::verifyVersionData(const string& data, const ByteVector& signature) {
	int res = -1;

	// Make SHA hash
	SHA_CTX sha_ctx = { 0 };
	uint8_t digest[SHA_DIGEST_LENGTH];

	res = SHA1_Init(&sha_ctx);
	if(res != 1)
		return false;
	res = SHA1_Update(&sha_ctx, data.c_str(), data.size());
	if(res != 1)
		return false;
	res = SHA1_Final(digest, &sha_ctx);
	if(res != 1)
		return false;

	// Extract Key
	const uint8_t* key = UpdateManager::publicKey;
	RSA* rsa = d2i_RSAPublicKey(NULL, &key, sizeof(UpdateManager::publicKey));
	if(rsa) {
		res = RSA_verify(NID_sha1, digest, sizeof(digest), &signature[0], signature.size(), rsa);

		RSA_free(rsa);
		rsa = NULL;
	} else return false;

	return (res == 1); 
}

bool UpdateManager::applyUpdate(const string& sourcePath, const string& installPath) {
	bool ret = true;
	FileFindIter end;
#ifdef _WIN32
	for(FileFindIter i(sourcePath + "*"); i != end; ++i) {
#else
	for(FileFindIter i(sourcePath); i != end; ++i) {
#endif
		string name = i->getFileName();
		if(name == "." || name == "..")
			continue;

		if(i->isLink() || name.empty())
			continue;

		if(!i->isDirectory()) {
			try {
				if(Util::fileExists(installPath + name))
					File::deleteFile(installPath + name);
				File::copyFile(sourcePath + name, installPath + name);
			} catch(Exception&) { return false; }
		} else {
			ret = UpdateManager::applyUpdate(sourcePath + name + PATH_SEPARATOR, installPath + name + PATH_SEPARATOR);
			if(!ret) break;
		}
	}

	return ret;
}

void UpdateManager::cleanTempFiles(const string& tmpPath) {
	FileFindIter end;
#ifdef _WIN32
	for(FileFindIter i(tmpPath + "*"); i != end; ++i) {
#else
	for(FileFindIter i(tmpPath); i != end; ++i) {
#endif
		string name = i->getFileName();
		if(name == "." || name == "..")
			continue;

		if(i->isLink() || name.empty())
			continue;

		if(i->isDirectory()) {
			UpdateManager::cleanTempFiles(tmpPath + name + PATH_SEPARATOR);
		} else File::deleteFile(tmpPath + name);
	}

	// Remove the empty dir
	File::removeDirectory(tmpPath);
}

/** if currentVersion > otherVersion -> positive, if currentVersion = otherVersion -> 0 and if currentVersion < otherVersion -> negative. */
int64_t UpdateManager::compareVersion(const string& otherVersion, const string& currentVersion /* = VERSIONSTRING*/) {
	auto otherSplit = otherVersion.find('-');
	auto other = StringTokenizer<string>(otherVersion.substr(0, otherSplit), '.').getTokens();

	auto currentSplit = currentVersion.find('-');
	auto current = StringTokenizer<string>(currentVersion.substr(0, currentSplit), '.').getTokens();

	// in case of invalid version numbers treat the versions as equal because comparing them is not relevant
	if(current.size() > 4 || other.size() > 4 || currentVersion.find(".x") != string::npos)
		return 0;

	// we are comparing against something that looks like old build id
	if(other.size() == 1 && Util::toInt64(other.front()) > 1000)
		return COMPATIBLE_VERSIONID - Util::toInt64(other.front());

	std::reverse(other.begin(), other.end());
	std::reverse(current.begin(), current.end());

	int64_t result = 0;
	while(!current.empty() || !other.empty()) {
		auto currentField = (current.empty() ? 0 : Util::toInt64(current.back()));
		auto otherField = (other.empty() ? 0 : Util::toInt64(other.back()));

		result = currentField - otherField;
		if(result != 0)
			return result;

		current.pop_back();
		other.pop_back();
	}

	// still here, do we have commit count
	if(currentSplit != string::npos || otherSplit != string::npos) {
		auto currentCommits = 0ll;
		if (currentSplit != string::npos)
			currentCommits = Util::toInt64(currentVersion.substr(currentSplit + 1));

		auto otherCommits = 0ll;
		if (otherSplit != string::npos)
			otherCommits = Util::toInt64(otherVersion.substr(otherSplit + 1));

		result = currentCommits - otherCommits;
	}

	return result;
}

void UpdateManager::clientLogin(const string& url, const string& binaryHash) {
	string username = SETTING(CLIENTAUTH_USER);
	string password = SETTING(CLIENTAUTH_PASSWORD);

	if (!username.empty() && !password.empty()) {
		StringMap data;
		data["username"] = username;
		data["password"] = password;

		data["binary_hash"] = currentHash = binaryHash;
		data["binary_type"] = CONFIGURATION_TYPE;
		data["binary_version"] = VERSIONSTRING;

		HttpManager::getInstance()->download(url, data, [this](const HttpConnection* conn, OutputStream* os, Flags::MaskType flags) {
			loginCheck(conn, static_cast<StringOutputStream*>(os)->getString(), flags);
		});
	}
}

void UpdateManager::setLogin(const string& username, const string& password) {
	SettingsManager::getInstance()->set(SettingsManager::CLIENTAUTH_USER, username);

	// This is stupid but marginally better than plain text (the connection should be https anyways)
	ByteVector passVec(password.begin(), password.end());
	SettingsManager::getInstance()->set(SettingsManager::CLIENTAUTH_PASSWORD, "v4:" + Encoder::toBase32(&passVec[0], passVec.size()));
}

void UpdateManager::resetLogin() {
	SettingsManager::getInstance()->unset(SettingsManager::CLIENTAUTH_USER);
	SettingsManager::getInstance()->unset(SettingsManager::CLIENTAUTH_PASSWORD);
	SettingsManager::getInstance()->set(SettingsManager::LAST_AUTH_TIME, 0);
}

void UpdateManager::checkUpdates(const string& url, const string& binaryHash, bool manual, int64_t delay) {
	if(manual && manualCheck)
		return;

	currentHash = binaryHash;
	manualCheck = manual;
	time_t curTime = GET_TIME();

	if((manualCheck && versionCache.empty()) || (curTime - SETTING(LAST_UPDATE_NOTICE)) > delay || SETTING(LAST_UPDATE_NOTICE) > curTime) {
		versionUrl = url;
		versionSig.clear();

		HttpManager::getInstance()->download(url + ".sign", [this](const HttpConnection* conn, OutputStream* os, Flags::MaskType flags) {
			versionSignature(conn, static_cast<StringOutputStream*>(os)->getString(), flags);
		});
	} else if(!versionCache.empty()) {
		parseVersion(versionCache);
	}
}

void UpdateManager::downloadUpdate(const string& url, const string& binaryPath) {
	if(updating)
		return;

	updating = true;
	currentBinary = binaryPath;

	HttpManager::getInstance()->download(url, UPDATE_TEMP_DIR "Apex_Update.zip", [this](const HttpConnection* conn, OutputStream* os, Flags::MaskType flags) {
		updateDownload(conn, static_cast<File*>(os), flags);
	});
}

void UpdateManager::updateGeoIP(const string& url, const string& binaryPath) {
	if(updating)
		return;

	updating = true;
	currentBinary = binaryPath;

	HttpManager::getInstance()->download(url, UPDATE_TEMP_DIR "GeoIP_Update.zip", [this](const HttpConnection* conn, OutputStream* os, Flags::MaskType flags) {
		geoipDownload(conn, static_cast<File*>(os), flags);
	});
}

void UpdateManager::updateDownload(const HttpConnection* conn, File* file, Flags::MaskType stFlags) {
	bool success = ((stFlags & HttpManager::CONN_COMPLETE) == HttpManager::CONN_COMPLETE);
	if(success && Util::fileExists(UPDATE_TEMP_DIR "Apex_Update.zip")) {
		file->close();

		// Check integrity
		if(TTH(UPDATE_TEMP_DIR "Apex_Update.zip") != updateTTH) {
			updating = false;
			File::deleteFile(UPDATE_TEMP_DIR "Apex_Update.zip");
			fire(UpdateListener::UpdateFailed(), "File integrity check failed");
			return;
		}

		// Unzip the update
		try {
			ZipFile zip;
			zip.Open(UPDATE_TEMP_DIR "Apex_Update.zip");

			string srcPath = UPDATE_TEMP_DIR + updateTTH + PATH_SEPARATOR;
			string dstPath = Util::getFilePath(currentBinary);
			string srcBinary = srcPath + Util::getFileName(currentBinary);

#ifdef _WIN32
			if(srcPath[srcPath.size() - 1] == PATH_SEPARATOR)
				srcPath.insert(srcPath.size() - 1, "\\");

			if(dstPath[dstPath.size() - 1] == PATH_SEPARATOR)
				dstPath.insert(dstPath.size() - 1, "\\");
#endif

			if(zip.GoToFirstFile()) {
				do {
					zip.OpenCurrentFile();
					if (zip.GetCurrentFileName().find(Util::getFileExt(currentBinary)) != string::npos) {
						zip.ReadCurrentFile(srcBinary);
					} else zip.ReadCurrentFile(srcPath);
					zip.CloseCurrentFile();
				} while(zip.GoToNextFile());
			}

			zip.Close();

			File::deleteFile(UPDATE_TEMP_DIR "Apex_Update.zip");
			fire(UpdateListener::UpdateComplete(), srcBinary, "/update \"" + srcPath + "\" \"" + dstPath + "\"");
		} catch(ZipFileException& e) {
			updating = false;
			File::deleteFile(UPDATE_TEMP_DIR "Apex_Update.zip");
			fire(UpdateListener::UpdateFailed(), e.getError());
		}
	} else {
		updating = false;
		File::deleteFile(UPDATE_TEMP_DIR "Apex_Update.zip");
		fire(UpdateListener::UpdateFailed(), str(F_("%1% (%2%)") % conn->getStatus() % conn->getUrl()));
	}
}

void UpdateManager::geoipDownload(const HttpConnection* conn, File* file, Flags::MaskType stFlags) {
	bool success = ((stFlags & HttpManager::CONN_COMPLETE) == HttpManager::CONN_COMPLETE);
	if(success && Util::fileExists(UPDATE_TEMP_DIR "GeoIP_Update.zip")) {
		file->close();

		// Unzip the update
		try {
			ZipFile zip;
			zip.Open(UPDATE_TEMP_DIR "GeoIP_Update.zip");

			string srcPath = UPDATE_TEMP_DIR + TTH(UPDATE_TEMP_DIR "GeoIP_Update.zip") + PATH_SEPARATOR;
			string dstPath = Util::getFilePath(currentBinary);

#ifdef _WIN32
			if(srcPath[srcPath.size() - 1] == PATH_SEPARATOR)
				srcPath.insert(srcPath.size() - 1, "\\");

			if(dstPath[dstPath.size() - 1] == PATH_SEPARATOR)
				dstPath.insert(dstPath.size() - 1, "\\");
#endif

			if(zip.GoToFirstFile()) {
				do {
					zip.OpenCurrentFile();
					zip.ReadCurrentFile(srcPath);
					zip.CloseCurrentFile();
				} while(zip.GoToNextFile());
			}

			zip.Close();

			File::deleteFile(UPDATE_TEMP_DIR "GeoIP_Update.zip");
			fire(UpdateListener::UpdateComplete(), currentBinary, "/update \"" + srcPath + "\" \"" + dstPath + "\"");
		} catch(ZipFileException& e) {
			updating = false;
			File::deleteFile(UPDATE_TEMP_DIR "GeoIP_Update.zip");
			fire(UpdateListener::UpdateFailed(), e.getError());
		}
	} else {
		updating = false;
		File::deleteFile(UPDATE_TEMP_DIR "GeoIP_Update.zip");
		fire(UpdateListener::UpdateFailed(), str(F_("%1% (%2%)") % conn->getStatus() % conn->getUrl()));
	}
}

void UpdateManager::versionSignature(const HttpConnection* conn, const string& aData, Flags::MaskType stFlags) {
	if((stFlags & HttpManager::CONN_FAILED) == HttpManager::CONN_FAILED) {
		LogManager::getInstance()->message("Update Check: could not download digital signature (" + conn->getStatus() + ")", LogManager::LOG_WARNING);
	} else {
		versionSig.assign(aData.begin(), aData.end());
	}

	StringMap data;
	data["uniqid"] = TIGER(ClientManager::getInstance()->getMyCID().toBase32());
	data["binary_hash"] = currentHash;
	data["manual_check"] = manualCheck ? "1" : "0";

	HttpManager::getInstance()->download(versionUrl, data, [this](const HttpConnection* conn, OutputStream* os, Flags::MaskType flags) {
		versionCheck(conn, static_cast<StringOutputStream*>(os)->getString(), flags);
	});
}

void UpdateManager::versionCheck(const HttpConnection* conn, const string& versionInfo, Flags::MaskType stFlags) {
	if((stFlags & HttpManager::CONN_FAILED) == HttpManager::CONN_FAILED || conn->getMimeType().find("/xml") == string::npos) {
		LogManager::getInstance()->message("Update Check: could not connect to the update server (" + conn->getStatus() + ")", LogManager::LOG_WARNING);
		SettingsManager::getInstance()->set(SettingsManager::LAST_UPDATE_NOTICE, GET_TIME() - (60 * 60 * 71));
		manualCheck = false;
		return;
	}

	bool verified = UpdateManager::verifyVersionData(versionInfo, versionSig);
	if(!verified) {
		fire(UpdateListener::AuthFailure(), "Update Check: could not verify origin of update information");
	} else versionCache = versionInfo;

	SettingsManager::getInstance()->set(SettingsManager::LAST_UPDATE_NOTICE, GET_TIME());

	parseVersion(versionInfo, verified);
}

void UpdateManager::parseVersion(const string& versionInfo, bool verified) {
	try {
		SimpleXML xml;
		xml.fromXML(versionInfo);
		xml.stepIn();

		UpdateCheckData ui;

		if(xml.findChild(UPGRADE_TAG)) {
			ui.updateUrl = xml.getChildData();
			ui.isAutoUpdate = (verified && xml.getChildAttrib("Enabled", "1") == "1");
			updateTTH = xml.getChildAttrib("TTH");
		}
		xml.resetCurrentChild();

		if(xml.findChild("URL"))
			ui.url = xml.getChildData();
		xml.resetCurrentChild();

		if(verified && xml.findChild("VeryOldVersion")) {
			if(UpdateManager::compareVersion(xml.getChildData()) <= 0) {
				ui.isForced = true;
				ui.forcedReason = xml.getChildAttrib("Message", "Your version of ApexDC++ contains a serious bug that affects all users of the DC network or the security of your computer.");
			}
		}
		xml.resetCurrentChild();

		if(verified && !ui.isForced && xml.findChild("BadVersion")) {
			xml.stepIn();
			while(xml.findChild("BadVersion")) {
				if(UpdateManager::compareVersion(xml.getChildData()) == 0) {
					ui.isForced = true;
					ui.forcedReason = xml.getChildAttrib("Message", "Your version of ApexDC++ contains a serious bug that affects all users of the DC network or the security of your computer.");
				}
			}
			xml.stepOut();
		}
		xml.resetCurrentChild();

		if(verified && xml.findChild("Blacklist")) {
			xml.stepIn();
			while(xml.findChild("Blacklisted")) {
				const string& domain = xml.getChildAttrib("Domain");
				if(domain.empty())
					continue;
				const string& reason = xml.getChildAttrib("Reason");
				if(reason.empty())
					continue;
				FavoriteManager::getInstance()->addBlacklist(domain, reason);
			}
			xml.stepOut();
		}
		xml.resetCurrentChild();

		if(xml.findChild("Version")) {
			ui.version = xml.getChildData();
			xml.resetCurrentChild();

			if(UpdateManager::compareVersion(ui.version) < 0 || manualCheck) {
				if(xml.findChild("Title"))
					ui.title = xml.getChildData();
				xml.resetCurrentChild();

				if(xml.findChild("Message")) {
					ui.message = xml.getChildData();
					fire(UpdateListener::UpdateAvailable(), ui);
				}
				xml.resetCurrentChild();
			}
		}

		xml.stepOut();
	} catch (const Exception& e) {
		LogManager::getInstance()->message("Update Check: could not parse update information (" + e.getError() + ")", LogManager::LOG_WARNING);
	}

	manualCheck = false;
}

void UpdateManager::loginCheck(const HttpConnection* conn, const string& loginInfo, Flags::MaskType stFlags) {
	// Not interested in failures for now
	if((stFlags & HttpManager::CONN_FAILED) == HttpManager::CONN_FAILED || conn->getMimeType().find("/xml") == string::npos) {
		failLogin(conn->getStatus());
		return;
	}

	try {
		SimpleXML xml;
		xml.fromXML(loginInfo);
		xml.stepIn();

		if(xml.findChild("ResultCode")) {
			if(xml.getChildData().compare(APP_AUTH_KEY) == 0) {
				xml.resetCurrentChild();

				// Here we are login correct
				string message, title;
				loggedIn = true;

				SettingsManager::getInstance()->set(SettingsManager::LAST_AUTH_TIME, GET_TIME());

				// Let's see if we have something to say to our beloved testers
				if(xml.findChild("Message") && (Util::toInt64(SETTING(CONFIG_VERSION)) < COMPATIBLE_VERSIONID || xml.getBoolChildAttrib("Force"))) {
					message = xml.getChildData();
					title = xml.getChildAttrib("Title", "News Flash!");
				}
				xml.resetCurrentChild();

				fire(UpdateListener::AuthSuccess(), title, message);
			} else {
				failLogin(xml.getChildData());
			}
		} else {
			failLogin("There was an error while parsing server response");
		}

		xml.stepOut();
	} catch (const Exception& e) {
		failLogin(e.getError());
	}
}

void UpdateManager::failLogin(const string& line) {
	time_t curTime = GET_TIME();
	if ((curTime - SETTING(LAST_AUTH_TIME)) > (60 * 60 * 24 * 3) || SETTING(LAST_AUTH_TIME) > curTime) {
		fire(UpdateListener::AuthFailure(), line);
	} else {
		loggedIn = true;
		fire(UpdateListener::AuthSuccess(), Util::emptyString, Util::emptyString);
	}
}

void UpdateManager::updateIP(const HttpConnection*, const string& ipData, Flags::MaskType stFlags) {
	if((stFlags & HttpManager::CONN_FAILED) == HttpManager::CONN_FAILED) {
		// Fire this to indicate that the process completed without results
		fire(UpdateListener::SettingUpdated(), SettingsManager::EXTERNAL_IP, Util::emptyString);
		return;
	}

	try {
		const string pattern = "\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b";
		const boost::regex reg(pattern, boost::regex_constants::icase);
		boost::match_results<string::const_iterator> results;
		// RSX++ workaround for msvc std lib problems
		string::const_iterator start = ipData.begin();
		string::const_iterator end = ipData.end();

		if(boost::regex_search(start, end, results, reg, boost::match_default)) {
			if(!results.empty()) {
				const string& ip = results.str(0);
				SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, ip);
				fire(UpdateListener::SettingUpdated(), SettingsManager::EXTERNAL_IP, ip);
			}
		}
	} catch(...) { }
}

void UpdateManager::on(TimerManagerListener::Minute, uint64_t /*aTick*/) noexcept {
	time_t curTime = GET_TIME();
	if((curTime - SETTING(LAST_UPDATE_NOTICE)) > (60 * 60 * 24 * 3) || SETTING(LAST_UPDATE_NOTICE) > curTime)
		checkUpdates(VERSION_URL, currentHash, false);
}

} // namespace dcpp
