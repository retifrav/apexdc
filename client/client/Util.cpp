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
#include "Util.h"

#ifdef _WIN32

#include "w.h"
#include "shlobj.h"

#endif

#include <boost/algorithm/string/trim.hpp>

#include "CID.h"
#include "FastAlloc.h"
#include "SettingsManager.h"
#include "ResourceManager.h"
#include "SettingsManager.h"
#include "version.h"
#include "File.h"
#include "SimpleXML.h"
#include "RegEx.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <cctype>
#include <cstring>
#endif
#include <clocale>

namespace dcpp {

using std::make_pair;

#ifdef NDEBUG
FastCriticalSection FastAllocBase::cs;
#endif

time_t Util::startTime = time(NULL);
string Util::emptyString;
wstring Util::emptyStringW;
tstring Util::emptyStringT;

bool Util::away = false;
string Util::awayMsg;
time_t Util::awayTime;

Util::CountryList Util::countries;
StringPairList Util::countryNames;

string Util::paths[Util::PATH_LAST];

bool Util::localMode = true;

static void sgenrand(unsigned long seed);

extern "C" void bz_internal_error(int errcode) { 
	dcdebug("bzip2 internal error: %d\n", errcode); 
}

#if (_MSC_VER >= 1400)
void WINAPI invalidParameterHandler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) {
	//do nothing, this exist because vs2k5 crt needs it not to crash on errors.
}
#endif

#ifdef _WIN32

typedef HRESULT (WINAPI* _SHGetKnownFolderPath)(GUID& rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);

static string getDownloadsPath(const string& def) {
	// Try Vista downloads path
	static _SHGetKnownFolderPath getKnownFolderPath = 0;
	static HINSTANCE shell32 = NULL;

	if(!shell32) {
	    shell32 = ::LoadLibrary(_T("Shell32.dll"));
	    if(shell32)
	    {
	    	getKnownFolderPath = (_SHGetKnownFolderPath)::GetProcAddress(shell32, "SHGetKnownFolderPath");

	    	if(getKnownFolderPath) {
	    		 PWSTR path = NULL;
	             // Defined in KnownFolders.h.
	             static GUID downloads = {0x374de290, 0x123f, 0x4565, {0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b}};
	    		 if(getKnownFolderPath(downloads, 0, NULL, &path) == S_OK) {
	    			 string ret = Text::fromT(path) + "\\";
	    			 ::CoTaskMemFree(path);
	    			 return ret;
	    		 }
	    	}
	    }
	}

	return def + APPNAME " Downloads\\";
}

#endif

void Util::initialize(PathsMap pathOverrides) {
	Text::initialize();

	sgenrand((unsigned long)time(NULL));

#if (_MSC_VER >= 1400)
	_set_invalid_parameter_handler(reinterpret_cast<_invalid_parameter_handler>(invalidParameterHandler));
#endif

#ifdef _WIN32
	TCHAR buf[MAX_PATH+1] = { 0 };
	::GetModuleFileName(NULL, buf, MAX_PATH);

	string exePath = Util::getFilePath(Text::fromT(buf));

	// Global config path is ApexDC++ executable path...
	paths[PATH_GLOBAL_CONFIG] = exePath;

	paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG] + "Settings\\";

	loadBootConfig();

	if(!File::isAbsolute(paths[PATH_USER_CONFIG])) {
		paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG] + paths[PATH_USER_CONFIG];
	}

	paths[PATH_USER_CONFIG] = validateFileName(paths[PATH_USER_CONFIG]);

	if(localMode) {
		paths[PATH_USER_LOCAL] = paths[PATH_USER_CONFIG];

		paths[PATH_DOWNLOADS] = paths[PATH_USER_CONFIG] + "Downloads\\";
	} else {
		if(::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf) == S_OK) {
			paths[PATH_USER_CONFIG] = Text::fromT(buf) + "\\" APPNAME "\\";
		}

		paths[PATH_DOWNLOADS] = getDownloadsPath(::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, buf) == S_OK ? Text::fromT(buf) + "\\" : paths[PATH_GLOBAL_CONFIG]);

		paths[PATH_USER_LOCAL] = ::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf) == S_OK ? Text::fromT(buf) + "\\" APPNAME "\\" : paths[PATH_USER_CONFIG];
	}

	paths[PATH_RESOURCES] = exePath;
	paths[PATH_LOCALE] = exePath;

#else
	paths[PATH_GLOBAL_CONFIG] = "/etc/";
	const char* home_ = getenv("HOME");
	string home = home_ ? Text::toUtf8(home_) : "/tmp/";

	paths[PATH_USER_CONFIG] = home + "/.apexdc++/";

	loadBootConfig();

	if(!File::isAbsolute(paths[PATH_USER_CONFIG])) {
		paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG] + paths[PATH_USER_CONFIG];
	}

	paths[PATH_USER_CONFIG] = validateFileName(paths[PATH_USER_CONFIG]);

	if(localMode) {
		// @todo implement...
	}

	paths[PATH_USER_LOCAL] = paths[PATH_USER_CONFIG];
	paths[PATH_RESOURCES] = "/usr/share/";
	paths[PATH_LOCALE] = paths[PATH_RESOURCES] + "locale/";
	paths[PATH_DOWNLOADS] = home + "/Downloads/";
#endif

	paths[PATH_FILE_LISTS] = paths[PATH_USER_LOCAL] + "FileLists" PATH_SEPARATOR_STR;
	paths[PATH_HUB_LISTS] = paths[PATH_USER_LOCAL] + "HubLists" PATH_SEPARATOR_STR;
	paths[PATH_NOTEPAD] = paths[PATH_USER_CONFIG] + "Notepad.txt";
	paths[PATH_EMOPACKS] = paths[PATH_RESOURCES] + "EmoPacks" PATH_SEPARATOR_STR;

	// Override core generated paths
	for (PathsMap::const_iterator it = pathOverrides.begin(); it != pathOverrides.end(); ++it)
	{
		if (!it->second.empty())
			paths[it->first] = it->second;
	}
	
	File::ensureDirectory(paths[PATH_USER_CONFIG]);
	File::ensureDirectory(paths[PATH_USER_LOCAL]);
	
	try {
		// This product includes GeoIP data created by MaxMind, available from http://maxmind.com/
		// Updates at http://www.maxmind.com/app/geoip_country
		string file = getPath(PATH_RESOURCES) + "GeoIpCountryWhois.csv";
		string data = File(file, File::READ, File::OPEN).read();

		const char* start = data.c_str();
		string::size_type linestart = 0, lineend, pos, pos2;
		auto last = countries.end();
		uint32_t startIP = 0, endIP = 0, endIPprev = 0;

		countryNames.push_back(make_pair(STRING(UNKNOWN), "??"));
		auto addCountry = [](const string& countryName, const string& countryCode) -> size_t {
			auto begin = countryNames.cbegin(), end = countryNames.cend();
			auto countryPair = make_pair(countryName, countryCode);
			auto pos = std::find(begin, end, countryPair);
			if(pos != end)
				return pos - begin;
			countryNames.push_back(countryPair);
			return countryNames.size() - 1;
		};

		while(true) {
			pos = data.find(',', linestart);
			if(pos == string::npos) break;
			pos = data.find(',', pos + 1);
			if(pos == string::npos) break;
			startIP = toUInt32(start + pos + 2) - 1;

			pos = data.find(',', pos + 1);
			if(pos == string::npos) break;
			endIP = toUInt32(start + pos + 2);

			pos = data.find(',', pos + 1);
			if(pos == string::npos) break;

			pos2 = data.find(',', pos + 1);
			if(pos2 == string::npos) break;

			lineend = data.find('\n', pos);
			if(lineend == string::npos) break;

			if(startIP != endIPprev)
				last = countries.insert(last, make_pair(startIP, 0));
			pos += 2;
			pos2 += 2;
			last = countries.insert(last, make_pair(endIP, addCountry(data.substr(pos2, lineend - 1 - pos2), data.substr(pos, 2))));

			endIPprev = endIP;
			linestart = lineend + 1;
		}
	} catch(const FileException&) {
	}
}

void Util::migrate(const string& file) noexcept {
	if(localMode) {
		return;
	}

	if(File::getSize(file) != -1) {
		return;
	}

	string fname = getFileName(file);
	string old = paths[PATH_GLOBAL_CONFIG] + "Settings\\" + fname;
	if(File::getSize(old) == -1) {
		return;
	}

	try {
		File::renameFile(old, file);
	} catch(const FileException&) {
		// TODO: report to user?
	}
}

void Util::loadBootConfig() {
	// Load boot settings
	try {
		SimpleXML boot;
		boot.fromXML(File(getPath(PATH_GLOBAL_CONFIG) + "dcppboot.xml", File::READ, File::OPEN).read());
		boot.stepIn();

		if(boot.findChild("LocalMode")) {
			localMode = boot.getChildData() != "0";
		}

		boot.resetCurrentChild();
		
		if(boot.findChild("ConfigPath")) {
			StringMap params;
#ifdef _WIN32
			// @todo load environment variables instead? would make it more useful on *nix
			TCHAR path[MAX_PATH];

			params["APPDATA"] = Text::fromT((::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path), path));
			params["PERSONAL"] = Text::fromT((::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path), path));
#endif
			paths[PATH_USER_CONFIG] = Util::formatParams(boot.getChildData(), params, false);
		}
	} catch(const Exception& ) {
		// Unable to load boot settings...
	}
}

#ifdef _WIN32
static const char badChars[] = { 
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, '<', '>', '/', '"', '|', '?', '*', 0
};
#else

static const char badChars[] = { 
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, '<', '>', '\\', '"', '|', '?', '*', 0
};
#endif

/**
 * Replaces all strange characters in a file with '_'
 * @todo Check for invalid names such as nul and aux...
 */
string Util::validateFileName(string tmp) {
	string::size_type i = 0;

	// First, eliminate forbidden chars
	while( (i = tmp.find_first_of(badChars, i)) != string::npos) {
		tmp[i] = '_';
		i++;
	}

	// Then, eliminate all ':' that are not the second letter ("c:\...")
	i = 0;
	while( (i = tmp.find(':', i)) != string::npos) {
		if(i == 1) {
			i++;
			continue;
		}
		tmp[i] = '_';	
		i++;
	}

	// Remove the .\ that doesn't serve any purpose
	i = 0;
	while( (i = tmp.find("\\.\\", i)) != string::npos) {
		tmp.erase(i+1, 2);
	}
	i = 0;
	while( (i = tmp.find("/./", i)) != string::npos) {
		tmp.erase(i+1, 2);
	}

	// Remove any double \\ that are not at the beginning of the path...
	i = 1;
	while( (i = tmp.find("\\\\", i)) != string::npos) {
		tmp.erase(i+1, 1);
	}
	i = 1;
	while( (i = tmp.find("//", i)) != string::npos) {
		tmp.erase(i+1, 1);
	}

	// And last, but not least, the infamous ..\! ...
	i = 0;
	while( ((i = tmp.find("\\..\\", i)) != string::npos) ) {
		tmp[i + 1] = '_';
		tmp[i + 2] = '_';
		tmp[i + 3] = '_';
		i += 2;
	}
	i = 0;
	while( ((i = tmp.find("/../", i)) != string::npos) ) {
		tmp[i + 1] = '_';
		tmp[i + 2] = '_';
		tmp[i + 3] = '_';
		i += 2;
	}

	// Dots at the end of path names aren't popular
	i = 0;
	while( ((i = tmp.find(".\\", i)) != string::npos) ) {
		if(i != 0)
			tmp[i] = '_';
		i += 1;
	}
	i = 0;
	while( ((i = tmp.find("./", i)) != string::npos) ) {
		if(i != 0)
			tmp[i] = '_';
		i += 1;
	}


	return tmp;
}

string Util::cleanPathChars(string aNick) {
	string::size_type i = 0;

	while( (i = aNick.find_first_of("/.\\", i)) != string::npos) {
		aNick[i] = '_';
	}
	return aNick;
}

string Util::getShortTimeString(time_t t) {
	char buf[255];
	tm* _tm = localtime(&t);
	if(_tm == NULL) {
		strcpy(buf, "xx:xx");
	} else {
		strftime(buf, 254, SETTING(TIME_STAMPS_FORMAT).c_str(), _tm);
	}
	return Text::toUtf8(buf);
}

string Util::addBrackets(const string& s) {
	return '<' + s + '>';
}

void Util::sanitizeUrl(string& url) {
	boost::algorithm::trim_if(url, boost::is_space() || boost::is_any_of("<>\""));
}

/**
 * Decodes a URL the best it can...
 * Default ports:
 * http:// -> port 80
 * https:// -> port 443
 * dchub:// -> port 411
 */
void Util::decodeUrl(const string& url, string& protocol, string& host, string& port, string& path, string& query, string& fragment, bool& isSecure) {
	auto fragmentEnd = url.size();
	auto fragmentStart = url.rfind('#');

	size_t queryEnd;
	if(fragmentStart == string::npos) {
		queryEnd = fragmentStart = fragmentEnd;
	} else {
		queryEnd = fragmentStart;
		fragmentStart++;
	}

	auto queryStart = url.rfind('?', queryEnd);
	size_t fileEnd;

	if(queryStart == string::npos) {
		fileEnd = queryStart = queryEnd;
	} else {
		fileEnd = queryStart;
		queryStart++;
	}

	auto protoStart = 0;
	auto protoEnd = url.find("://", protoStart);

	auto authorityStart = protoEnd == string::npos ? protoStart : protoEnd + 3;
	auto authorityEnd = url.find_first_of("/#?", authorityStart);

	size_t fileStart;
	if(authorityEnd == string::npos) {
		authorityEnd = fileStart = fileEnd;
	} else {
		fileStart = authorityEnd;
	}

	isSecure = false;
	protocol = (protoEnd == string::npos ? Util::emptyString : url.substr(protoStart, protoEnd - protoStart));

	if(authorityEnd > authorityStart) {
		dcdebug("x");
		size_t portStart = string::npos;
		if(url[authorityStart] == '[') {
			// IPv6?
			auto hostEnd = url.find(']');
			if(hostEnd == string::npos) {
				return;
			}

			host = url.substr(authorityStart + 1, hostEnd - authorityStart - 1);
			if(hostEnd + 1 < url.size() && url[hostEnd + 1] == ':') {
				portStart = hostEnd + 2;
			}
		} else {
			size_t hostEnd;
			portStart = url.find(':', authorityStart);
			if(portStart != string::npos && portStart > authorityEnd) {
				portStart = string::npos;
			}

			if(portStart == string::npos) {
				hostEnd = authorityEnd;
			} else {
				hostEnd = portStart;
				portStart++;
			}

			dcdebug("h");
			host = url.substr(authorityStart, hostEnd - authorityStart);
		}

		if(portStart == string::npos) {
			if(protocol == "http") {
				port = "80";
			} else if(protocol == "https") {
				isSecure = true;
				port = "443";
			} else if(protocol == "adcs") {
				isSecure = true;
			} else if(protocol == "dchub" || protocol == "nmdcs" || protocol.empty()) {
				isSecure = (protocol == "nmdcs");
				port = "411";
			}
		} else {
			port = url.substr(portStart, authorityEnd - portStart);

			if(protocol == "https") {
				isSecure = true;
			} else if(protocol == "adcs") {
				isSecure = true;
			}
		}
	}

	path = url.substr(fileStart, fileEnd - fileStart);
	query = url.substr(queryStart, queryEnd - queryStart);
	fragment = url.substr(fragmentStart, fragmentEnd - fragmentStart);

	dcdebug("Util::decodeUrl(): protocol = '%s', host = '%s', port = %s, path = '%s', query = '%s', fragment = '%s'", protocol.c_str(), host.c_str(), port.c_str(), path.c_str(), query.c_str(), fragment.c_str());
}

map<string, string> Util::decodeQuery(const string& query) {
	map<string, string> ret;
	size_t start = 0;
	while(start < query.size()) {
		auto eq = query.find('=', start);
		if(eq == string::npos) {
			break;
		}

		auto param = eq + 1;
		auto end = query.find('&', param);

		if(end == string::npos) {
			end = query.size();
		}

		if(eq > start && end > param) {
			ret[query.substr(start, eq-start)] = query.substr(param, end - param);
		}

		start = end + 1;
	}

	return ret;
}

void Util::setAway(bool aAway) {
	away = aAway;

	SettingsManager::getInstance()->set(SettingsManager::AWAY, aAway);

	if (away)
		awayTime = time(NULL);
}

string Util::getAwayMessage(StringMap& params) { 
	time_t currentTime;
	time(&currentTime);
	int currentHour = localtime(&currentTime)->tm_hour;

	params["idleTI"] = Text::fromT(formatSeconds(time(NULL) - awayTime));

	if(BOOLSETTING(AWAY_TIME_THROTTLE) && ((SETTING(AWAY_START) < SETTING(AWAY_END) && 
			currentHour >= SETTING(AWAY_START) && currentHour < SETTING(AWAY_END)) ||
		(SETTING(AWAY_START) > SETTING(AWAY_END) &&
			(currentHour >= SETTING(AWAY_START) || currentHour < SETTING(AWAY_END))))) {
		return formatParams((awayMsg.empty() ? SETTING(SECONDARY_AWAY_MESSAGE) : awayMsg), params, false, awayTime);
	} else {
		return formatParams((awayMsg.empty() ? SETTING(DEFAULT_AWAY_MESSAGE) : awayMsg), params, false, awayTime);
	}
}

string Util::formatBytes(int64_t aBytes) {
	char buf[64];
	if(aBytes < 1024) {
		snprintf(buf, sizeof(buf), "%d %s", (int)(aBytes&0xffffffff), CSTRING(B));
	} else if(aBytes < 1048576) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes/(1024.0), CSTRING(KB));
	} else if(aBytes < 1073741824) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes/(1048576.0), CSTRING(MB));
	} else if(aBytes < (int64_t)1099511627776) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes/(1073741824.0), CSTRING(GB));
	} else if(aBytes < (int64_t)1125899906842624) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes/(1099511627776.0), CSTRING(TB));
	} else if(aBytes < (int64_t)1152921504606846976)  {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes/(1125899906842624.0), CSTRING(PB));
	} else {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes/(1152921504606846976.0), CSTRING(EB));
	}

	return buf;
}

wstring Util::formatBytesW(int64_t aBytes) {
	wchar_t buf[64];
	const auto maxLen = sizeof buf / sizeof *buf;
	if(aBytes < 1024) {
		swprintf(buf, maxLen, L"%d %s", (int)(aBytes&0xffffffff), CWSTRING(B));
	} else if(aBytes < 1048576) {
		swprintf(buf, maxLen, L"%.02f %s", (double)aBytes/(1024.0), CWSTRING(KB));
	} else if(aBytes < 1073741824) {
		swprintf(buf, maxLen, L"%.02f %s", (double)aBytes/(1048576.0), CWSTRING(MB));
	} else if(aBytes < (int64_t)1099511627776) {
		swprintf(buf, maxLen, L"%.02f %s", (double)aBytes/(1073741824.0), CWSTRING(GB));
	} else if(aBytes < (int64_t)1125899906842624) {
		swprintf(buf, maxLen, L"%.02f %s", (double)aBytes/(1099511627776.0), CWSTRING(TB));
	} else if(aBytes < (int64_t)1152921504606846976)  {
		swprintf(buf, maxLen, L"%.02f %s", (double)aBytes/(1125899906842624.0), CWSTRING(PB));
	} else {
		swprintf(buf, maxLen, L"%.02f %s", (double)aBytes/(1152921504606846976.0), CWSTRING(EB));
	}

	return buf;
}

string Util::formatExactSize(int64_t aBytes) {
#ifdef _WIN32	
	TCHAR tbuf[128];
	TCHAR number[64];
	NUMBERFMT nf;
	_sntprintf(number, 64, _T("%I64d"), aBytes);
	TCHAR Dummy[16];
	TCHAR sep[2] = _T(",");
    
	/*No need to read these values from the system because they are not
	used to format the exact size*/
	nf.NumDigits = 0;
	nf.LeadingZero = 0;
	nf.NegativeOrder = 0;
	nf.lpDecimalSep = sep;

	GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, Dummy, 16 );
	nf.Grouping = Util::toInt(Text::fromT(Dummy));
	GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, Dummy, 16 );
	nf.lpThousandSep = Dummy;

	GetNumberFormat(LOCALE_USER_DEFAULT, 0, number, &nf, tbuf, sizeof(tbuf)/sizeof(tbuf[0]));

	char buf[128];
	_snprintf(buf, sizeof(buf), "%s %s", Text::fromT(tbuf).c_str(), CSTRING(B));
	return buf;
#else
	char buf[128];
	snprintf(buf, sizeof(buf), "%'lld %s", (long long int)aBytes, CSTRING(B));
	return string(buf);
#endif
}

string Util::getLocalIp() {
	if(!SettingsManager::getInstance()->isDefault(SettingsManager::BIND_ADDRESS)) {
		return SETTING(BIND_ADDRESS);
	}

	string tmp;
	
	char buf[256];
	gethostname(buf, 255);
	hostent* he = gethostbyname(buf);
	if(he == NULL || he->h_addr_list[0] == 0)
		return Util::emptyString;
	sockaddr_in dest;
	int i = 0;
	
	// We take the first ip as default, but if we can find a better one, use it instead...
	memcpy(&(dest.sin_addr), he->h_addr_list[i++], he->h_length);
	tmp = inet_ntoa(dest.sin_addr);
	if(Util::isPrivateIp(tmp) || strncmp(tmp.c_str(), "169", 3) == 0) {
		while(he->h_addr_list[i]) {
			memcpy(&(dest.sin_addr), he->h_addr_list[i], he->h_length);
			string tmp2 = inet_ntoa(dest.sin_addr);
			if(!Util::isPrivateIp(tmp2) && strncmp(tmp2.c_str(), "169", 3) != 0) {
				tmp = tmp2;
			}
			i++;
		}
	}
	return tmp;
}

bool Util::isPrivateIp(string const& ip) {
	struct in_addr addr;

	addr.s_addr = inet_addr(ip.c_str());

	if (addr.s_addr  != INADDR_NONE) {
		unsigned long haddr = ntohl(addr.s_addr);
		return ((haddr & 0xff000000) == 0x0a000000 || // 10.0.0.0/8
				(haddr & 0xff000000) == 0x7f000000 || // 127.0.0.0/8
				(haddr & 0xffff0000) == 0xa9fe0000 || // 169.254.0.0/16
				(haddr & 0xfff00000) == 0xac100000 || // 172.16.0.0/12
				(haddr & 0xffff0000) == 0xc0a80000);  // 192.168.0.0/16
	}
	return false;
}

typedef const uint8_t* ccp;
static wchar_t utf8ToLC(ccp& str) {
	wchar_t c = 0;
   if(str[0] & 0x80) { 
      if(str[0] & 0x40) { 
         if(str[0] & 0x20) { 
            if(str[1] == 0 || str[2] == 0 ||
				!((((unsigned char)str[1]) & ~0x3f) == 0x80) ||
					!((((unsigned char)str[2]) & ~0x3f) == 0x80))
				{
					str++;
					return 0;
				}
				c = ((wchar_t)(unsigned char)str[0] & 0xf) << 12 |
					((wchar_t)(unsigned char)str[1] & 0x3f) << 6 |
					((wchar_t)(unsigned char)str[2] & 0x3f);
				str += 3;
			} else {
				if(str[1] == 0 ||
					!((((unsigned char)str[1]) & ~0x3f) == 0x80)) 
				{
					str++;
					return 0;
				}
				c = ((wchar_t)(unsigned char)str[0] & 0x1f) << 6 |
					((wchar_t)(unsigned char)str[1] & 0x3f);
				str += 2;
			}
		} else {
			str++;
			return 0;
		}
	} else {
		wchar_t c = Text::asciiToLower((char)str[0]);
		str++;
		return c;
	}

	return Text::toLower(c);
}

string Util::toString(const string& sep, const StringList& lst) {
	string ret;
	for(StringList::const_iterator i = lst.begin(), iend = lst.end(); i != iend; ++i) {
		ret += *i;
		if(i + 1 != iend)
			ret += sep;
	}
	return ret;
}

string Util::toString(const StringList& lst) {
	if(lst.size() == 1)
		return lst[0];
	string tmp("[");
	for(StringList::const_iterator i = lst.begin(), iend = lst.end(); i != iend; ++i) {
		tmp += *i + ',';
	}
	if(tmp.length() == 1)
		tmp.push_back(']');
	else
		tmp[tmp.length()-1] = ']';
	return tmp;
}

string::size_type Util::findSubString(const string& aString, const string& aSubString, string::size_type start) noexcept {
	if(aString.length() < start)
		return (string::size_type)string::npos;

	if(aString.length() - start < aSubString.length())
		return (string::size_type)string::npos;

	if(aSubString.empty())
		return 0;

	// Hm, should start measure in characters or in bytes? bytes for now...
	const uint8_t* tx = (const uint8_t*)aString.c_str() + start;
	const uint8_t* px = (const uint8_t*)aSubString.c_str();

	const uint8_t* end = tx + aString.length() - start - aSubString.length() + 1;

	wchar_t wp = utf8ToLC(px);

	while(tx < end) {
		const uint8_t* otx = tx;
		if(wp == utf8ToLC(tx)) {
			const uint8_t* px2 = px;
			const uint8_t* tx2 = tx;

			for(;;) {
				if(*px2 == 0)
					return otx - (uint8_t*)aString.c_str();

				if(utf8ToLC(px2) != utf8ToLC(tx2))
					break;
			}
		}
	}
	return (string::size_type)string::npos;
}

wstring::size_type Util::findSubString(const wstring& aString, const wstring& aSubString, wstring::size_type pos) noexcept {
	if(aString.length() < pos)
		return static_cast<wstring::size_type>(wstring::npos);

	if(aString.length() - pos < aSubString.length())
		return static_cast<wstring::size_type>(wstring::npos);

	if(aSubString.empty())
		return 0;

	wstring::size_type j = 0;
	wstring::size_type end = aString.length() - aSubString.length() + 1;

	for(; pos < end; ++pos) {
		if(Text::toLower(aString[pos]) == Text::toLower(aSubString[j])) {
			wstring::size_type tmp = pos+1;
			bool found = true;
			for(++j; j < aSubString.length(); ++j, ++tmp) {
				if(Text::toLower(aString[tmp]) != Text::toLower(aSubString[j])) {
					j = 0;
					found = false;
					break;
				}
			}

			if(found)
				return pos;
		}
	}
	return static_cast<wstring::size_type>(wstring::npos);
}

string Util::encodeURI(const string& aString, bool reverse) {
	// reference: rfc2396
	string tmp = aString;
	if(reverse) {
		string::size_type idx;
		for(idx = 0; idx < tmp.length(); ++idx) {
			if(tmp.length() > idx + 2 && tmp[idx] == '%' && isxdigit(tmp[idx+1]) && isxdigit(tmp[idx+2])) {
				tmp[idx] = fromHexEscape(tmp.substr(idx+1,2));
				tmp.erase(idx+1, 2);
			} else { // reference: rfc1630, magnet-uri draft
				if(tmp[idx] == '+')
					tmp[idx] = ' ';
			}
		}
	} else {
		const string disallowed = ";/?:@&=+$," // reserved
			                      "<>#%\" "    // delimiters
		                          "{}|\\^[]`"; // unwise
		string::size_type idx, loc;
		for(idx = 0; idx < tmp.length(); ++idx) {
			if(tmp[idx] == ' ') {
				tmp[idx] = '+';
			} else {
				if(tmp[idx] <= 0x1F || tmp[idx] >= 0x7f || (loc = disallowed.find_first_of(tmp[idx])) != string::npos) {
					tmp.replace(idx, 1, toHexEscape(tmp[idx]));
					idx+=2;
				}
			}
		}
	}
	return tmp;
}

/**
 * This function takes a string and a set of parameters and transforms them according to
 * a simple formatting rule, similar to strftime. In the message, every parameter should be
 * represented by %[name]. It will then be replaced by the corresponding item in 
 * the params stringmap. After that, the string is passed through strftime with the current
 * date/time and then finally written to the log file. If the parameter is not present at all,
 * it is removed from the string completely...
 */
string Util::formatParams(const string& msg, const StringMap& params, bool filter, const time_t t) {
	string result = msg;

	string::size_type i, j, k;
	i = 0;
	while (( j = result.find("%[", i)) != string::npos) {
		if( (result.size() < j + 2) || ((k = result.find(']', j + 2)) == string::npos) ) {
			break;
		}
		string name = result.substr(j + 2, k - j - 2);
		StringMap::const_iterator smi = params.find(name);
		if(smi == params.end()) {
			result.erase(j, k-j + 1);
			i = j;
		} else {
			if(smi->second.find_first_of("%\\./") != string::npos) {
				string tmp = smi->second;	// replace all % in params with %% for strftime
				string::size_type m = 0;
				while(( m = tmp.find('%', m)) != string::npos) {
					tmp.replace(m, 1, "%%");
					m+=2;
				}
				if(filter) {
					// Filter chars that produce bad effects on file systems
					m = 0;
					while(( m = tmp.find_first_of("\\./", m)) != string::npos) {
						tmp[m] = '_';
					}
				}
				
				result.replace(j, k-j + 1, tmp);
				i = j + tmp.size();
			} else {
				result.replace(j, k-j + 1, smi->second);
				i = j + smi->second.size();
			}
		}
	}

	result = formatTime(result, t);
	
	return result;
}

string Util::formatRegExp(const string& msg, const StringMap& params) {
	string result = msg;
	string::size_type i, j, k;
	i = 0;
	while (( j = result.find("%[", i)) != string::npos) {
		if( (result.size() < j + 2) || ((k = result.find(']', j + 2)) == string::npos) ) {
			break;
		}
		string name = result.substr(j + 2, k - j - 2);
		StringMap::const_iterator smi = params.find(name);
		if(smi != params.end()) {
			result.replace(j, k-j + 1, smi->second);
			i = j + smi->second.size();
		} else {
			i = k + 1;
		}
	}
	return result;
}

uint64_t Util::getDirSize(const string &sFullPath) {
	uint64_t total = 0;

	FileFindIter end;
#ifdef _WIN32
	for(FileFindIter i(sFullPath + PATH_SEPARATOR_STR "*"); i != end; ++i) {
#else
	for(FileFindIter i(sFullPath + PATH_SEPARATOR_STR); i != end; ++i) {
#endif
		string name = i->getFileName();

		if(name == "." || name == "..")
			continue;

		//check for forbidden file patterns
		if(BOOLSETTING(REMOVE_FORBIDDEN)) {
			string::size_type nameLen = name.size();
			string fileExt = Util::getFileExt(name);
			if ((stricmp(fileExt.c_str(), ".tdc") == 0) ||
				(stricmp(fileExt.c_str(), ".GetRight") == 0) ||
				(stricmp(fileExt.c_str(), ".temp") == 0) ||
				(stricmp(fileExt.c_str(), ".jc!") == 0) ||	//FlashGet
				(stricmp(fileExt.c_str(), ".dmf") == 0) ||	//Download Master
				(stricmp(fileExt.c_str(), ".!ut") == 0) ||	//uTorrent
				(stricmp(fileExt.c_str(), ".bc!") == 0) ||	//BitComet
				(nameLen > 9 && name.rfind("part.met") == nameLen - 8) ||				
				(name.find("__padding_") == 0) ||			//BitComet padding
				(name.find("__INCOMPLETE__") == 0) ||		//winmx
				(name.find("__incomplete__") == 0) ||		//winmx
				(nameLen >= 28 && name.find("download") == 0 && name.rfind(".dat") == nameLen - 4 && Util::toInt64(name.substr(8, 16)))) {	//kazaa temps
					continue;
			}
		}

		if(Wildcard::match(name, SETTING(SKIPLIST_SHARE), ';'))
			continue;

		if(!BOOLSETTING(SHARE_HIDDEN) && i->isHidden())
			continue;
		if(i->isLink())
 			continue;
 			
		if(i->isDirectory()) {
			string newName = sFullPath + PATH_SEPARATOR + name;

			if((stricmp(newName, SETTING(TEMP_DOWNLOAD_DIRECTORY)) == 0)) { continue; }

#ifdef _WIN32
			// don't share Windows directory
			TCHAR path[MAX_PATH];
			::SHGetFolderPath(NULL, CSIDL_WINDOWS, NULL, SHGFP_TYPE_CURRENT, path);
			if(strnicmp(newName, Text::fromT((tstring)path), _tcslen(path)) == 0) { continue; }
#endif

			total += getDirSize(newName);
		} else {
			// Not a directory, assume it's a file...make sure we're not sharing the settings file...
			if( (stricmp(name.c_str(), "DCPlusPlus.xml") == 0) ||
				(stricmp(name.c_str(), "Favorites.xml") == 0) ||
				(stricmp(Util::getFileExt(name).c_str(), ".dctmp") == 0) ||
				(stricmp(Util::getFileExt(name).c_str(), ".antifrag") == 0) ) { continue; }

			string fileName = sFullPath + PATH_SEPARATOR + name;
			if(stricmp(fileName, SETTING(TLS_PRIVATE_KEY_FILE)) == 0) { continue; }

			total += i->getSize();
		}
	}

	return total;
}

bool Util::fileExists(const string &aFile) {
#ifdef _WIN32
	DWORD attr = GetFileAttributes(Text::toT(aFile).c_str());
	return (attr != 0xFFFFFFFF);
#else
    struct stat st;
    return (stat(aFile.c_str(), &st) == 0);
#endif
}

string Util::formatTime(const string &msg, const time_t t) {
	if(!msg.empty()) {
		string ret = msg;
		tm* loc = localtime(&t);

		if(!loc) {
			return Util::emptyString;
		}

#ifdef _MSC_VER
		// Work it around :P
		string::size_type i = 0;
		while((i = ret.find("%", i)) != string::npos) {
			if(string("aAbBcdHIjmMpSUwWxXyYzZ%").find(ret[i+1]) == string::npos) {
				ret.replace(i, 1, "%%");
			}
			i += 2;
		}
#endif

		size_t bufsize = ret.size() + 256;
		string buf(bufsize, 0);

		errno = 0;

		buf.resize(strftime(&buf[0], bufsize-1, ret.c_str(), loc));
		
		while(buf.empty()) {
			if(errno == EINVAL)
				return Util::emptyString;

			bufsize+=64;
			buf.resize(bufsize);
			buf.resize(strftime(&buf[0], bufsize-1, ret.c_str(), loc));
		}

#ifdef _WIN32
		if(!Text::validateUtf8(buf))
#endif
		{
			buf = Text::toUtf8(buf);
		}
		return buf;
	}
	return Util::emptyString;
}

/* Below is a high-speed random number generator with much
   better granularity than the CRT one in msvc...(no, I didn't
   write it...see copyright) */ 
/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.
   Any feedback is very welcome. For any question, comments,       
   see http://www.math.keio.ac.jp/matumoto/emt.html or email       
   matumoto@math.keio.ac.jp */       
/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializing the array with a NONZERO seed */
static void sgenrand(unsigned long seed) {
	/* setting initial seeds to mt[N] using         */
	/* the generator Line 25 of Table 1 in          */
	/* [KNUTH 1981, The Art of Computer Programming */
	/*    Vol. 2 (2nd Ed.), pp102]                  */
	mt[0]= seed & 0xffffffff;
	for (mti=1; mti<N; mti++)
		mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}

uint32_t Util::rand() {
	unsigned long y;
	static unsigned long mag01[2]={0x0, MATRIX_A};
	/* mag01[x] = x * MATRIX_A  for x=0,1 */

	if (mti >= N) { /* generate N words at one time */
		int kk;

		if (mti == N+1)   /* if sgenrand() has not been called, */
			sgenrand(4357); /* a default initial seed is used   */

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

		mti = 0;
	}

	y = mt[mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	return y; 
}

/*	getIpCountry
	This function returns the full country name of an ip, eg "Portugal"
	or the 2 character code, for above "PT".
	more info: http://www.maxmind.com/app/csv
*/
const string& Util::getIpCountry (const string& IP, bool full) {
	if (BOOLSETTING(GET_USER_COUNTRY)) {
		if(count(IP.begin(), IP.end(), '.') != 3)
			return emptyString;

		//e.g IP 23.24.25.26 : w=23, x=24, y=25, z=26
		string::size_type a = IP.find('.');
		string::size_type b = IP.find('.', a+1);
		string::size_type c = IP.find('.', b+2);

		/// @todo this is impl dependant and is working by chance because we are currently using atoi!
		uint32_t ipnum = (toUInt32(IP.c_str()) << 24) |
			(toUInt32(IP.c_str() + a + 1) << 16) |
			(toUInt32(IP.c_str() + b + 1) << 8) |
			(toUInt32(IP.c_str() + c + 1) );

		auto i = countries.lower_bound(ipnum);
		if(i != countries.end()) {
			const auto& countryPair = countryNames[i->second];
			return full ? countryPair.first : countryPair.second;
		}
	}

	return emptyString;
}

string Util::getTimeString(time_t _tt /* = time(NULL)*/) {
	char buf[64];
	tm* _tm = localtime(&_tt);
	if(_tm == NULL) {
		strcpy(buf, "xx:xx:xx");
	} else {
		strftime(buf, 64, "%X", _tm);
	}
	return buf;
}

string Util::toAdcFile(const string& file) {
	if(file == "files.xml.bz2" || file == "files.xml")
		return file;

	string ret;
	ret.reserve(file.length() + 1);
	ret += '/';
	ret += file;
	for(string::size_type i = 0; i < ret.length(); ++i) {
		if(ret[i] == '\\') {
			ret[i] = '/';
		}
	}
	return ret;
}

string Util::fromAdcFile(const string& file) {
	if(file.empty())
		return Util::emptyString;

	string ret(file.substr(1));
#ifdef _WIN32
	for(string::size_type i = 0; i < ret.length(); ++i) {
		if(ret[i] == '/') {
			ret[i] = '\\';
		}
	}
#endif
	return ret;
}

string Util::translateError(int aError) {
#ifdef _WIN32
	LPTSTR lpMsgBuf;
	DWORD chars = FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		aError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	if(chars == 0) {
		return string();
	}
	string tmp = Text::fromT(lpMsgBuf);
	// Free the buffer.
	LocalFree( lpMsgBuf );
	string::size_type i = 0;

	while( (i = tmp.find_first_of("\r\n", i)) != string::npos) {
		tmp.erase(i, 1);
	}
	return tmp;
#else // _WIN32
	return Text::toUtf8(strerror(aError));
#endif // _WIN32
}

string Util::getTempPath() {
#ifdef _WIN32
	TCHAR buf[MAX_PATH + 1];
	DWORD x = GetTempPath(MAX_PATH, buf);
	return Text::fromT(tstring(buf, x));
#else
	return "/tmp/";
#endif
}

string Util::getFatalLogPath() {
#ifdef _WIN32
	return getPath(PATH_RESOURCES);
#else
	mkdir("/tmp/apexdc-log", S_IRWXU | S_IRWXG | S_IRWXO);
	return "/tmp/apexdc-log/";
#endif
}

/* natural sorting */ 
int Util::DefaultSort(const wchar_t* a, const wchar_t* b, bool noCase /*=  true*/) {
	if(BOOLSETTING(NAT_SORT)) {
		int v1, v2;
		while(*a != 0 && *b != 0) {
			v1 = 0; v2 = 0;
			bool t1 = isNumericW(*a);
			bool t2 = isNumericW(*b);
			if(t1 != t2)
				return (t1) ? -1 : 1;

			if(!t1 && noCase) {
				wchar_t c1 = Text::toLower(*a);
				wchar_t c2 = Text::toLower(*b);
				if(c1 != c2)
					return (static_cast<int>(c1) - static_cast<int>(c2));
				a++; b++;
			} else if(!t1) {
				if(*a != *b)
					return (static_cast<int>(*a) - static_cast<int>(*b));
				a++; b++;
			} else {
			    while(isNumericW(*a)) {
			       v1 *= 10;
			       v1 += *a - L'0';
			       a++;
			    }

	            while(isNumericW(*b)) {
		           v2 *= 10;
		           v2 += *b - L'0';
		           b++;
		        }

				if(v1 != v2)
					return (v1 < v2) ? -1 : 1;
			}			
		}

		return noCase ? ((int)Text::toLower(*a) - (int)Text::toLower(*b)) : (static_cast<int>(*a) - static_cast<int>(*b));
	} else {
#ifdef _WIN32
		return noCase ? lstrcmpiW(a, b) : lstrcmpW(a, b);
#else
		return noCase ? wcsicmp(a, b) : wcscmp(a, b);
#endif
	}
}

int Util::DefaultSort(const char* a, const char* b, bool noCase /*= true*/) {
	if(BOOLSETTING(NAT_SORT)) {
		int v1, v2;
		while(*a != 0 && *b != 0) {
			v1 = 0; v2 = 0;
			bool t1 = isNumeric(*a);
			bool t2 = isNumeric(*b);
			if(t1 != t2)
				return (t1) ? -1 : 1;

			if(!t1 && noCase) {
				int c1 = towlower(*a);
				int c2 = towlower(*b);
				if(c1 != c2)
					return c1 - c2;
				a++; b++;
			} else if(!t1) {
				if(*a != *b)
					return (static_cast<int>(*a) - static_cast<int>(*b));
				a++; b++;
			} else {
			    while(isNumeric(*a)) {
			       v1 *= 10;
			       v1 += *a - '0';
			       a++;
			    }

	            while(isNumeric(*b)) {
		           v2 *= 10;
		           v2 += *b - '0';
		           b++;
		        }

				if(v1 != v2)
					return (v1 < v2) ? -1 : 1;
			}			
		}

		return noCase ? (towlower(*a) - towlower(*b)) : (static_cast<int>(*a) - static_cast<int>(*b));
	} else {
#ifdef _WIN32
		return noCase ? lstrcmpiA(a, b) : lstrcmpA(a, b);
#else
		return noCase ? stricmp(a, b) : strcmp(a, b);
#endif
	}
}
	
} // namespace dcpp

/**
 * @file
 * $Id$
 */
