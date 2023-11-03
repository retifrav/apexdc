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
#include "User.h"

#include <boost/regex.hpp>

#include "AdcHub.h"
#include "Client.h"
#include "StringTokenizer.h"
#include "FavoriteUser.h"

#include "ClientManager.h"
#include "DetectionManager.h"
#include "UserCommand.h"
#include "ResourceManager.h"
#include "FavoriteManager.h"
#include "RegEx.h"

namespace dcpp {

FastCriticalSection Identity::cs;
FastCriticalSection Identity::csCached;

OnlineUser::OnlineUser(const UserPtr& ptr, ClientBase& client_, uint32_t sid_) : identity(ptr, sid_), client(client_), isInList(false) { 

}

UserData* OnlineUser::getPluginObject() noexcept {
	resetEntity();

	pod.nick = pluginString(getIdentity().getNick());
	pod.hubHint = pluginString(getClient().getHubUrl());
	pod.cid = pluginString(getUser()->getCID().toBase32());
	pod.sid = getIdentity().getSID();
	pod.protocol = getUser()->isNMDC() ? PROTOCOL_NMDC : PROTOCOL_ADC;
	pod.isOp = getIdentity().isOp() ? True : False;

	return &pod;
}

template < typename T, typename Func >
T Identity::getCachedInfo(boost::optional<T>& cache, Func fetchFuntion)
{
	{
		FastLock l(csCached);
		if (cache)
			return *cache;
	}
	T t = fetchFuntion();
	// releasing csCached and grabbing it back like this can in rare cases result in multiple successive writes to same cache, but there's no harm from that
	FastLock l(csCached);
	cache = t;
	return t;
}

bool Identity::isTcpActive(const Client* c) const {
	if(c != NULL && user == ClientManager::getInstance()->getMe()) {
		return c->isActive(); // userlist should display our real mode
	} else {
		return (!user->isSet(User::NMDC)) ?
			!getIp().empty() && supports(AdcHub::TCP4_FEATURE) :
			!user->isSet(User::PASSIVE);	
	}
}

bool Identity::isUdpActive() const {
	if(getIp().empty() || getUdpPort().empty())
		return false;
	return (!user->isSet(User::NMDC)) ? supports(AdcHub::UDP4_FEATURE) : !user->isSet(User::PASSIVE);
}

void Identity::getParams(StringMap& sm, const string& prefix, bool compatibility, bool dht) const {
	{
		FastLock l(cs);
		for(auto i = info.cbegin(); i != info.cend(); ++i) {
			sm[prefix + string((const char*)(&i->first), 2)] = i->second;
		}
	}
	if(!dht && user) {
		sm[prefix + "NI"] = getNick();
		sm[prefix + "SID"] = getSIDString();
		sm[prefix + "CID"] = user->getCID().toBase32();
		sm[prefix + "TAG"] = getTag();
		sm[prefix + "CO"] = getConnection();
		sm[prefix + "SSshort"] = Util::formatBytes(get("SS"));
		sm[prefix + "RSshort"] = Util::formatBytes(get("RS"));
		sm[prefix + "LSshort"] = Util::formatBytes(get("LS"));
		sm[prefix + "FCTime"] = Util::formatTime(SETTING(TIME_STAMPS_FORMAT), Util::toInt64(get("FT")));
		sm[prefix + "LT"] = Util::formatTime(SETTING(TIME_STAMPS_FORMAT), Util::toInt64(get("LT")));

		if(compatibility) {
			if(prefix == "my") {
				sm["mynick"] = getNick();
				sm["mycid"] = user->getCID().toBase32();
			} else {
				sm["nick"] = getNick();
				sm["cid"] = user->getCID().toBase32();
				sm["ip"] = get("I4");
				sm["tag"] = getTag();
				sm["description"] = get("DE");
				sm["email"] = get("EM");
				sm["connection"] = getConnection();
				sm["share"] = get("SS");
				sm["shareshort"] = Util::formatBytes(get("SS"));
				sm["realshareformat"] = Util::formatBytes(get("RS"));
				sm["listsize"] = Util::formatBytes(get("LS"));
				sm["cheatingdescription"] = get("CS");
				sm["adlFile"] =	get("AI");
				sm["adlComment"] = get("AC");
				sm["adlFileSize"] =	get("AS");
				sm["adlTTH"] = get("AT");
				sm["adlForbiddenSize"] = get("AF");
				sm["adlFilesCount"] = get("AL");
				sm["adlFileSizeShort"] = Util::formatBytes(get("AS"));
				sm["adlForbiddenSizeShort"] = Util::formatBytes(get("AF"));
			}
		}
	}
}

bool Identity::isClientType(ClientType ct) const {
	const int type = getCachedInfo( cachedInfo.clientType, [this] { return Util::toInt(get("CT")); } );
	return (type & ct) == ct;
}

bool Identity::isCheckStatus(FakeFlags flag) const {
	int flags = Util::toInt(get("FC"));
	return (flags & flag) == flag;
}

string Identity::getTag() const {
	if(!get("TA").empty())
		return get("TA");
	if(get("VE").empty() || get("HN").empty() || get("HR").empty() || get("HO").empty() || get("SL").empty())
		return Util::emptyString;
	return "<" + getApplication() + ",M:" + string(isTcpActive() ? "A" : "P") + 
		",H:" + get("HN") + "/" + get("HR") + "/" + get("HO") + ",S:" + get("SL") + ">";
}

string Identity::getApplication() const {
	auto application = get("AP");
	auto version = get("VE");

	if(version.empty()) {
		return application;
	}

	if(application.empty()) {
		// AP is an extension, so we can't guarantee that the other party supports it, so default to VE.
		return version;
	}

	return application + ' ' + version;
}

string Identity::getConnection() const {
	if(!get("US").empty())
		return str(dcpp_fmt("%1%/s") % Util::formatBytes(get("US")));

	return get("CO");
}

const string& Identity::getCountry(bool full) const {
	return Util::getIpCountry(getIp(), full);
}

string Identity::get(const char* name) const {
	FastLock l(cs);
	InfIter i = info.find(*(const short*)name);
	return i == info.end() ? Util::emptyString : i->second;
}

bool Identity::isSet(const char* name) const {
	FastLock l(cs);
	InfIter i = info.find(*(const short*)name);
	return i != info.end();
}


void Identity::set(const char* name, const string& val) {
	FastLock l(cs);
	if(val.empty())
		info.erase(*(const short*)name);
	else
		info[*(const short*)name] = val;
	updateCachedInfo(name, val);
}

bool Identity::supports(const string& name) const {
	string su = get("SU");
	StringTokenizer<string> st(su, ',');
	for(StringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
		if(*i == name)
			return true;
	}
	return false;
}

std::map<string, string> Identity::getInfo() const {
	std::map<string, string> ret;

	FastLock l(cs);
	for(auto i = info.cbegin(); i != info.cend(); ++i) {
		ret[string((const char*)(&i->first), 2)] = i->second;
	}

	return ret;
}

void FavoriteUser::update(const OnlineUser& info) { 
	setNick(info.getIdentity().getNick()); 
	setUrl(info.getClient().getHubUrl()); 
}

int64_t Identity::getConnectionSpeed() const {
	const string& speedStr = get("US");
	if(speedStr.empty()) {
		int64_t speed = 0;
		const string& conn = get("CO");
		if(conn.find("Kbps") != string::npos) {
			// Ancient NMDC connection types (not sure if this is correct, can't bother checking)
			speed = (long)(Util::toDouble(conn) * 1024 / 8);
		} else if(conn.find("KiB/s") != string::npos) {
			// Limiter report
			speed = (long)(Util::toDouble(conn) * 1024);
		} else {
			speed = (long)(Util::toDouble(conn) * 1024 * 1024 / 8);
		}
		return (speed > 0) ? speed : -1;
	} else return Util::toInt64(speedStr);
}

string Identity::setCheat(const ClientBase& c, const string& aCheatDescription, bool aBadClient) {
	if(!c.isOp() || isOp()) return Util::emptyString;

	StringMap ucParams;
	getParams(ucParams, "user", true);
	string cheat = Util::formatParams(aCheatDescription, ucParams, false);
	
	set("CS", cheat);

	if(aBadClient)
		set("FC", Util::toString(Util::toInt(get("FC")) | BAD_CLIENT));
	else
		set("FC", Util::toString(Util::toInt(get("FC")) & ~BAD_CLIENT));

	string report = "*** " + STRING(USER) + " " + getNick() + " - " + cheat;
	return report;
}

map<string, string> Identity::getReport() const {
	map<string, string> reportSet;

	string sid = getSIDString();

	{
		FastLock l(cs);
		for(auto i = info.cbegin(); i != info.cend(); ++i) {
			string name = string((const char*)(&i->first), 2);
			string value = i->second;

#define TAG(x,y) (x + (y << 8))	// TODO: boldly assumes little endian?
			
			// TODO: translate known tags and format values to something more readable
			switch(i->first) {
				case TAG('A','W'): name = "Away mode"; break;
				case TAG('B','O'): name = "Bot"; break;
				case TAG('C','L'): name = "Client name"; break;
				case TAG('C','M'): name = "Comment"; break;
				case TAG('C','O'): name = "Connection"; break;
				case TAG('C','S'): name = "Cheat description"; break;
				case TAG('C','T'): name = "Client type"; break;
				case TAG('D','E'): name = "Description"; break;
				case TAG('D','S'): name = "Download speed"; value = Util::formatBytes(value) + "/s"; break;
				case TAG('E','M'): name = "E-mail"; break;
				case TAG('F','C'): name = "Fake Check status"; break;
				case TAG('F','D'): name = "Filelist disconnects"; break;
				case TAG('F','T'): name = "User checked at"; value = Util::formatTime(SETTING(TIME_STAMPS_FORMAT), Util::toInt64(value)); break;
				case TAG('G','E'): name = "Filelist generator"; break;
				case TAG('H','N'): name = "Hubs Normal"; break;
				case TAG('H','O'): name = "Hubs OP"; break;
				case TAG('H','R'): name = "Hubs Registered"; break;
				case TAG('I','4'): name = "IPv4 Address"; value += " (" + Socket::getRemoteHost(value) + ")"; break;
				case TAG('I','6'): name = "IPv6 Address"; value += " (" + Socket::getRemoteHost(value) + ")"; break;
				case TAG('I','D'): name = "Client ID"; break;
				case TAG('K','P'): name = "KeyPrint"; break;
				case TAG('L','O'): name = "NMDC Lock"; break;
				case TAG('L','T'): name = "Logged in"; value = Util::formatTime(SETTING(TIME_STAMPS_FORMAT), Util::toInt64(value)); break;
				case TAG('N','I'): name = "Nick"; break;
				case TAG('O','P'): name = "Operator"; break;
				case TAG('P','K'): name = "NMDC Pk"; break;
				case TAG('R','S'): name = "Shared bytes - real"; value = Util::formatExactSize(Util::toInt64(value)); break;
				case TAG('S','F'): name = "Shared files"; break;
				case TAG('S','I'): name = "Session ID"; value = sid; break;
				case TAG('S','L'): name = "Slots"; break;
				case TAG('S','S'): name = "Shared bytes - reported"; value = Util::formatExactSize(Util::toInt64(value)); break;
				case TAG('S','T'): name = "NMDC Status"; value = formatStatus(Util::toInt(value)); break;
				case TAG('S','U'): name = "Supports"; break;
				case TAG('T','A'): name = "Tag"; break;
				case TAG('T','O'): name = "Timeouts"; break;
				case TAG('U','4'): name = "IPv4 UDP port"; break;
				case TAG('U','6'): name = "IPv6 UDP port"; break;
				case TAG('U','S'): name = "Upload speed"; value = Util::formatBytes(value) + "/s"; break;
				case TAG('V','E'): name = "Client version"; break;
				case TAG('F','Q'): name = ""; break;	// Threaded checking
				case TAG('W','O'): name = ""; break;	// for GUI purposes
				default: name += " (unknown)";

			}

			if(!name.empty())
				reportSet.insert(make_pair(name, value));
		}
	}

	return reportSet;
}

void Identity::updateCachedInfo(const char* name, const string& val)
{
	if (*(const short*)name == TAG('C', 'T')) {
		FastLock l(csCached);
		cachedInfo.clientType = Util::toInt(val);
	}
}

string Identity::updateClientType(const OnlineUser& ou, uint32_t& aFlag) {
	uint64_t tick = GET_TICK();

	StringMap params;
	getDetectionParams(params); // get identity fields and escape them, then get the rest and leave as-is
	const DetectionManager::DetectionItems& profiles = DetectionManager::getInstance()->getProfiles(params);
   
	for(DetectionManager::DetectionItems::const_iterator i = profiles.begin(); i != profiles.end(); ++i) {
		const DetectionEntry& entry = *i;
		if(!entry.isEnabled)
			continue;
		DetectionEntry::INFMap INFList;
		if(!entry.defaultMap.empty()) {
			// fields to check for both, adc and nmdc
			INFList = entry.defaultMap;
		} 

		if(getUser()->isSet(User::NMDC) && !entry.nmdcMap.empty()) {
			INFList.insert(INFList.end(), entry.nmdcMap.begin(), entry.nmdcMap.end());
		} else if(!entry.adcMap.empty()) {
			INFList.insert(INFList.end(), entry.adcMap.begin(), entry.adcMap.end());
		}

		if(INFList.empty())
			continue;

		bool _continue = false;

		DETECTION_DEBUG("\tChecking profile: " + entry.name);

		for(DetectionEntry::INFMap::const_iterator j = INFList.begin(); j != INFList.end(); ++j) {

			// TestSUR not supported anymore, so ignore it to be compatible with older profiles
			if(j->first == "TS") {
				if(INFList.size() == 1) {
					DETECTION_DEBUG("Profile: " + entry.name + " skipped as obsolete");
					_continue = true;
					break;
				}
				continue;
			}

			string aPattern = Util::formatRegExp(j->second, params);
			string aField = getDetectionField(j->first);
			DETECTION_DEBUG("Pattern: " + aPattern + " Field: " + aField);
			if(!Identity::matchProfile(aField, aPattern)) {
				_continue = true;
				break;
			}
		}

		INFList.clear();
		if(_continue)
			continue;

		DETECTION_DEBUG("Client found: " + entry.name + " time taken: " + Util::toString(GET_TICK()-tick) + " milliseconds");

		set("CL", entry.name);
		set("CM", entry.comment);
		
		if(entry.clientFlag != DetectionEntry::RED)
			set("FC", Util::toString(Util::toInt(get("FC")) & ~BAD_CLIENT));
		else
			set("FC", Util::toString(Util::toInt(get("FC")) | BAD_CLIENT));

		if(entry.checkMismatch && getUser()->isSet(User::NMDC) &&  (params["VE"] != params["PKVE"])) { 
			set("CL", entry.name + " Version mis-match");
			aFlag = DetectionEntry::RED;
			return setCheat(ou.getClient(), entry.cheat + " Version mis-match", true);
		}

		string report = Util::emptyString;
		if(!entry.cheat.empty()) {
			report = setCheat(ou.getClient(), entry.cheat, (entry.clientFlag == DetectionEntry::RED));
		} if(entry.rawToSend > 0) {
			ClientManager::getInstance()->sendRawCommand(ou, entry.rawToSend);
		}

		aFlag = entry.clientFlag;
		return report;
	}

	set("CL", "Unknown");
	set("CS", Util::emptyString);
	set("FC", Util::toString(Util::toInt(get("FC")) & ~BAD_CLIENT));
	return Util::emptyString;
}

string Identity::getDetectionField(const string& aName) const {
	if(aName.length() == 2) {
		return (aName == "TA") ? getTag() : get(aName.c_str());
	} else {
		if(aName == "PKVE") {
			return getPkVersion();
		} else if(aName == "CID") {
			return getUser()->getCID().toBase32();
		} else if(aName == "TAG") {
			return getTag();
		}
		return Util::emptyString;
	}
}

void Identity::getDetectionParams(StringMap& p) {
	getParams(p, Util::emptyString, false);
	p["PKVE"] = getPkVersion();
	//p["VEformat"] = getVersion();
   
	if(!user->isSet(User::NMDC)) {
		string version = get("VE");
		string::size_type i = version.find(' ');
		if(i != string::npos)
			p["VEformat"] = version.substr(i+1);
		else
			p["VEformat"] = version;
	} else {
		p["VEformat"] = get("VE");
	}

	// convert all special chars to make regex happy
	for(StringMap::iterator i = p.begin(); i != p.end(); ++i) {
		// looks really bad... but do the job
		Util::replace("\\", "\\\\", i->second); // this one must be first
		Util::replace("[", "\\[", i->second);
		Util::replace("]", "\\]", i->second);
		Util::replace("^", "\\^", i->second);
		Util::replace("$", "\\$", i->second);
		Util::replace(".", "\\.", i->second);
		Util::replace("|", "\\|", i->second);
		Util::replace("?", "\\?", i->second);
		Util::replace("*", "\\*", i->second);
		Util::replace("+", "\\+", i->second);
		Util::replace("(", "\\(", i->second);
		Util::replace(")", "\\)", i->second);
		Util::replace("{", "\\{", i->second);
		Util::replace("}", "\\}", i->second);
	}
	p["TA"] = p["TAG"];
}

string Identity::getPkVersion() const {
	string pk = get("PK");

	string::const_iterator begin = pk.begin();
	string::const_iterator end = pk.end();
	boost::match_results<string::const_iterator> result;
	boost::regex reg("[0-9]+\\.[0-9]+", boost::regex_constants::icase);
	if(boost::regex_search(begin, end, result, reg, boost::match_default))
		return result.str();

	return Util::emptyString;
}

bool Identity::matchProfile(const string& aString, const string& aProfile) const {
	DETECTION_DEBUG("\t\tMatching String: " + aString + " to Profile: " + aProfile);
	
	try {
		boost::regex reg(aProfile);
		return boost::regex_search(aString.begin(), aString.end(), reg);
	} catch(...) {
	}
	
	return false;
}

string Identity::formatStatus(int iStatus) const {
	string status;

	if(iStatus & Identity::NORMAL) {
		status += "Normal ";
	}
	if(iStatus & Identity::AWAY) {
		status += "Away ";
	}
	if(iStatus & Identity::SERVER) {
		status += "Fileserver ";
	}
	if(iStatus & Identity::FIREBALL) {
		status += "Fireball ";
	}
	if(iStatus & Identity::TLS) {
		status += "TLS ";
	}
	
	return (status.empty() ? "Unknown " : status) + "(" + Util::toString(iStatus) + ")";
}

int OnlineUser::compareItems(const OnlineUser* a, const OnlineUser* b, uint8_t col)  {
	if(col == COLUMN_NICK) {
		bool a_isOp = a->getIdentity().isOp(),
			b_isOp = b->getIdentity().isOp();
		if(a_isOp && !b_isOp)
			return -1;
		if(!a_isOp && b_isOp)
			return 1;
		if(BOOLSETTING(SORT_FAVUSERS_FIRST)) {
			bool a_isFav = FavoriteManager::getInstance()->isFavoriteUser(a->getIdentity().getUser()),
				b_isFav = FavoriteManager::getInstance()->isFavoriteUser(b->getIdentity().getUser());
			if(a_isFav && !b_isFav)
				return -1;
			if(!a_isFav && b_isFav)
				return 1;
		}
	} else if(col == COLUMN_CONNECTION) {
		int64_t a_numericConn = a->identity.getConnectionSpeed(),
			b_numericConn = b->identity.getConnectionSpeed();
		if(a_numericConn != -1 && b_numericConn != -1)
			return compare(a_numericConn, b_numericConn);
		if(a_numericConn != -1 && b_numericConn == -1)
			return 1;
		if(a_numericConn == -1 && b_numericConn != -1)
			return -1;
	}
	switch(col) {
		case COLUMN_SHARED:
		case COLUMN_EXACT_SHARED: return compare(a->identity.getBytesShared(), b->identity.getBytesShared());
		case COLUMN_SLOTS: return compare(Util::toInt(a->identity.get("SL")), Util::toInt(b->identity.get("SL")));
		case COLUMN_HUBS: return compare(Util::toInt(a->identity.get("HN"))+Util::toInt(a->identity.get("HR"))+Util::toInt(a->identity.get("HO")), Util::toInt(b->identity.get("HN"))+Util::toInt(b->identity.get("HR"))+Util::toInt(b->identity.get("HO")));
	}
	return Util::DefaultSort(a->getTextNarrow(col).c_str(), b->getTextNarrow(col).c_str());
}

string OnlineUser::getTextNarrow(uint8_t col) const {
	switch(col) {
		case COLUMN_NICK: return identity.getNick();
		case COLUMN_SHARED: return Util::formatBytes(identity.getBytesShared());
		case COLUMN_EXACT_SHARED: return Util::formatExactSize(identity.getBytesShared());
		case COLUMN_DESCRIPTION: return identity.getDescription();
		case COLUMN_TAG: return identity.getTag();
		case COLUMN_CONNECTION: {
			int64_t speed = identity.getConnectionSpeed();
			return speed == -1 ? identity.getConnection() : Util::formatBytes(speed) + "/s";
		}
		case COLUMN_IP: {
			string ip = identity.getIp();
			if (!ip.empty()) {
				string country = Util::getIpCountry(ip);
				if (!country.empty()) {
					ip = country + " (" + ip + ")";
				}
			}
			return ip;
		}
		case COLUMN_EMAIL: return identity.getEmail();
		case COLUMN_VERSION: return identity.get("CL").empty() ? identity.get("VE") : identity.get("CL");
		case COLUMN_MODE: return identity.isTcpActive(&getClient()) ? "A" : "P";
		case COLUMN_HUBS: {
			const string hn = identity.get("HN");
			const string hr = identity.get("HR");
			const string ho = identity.get("HO");
			const string total = Util::toString(Util::toInt(identity.get("HN"))+Util::toInt(identity.get("HR"))+Util::toInt(identity.get("HO")));
			return (hn.empty() || hr.empty() || ho.empty()) ? Util::emptyString : total + " (" + hn + "/" + hr + "/" + ho + ")";
		}
		case COLUMN_SLOTS: {
			string freeSlots = identity.get("FS");
			return (!freeSlots.empty()) ? freeSlots + "/" + identity.get("SL") : identity.get("SL");
		}
		case COLUMN_CID: return identity.getUser()->getCID().toBase32();
		default: return Util::emptyString;
	}
}

tstring OnlineUser::getText(uint8_t col) const {
	return Text::toT(getTextNarrow(col));
}

bool OnlineUser::update(int sortCol, const tstring& oldText) {
	bool needsSort = ((identity.get("WO").empty() ? false : true) != identity.isOp());
	
	if(sortCol == -1) {
		isInList = true;
	} else {
		needsSort = needsSort || (oldText != getText(static_cast<uint8_t>(sortCol)));
	}

	return needsSort;
}

bool OnlineUser::isCheckable(bool delay /* = true*/) const {
	return (!identity.isHub() && !identity.isBot() && !identity.isOp()) && (!delay || (GET_TIME() - Util::toInt64(identity.get("LT"))) > (SETTING(CHECK_DELAY) / 1000));
}

bool OnlineUser::shouldCheckFileList() const {
	return (identity.get("FQ").empty() && identity.get("FT").empty());
}

bool OnlineUser::getChecked() {
	if(identity.getUser()->isSet(User::PROTECTED))
		return true;

	if((!identity.isTcpActive() && !getClient().isActive()) || getUser()->isSet(User::OLD_CLIENT)) {
		// Can't do anything if this is the case...
		identity.set("CM", "User not connectable.");
		identity.getUser()->setFlag(User::PROTECTED);
		return true; 
	} else if(BOOLSETTING(PROT_FAVS) &&  FavoriteManager::getInstance()->isFavoriteUser(getUser())) {
		// And here we have favorite user that needs protection...
		identity.set("CM", "Auto-Protected as Favorite User.");
		identity.getUser()->setFlag(User::PROTECTED);
		return true;
	} else if(Wildcard::match(identity.getNick(), getClient().getProtectedPrefixes(), ';', false)) {
		// And other protected users...
		identity.set("CM", "Users nick matched one of the protected patterns.");
		identity.getUser()->setFlag(User::PROTECTED);
		return true;
	}

	// And here we see if other users are checked or not...
	return (!identity.get("FT").empty());
}

uint8_t UserInfoBase::getImage(const Identity& identity, const Client* c) {
	uint8_t image = 12;

	if(identity.isOp()) {
		image = 0;
	} else if(identity.getStatus() & Identity::FIREBALL) {
		image = 1;
	} else if(identity.getStatus() & Identity::SERVER) {
		image = 2;
	} else {
		string conn = identity.getConnection();
		
		if(	(conn == "28.8Kbps") ||
			(conn == "33.6Kbps") ||
			(conn == "56Kbps") ||
			(conn == "Modem") ||
			(conn == "ISDN")) {
			image = 6;
		} else if(	(conn == "Satellite") ||
					(conn == "Microwave") ||
					(conn == "Wireless")) {
			image = 8;
		} else if(	(conn == "DSL") ||
					(conn == "Cable")) {
			image = 9;
		} else if(	(strncmp(conn.c_str(), "LAN", 3) == 0)) {
			image = 11;
		} else if( (strncmp(conn.c_str(), "NetLimiter", 10) == 0)) {
			image = 3;
		} else {
			double us = conn.empty() ? (8 * Util::toDouble(identity.get("US")) / 1024 / 1024): Util::toDouble(conn);
			if(us >= 10) {
				image = 10;
			} else if(us > 0.1) {
				image = 7;
			} else if(us >= 0.01) {
				image = 4;
			} else if(us > 0) {
				image = 5;
			}
		}
	}

	if(identity.isAway()) {
		image += 13;
	}
		
	string freeSlots = identity.get("FS");

	if(!freeSlots.empty() && identity.supports(AdcHub::ADCS_FEATURE) && identity.supports(AdcHub::SEGA_FEATURE) &&
		((identity.supports(AdcHub::TCP4_FEATURE) && identity.supports(AdcHub::UDP4_FEATURE)) || identity.supports(AdcHub::NAT0_FEATURE))) 
	{
		image += 26;
	}

	if(!identity.isTcpActive(c) && !identity.supports(AdcHub::NAT0_FEATURE)) {
		// Users we can't connect to right now...
		image += 52;
	}		

	return image;
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
