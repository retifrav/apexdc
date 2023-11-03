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
#include "SearchManager.h"

#include <boost/range/algorithm/find_if.hpp>
#include <boost/scoped_array.hpp>

#include "UploadManager.h"
#include "ClientManager.h"
#include "ShareManager.h"
#include "PluginManager.h"
#include "SearchResult.h"
#include "ResourceManager.h"
#include "QueueManager.h"
#include "StringTokenizer.h"
#include "FinishedManager.h"
#include "SimpleXML.h"

namespace dcpp {

const char* SearchManager::types[TYPE_LAST] = {
	CSTRING(ANY),
	CSTRING(AUDIO),
	CSTRING(COMPRESSED),
	CSTRING(DOCUMENT),
	CSTRING(EXECUTABLE),
	CSTRING(PICTURE),
	CSTRING(VIDEO),
	CSTRING(DIRECTORY),
	"TTH"
};
const char* SearchManager::getTypeStr(int type) {
	return types[type];
}

SearchManager::SearchManager() :
	port(0),
	stop(false)
{
	SettingsManager::getInstance()->addListener(this);
}

SearchManager::~SearchManager() {
	SettingsManager::getInstance()->removeListener(this);
	if(socket.get()) {
		stop = true;
		socket->disconnect();
#ifdef _WIN32
		join();
#endif
	}
}

string SearchManager::normalizeWhitespace(const string& aString){
	string::size_type found = 0;
	string normalized = aString;
	while((found = normalized.find_first_of("\t\n\r", found)) != string::npos) {
		normalized[found] = ' ';
		found++;
	}
	return normalized;
}

bool SearchManager::addSearchToHistory(const tstring& search) {
	if(search.empty())
		return false;

	Lock l(cs);

	if(searchHistory.find(search) != searchHistory.end())
		return false;

		
	while(searchHistory.size() > static_cast<std::set<tstring>::size_type>(SETTING(SEARCH_HISTORY)))
		searchHistory.erase(searchHistory.begin());

	searchHistory.insert(search);

	return true;
}

std::set<tstring>& SearchManager::getSearchHistory() {
	Lock l(cs);
	return searchHistory;
}

void SearchManager::setSearchHistory(const std::set<tstring>& list) {
	Lock l(cs);
	searchHistory.clear();
	searchHistory = list;
}

void SearchManager::clearSearchHistory() {
	Lock l(cs);
	searchHistory.clear();
}

void SearchManager::search(const string& aName, int64_t aSize, TypeModes aTypeMode /* = TYPE_ANY */, SizeModes aSizeMode /* = SIZE_ATLEAST */, const string& aToken /* = Util::emptyString */, void* aOwner /* = NULL */) {
	ClientManager::getInstance()->search(aSizeMode, aSize, aTypeMode, normalizeWhitespace(aName), aToken, aOwner);
}

uint64_t SearchManager::search(StringList& who, const string& aName, int64_t aSize /* = 0 */, TypeModes aTypeMode /* = TYPE_ANY */, SizeModes aSizeMode /* = SIZE_ATLEAST */, const string& aToken /* = Util::emptyString */, const StringList& aExtList, void* aOwner /* = NULL */) {
	return ClientManager::getInstance()->search(who, aSizeMode, aSize, aTypeMode, normalizeWhitespace(aName), aToken, aExtList, aOwner);
}

void SearchManager::listen() {
	disconnect();

	try {
		socket.reset(new Socket);
		socket->create(Socket::TYPE_UDP);
		socket->setBlocking(true);
		socket->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
		port = socket->bind(static_cast<uint16_t>(SETTING(UDP_PORT)), SETTING(BIND_ADDRESS));
		start();
	} catch(...) {
		socket.reset();
		throw;
	}
}

void SearchManager::disconnect() noexcept {
	if(socket.get()) {
		stop = true;
		queue.shutdown();
		socket->disconnect();
		port = 0;

		join();

		socket.reset();

		stop = false;
	}
}

#define BUFSIZE 8192
int SearchManager::run() {
	boost::scoped_array<uint8_t> buf(new uint8_t[BUFSIZE]);
	int len;
	sockaddr_in remoteAddr = { 0 };

	queue.start();
	while(!stop) {
		try {
			while ( true ) {
				// @todo: remove this workaround for http://bugs.winehq.org/show_bug.cgi?id=22291
				// if that's fixed by reverting to simpler while (read(...) > 0) {...} code.
				while (socket->wait(400, Socket::WAIT_READ) != Socket::WAIT_READ);
				if (stop || (len = socket->read(&buf[0], BUFSIZE, remoteAddr)) <= 0)
					break;
				string data(reinterpret_cast<char*>(&buf[0]), len);
				string sRemoteAddr = inet_ntoa(remoteAddr.sin_addr);

				if(PluginManager::getInstance()->onUDP(false, sRemoteAddr, Util::toString(port), data))
					continue;

				onData(data, sRemoteAddr);
			}
		} catch(const SocketException& e) {
			dcdebug("SearchManager::run Error: %s\n", e.getError().c_str());
		}

		bool failed = false;
		while(!stop) {
			try {
				socket->disconnect();
				socket->create(Socket::TYPE_UDP);
				socket->setBlocking(true);
				socket->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
				socket->bind(port, SETTING(BIND_ADDRESS));
				if(failed) {
					LogManager::getInstance()->message("Search enabled again", LogManager::LOG_INFO); // TODO: translate
					failed = false;
				}
				break;
			} catch(const SocketException& e) {
				dcdebug("SearchManager::run Stopped listening: %s\n", e.getError().c_str());

				if(!failed) {
					LogManager::getInstance()->message("Search disabled: " + e.getError(), LogManager::LOG_ERROR); // TODO: translate
					failed = true;
				}

				// Spin for 60 seconds
				for(int i = 0; i < 60 && !stop; ++i) {
					Thread::sleep(1000);
				}
			}
		}
	}
	return 0;
}

int SearchManager::UdpQueue::run() {
	string x = Util::emptyString;
	string remoteIp = Util::emptyString;
	stop = false;

	while(true) {
		s.wait();
		if(stop)
			break;

		{
			Lock l(cs);
			if(resultList.empty()) continue;

			x = resultList.front().first;
			remoteIp = resultList.front().second;
			resultList.pop_front();
		}

		if(x.compare(0, 1, "$") == 0) {
			// NMDC commands
			if (x.compare(1, 3, "SR ") == 0) {
				SearchManager::getInstance()->onSR(x, remoteIp);
			} else {
				dcdebug("Unknown NMDC command received via UDP: %s\n", x.c_str());
			}

			continue;
		}

		// ADC commands
		// must end with \n
		if(x[x.length() - 1] != 0x0a) {
			dcdebug("Invalid UDP data received: %s (no newline)\n", x.c_str());
			continue;
		}

		if(!Text::validateUtf8(x)) {
			dcdebug("UTF-8 valition failed for received UDP data: %s\n", x.c_str());
			continue;
		}

		// Dispatch without newline
		dispatch(x.substr(0, x.length() - 1), false, remoteIp);
		Thread::sleep(10);
	}
	return 0;
}

void SearchManager::UdpQueue::handle(AdcCommand::RES, AdcCommand & c, const string & remoteIp) noexcept {
	if(c.getParameters().empty())
		return;

	string cid = c.getParam(0);
	if(cid.size() != 39)
		return;

	UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
	if(!user)
		return;

	// This should be handled by AdcCommand really...
	c.getParameters().erase(c.getParameters().begin());

	SearchManager::getInstance()->onRES(c, user, remoteIp);
}

void SearchManager::UdpQueue::handle(AdcCommand::PSR, AdcCommand & c, const string & remoteIp) noexcept {
	if(c.getParameters().empty())
		return;

	string cid = c.getParam(0);
	if(cid.size() != 39)
		return;

	UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
	// when user == NULL then it is probably NMDC user, check it later
			
	c.getParameters().erase(c.getParameters().begin());			
			
	SearchManager::getInstance()->onPSR(c, user, remoteIp);
}

void SearchManager::onData(const string& data, const string& remoteIp /*= Util::emptyString*/) {
	if(data.empty()) { return; } // shouldn't happen but rather be safe...
	queue.addResult(data, remoteIp);
}

void SearchManager::onSR(const string & x, const string & remoteIp) {
	string::size_type i, j;
	// Directories: $SR <nick><0x20><directory><0x20><free slots>/<total slots><0x05><Hubname><0x20>(<Hubip:port>)
	// Files:       $SR <nick><0x20><filename><0x05><filesize><0x20><free slots>/<total slots><0x05><Hubname><0x20>(<Hubip:port>)
	i = 4;
	if( (j = x.find(' ', i)) == string::npos) {
		return;
	}
	string nick = x.substr(i, j-i);
	i = j + 1;

	// A file has 2 0x05, a directory only one
	size_t cnt = count(x.begin() + j, x.end(), 0x05);
	
	SearchResult::Types type = SearchResult::TYPE_FILE;
	string file;
	int64_t size = 0;

	if(cnt == 1) {
		// We have a directory...find the first space beyond the first 0x05 from the back 
		// (dirs might contain spaces as well...clever protocol, eh?)
		type = SearchResult::TYPE_DIRECTORY;
		// Get past the hubname that might contain spaces
		if((j = x.rfind(0x05)) == string::npos) {
			return;
		}
		// Find the end of the directory info
		if((j = x.rfind(' ', j-1)) == string::npos) {
			return;
		}
		if(j < i + 1) {
			return;
		}	
		file = x.substr(i, j-i) + '\\';
	} else if(cnt == 2) {
		if( (j = x.find((char)5, i)) == string::npos) {
			return;
		}
		file = x.substr(i, j-i);
		i = j + 1;
		if( (j = x.find(' ', i)) == string::npos) {
			return;
		}
		size = Util::toInt64(x.substr(i, j-i));
	}	
	i = j + 1;

	if( (j = x.find('/', i)) == string::npos) {
		return;
	}
	uint8_t freeSlots = (uint8_t)Util::toInt(x.substr(i, j-i));
	i = j + 1;
	if( (j = x.find((char)5, i)) == string::npos) {
		return;
	}
	uint8_t slots = (uint8_t)Util::toInt(x.substr(i, j-i));
	i = j + 1;
	if( (j = x.rfind(" (")) == string::npos) {
		return;
	}
	string hubName = x.substr(i, j-i);
	i = j + 2;
	if( (j = x.rfind(')')) == string::npos) {
		return;
	}

	string hubIpPort = x.substr(i, j-i);
	string url = ClientManager::getInstance()->findHub(hubIpPort);

	string encoding = ClientManager::getInstance()->findHubEncoding(url);
	nick = Text::toUtf8(nick, encoding);
	file = Text::toUtf8(file, encoding);
	hubName = Text::toUtf8(hubName, encoding);

	UserPtr user = ClientManager::getInstance()->findUser(nick, url);
	if(!user) {
		// Could happen if hub has multiple URLs / IPs
		user = ClientManager::getInstance()->findLegacyUser(nick);
		if(!user)
			return;
	}
	ClientManager::getInstance()->setIPUser(user, remoteIp);

	string tth;
	if(hubName.compare(0, 4, "TTH:") == 0) {
		tth = hubName.substr(4);
		StringList names = ClientManager::getInstance()->getHubNames(user->getCID(), Util::emptyString);
		hubName = names.empty() ? STRING(OFFLINE) : Util::toString(names);
	}

	if(tth.empty() && type == SearchResult::TYPE_FILE) {
		return;
	}

	SearchResultPtr sr(new SearchResult(HintedUser(user, url), type, slots, freeSlots, size,
		file, hubName, remoteIp, TTHValue(tth), Util::emptyString));

	SearchManager::getInstance()->fire(SearchManagerListener::SR(), sr);
}

void SearchManager::onRES(const AdcCommand& cmd, const UserPtr& from, const string& remoteIp) {
	int freeSlots = -1;
	int64_t size = -1;
	string file;
	string tth;
	string token;

	for(auto& str: cmd.getParameters()) {
		if(str.compare(0, 2, "FN") == 0) {
			file = Util::fromAdcFile(str.substr(2));
		} else if(str.compare(0, 2, "SL") == 0) {
			freeSlots = Util::toInt(str.substr(2));
		} else if(str.compare(0, 2, "SI") == 0) {
			size = Util::toInt64(str.substr(2));
		} else if(str.compare(0, 2, "TR") == 0) {
			tth = str.substr(2);
		} else if(str.compare(0, 2, "TO") == 0) {
			token = str.substr(2);
		}
	}

	if(file.empty() || freeSlots == -1 || size == -1) { return; }

	auto type = (*(file.end() - 1) == '\\' ? SearchResult::TYPE_DIRECTORY : SearchResult::TYPE_FILE);
	if(type == SearchResult::TYPE_FILE && tth.empty()) { return; }

	string hubUrl;

	// token format: [per-hub unique id] "/" [per-search actual token] (see AdcHub::search)
	auto slash = token.find('/');
	if(slash == string::npos) { return; }
	{
		auto uniqueId = Util::toUInt32(token.substr(0, slash));
		auto lock = ClientManager::getInstance()->lock();
		auto& clients = ClientManager::getInstance()->getClients();
		auto i = boost::find_if(clients, [uniqueId](const ClientManager::ClientList::value_type& val) { return val.second->getUniqueId() == uniqueId; });
		if(i == clients.end()) { return; }
		hubUrl = *i->first;
	}
	token.erase(0, slash + 1);

	StringList names = ClientManager::getInstance()->getHubNames(from->getCID(), hubUrl);
	string hubName = names.empty() ? STRING(OFFLINE) : Util::toString(names);
		
	auto slots = ClientManager::getInstance()->getSlots(HintedUser(from, hubUrl));
	SearchResultPtr sr(new SearchResult(HintedUser(from, hubUrl), type, slots, (uint8_t)freeSlots, size,
		file, hubName, remoteIp, TTHValue(tth), token));
	fire(SearchManagerListener::SR(), sr);
}

void SearchManager::onPSR(const AdcCommand& cmd, UserPtr from, const string& remoteIp) {

	uint16_t udpPort = 0;
	uint32_t partialCount = 0;
	string tth;
	string hubIpPort;
	string nick;
	QueueItem::PartsInfo partialInfo;

	for(StringIterC i = cmd.getParameters().begin(); i != cmd.getParameters().end(); ++i) {
		const string& str = *i;
		if(str.compare(0, 2, "U4") == 0) {
			udpPort = static_cast<uint16_t>(Util::toInt(str.substr(2)));
		} else if(str.compare(0, 2, "NI") == 0) {
			nick = str.substr(2);
		} else if(str.compare(0, 2, "HI") == 0) {
			hubIpPort = str.substr(2);
		} else if(str.compare(0, 2, "TR") == 0) {
			tth = str.substr(2);
		} else if(str.compare(0, 2, "PC") == 0) {
			partialCount = Util::toUInt32(str.substr(2))*2;
		} else if(str.compare(0, 2, "PI") == 0) {
			StringTokenizer<string> tok(str.substr(2), ',');
			for(StringIter i = tok.getTokens().begin(); i != tok.getTokens().end(); ++i) {
				partialInfo.push_back((uint16_t)Util::toInt(*i));
			}
		}
	}

	string url = ClientManager::getInstance()->findHub(hubIpPort);
	if(!from || from == ClientManager::getInstance()->getMe()) {
		// for NMDC support
		
		if(nick.empty() || hubIpPort.empty()) {
			return;
		}
		
		from = ClientManager::getInstance()->findUser(nick, url);
		if(!from) {
			// Could happen if hub has multiple URLs / IPs
			from = ClientManager::getInstance()->findLegacyUser(nick);
			if(!from) {
				dcdebug("Search result from unknown user");
				return;
			}
		}
	}
	
	ClientManager::getInstance()->setIPUser(from, remoteIp, udpPort);

	if(partialInfo.size() != partialCount) {
		// what to do now ? just ignore partial search result :-/
		return;
	}

	QueueItem::PartsInfo outPartialInfo;
	QueueItem::PartialSource ps(from->isNMDC() ? ClientManager::getInstance()->getMyNick(url) : Util::emptyString, hubIpPort, remoteIp, udpPort);
	ps.setPartialInfo(partialInfo);

	QueueManager::getInstance()->handlePartialResult(HintedUser(from, url), TTHValue(tth), ps, outPartialInfo);
	
	if((udpPort > 0) && !outPartialInfo.empty()) {
		try {
			AdcCommand cmd = SearchManager::getInstance()->toPSR(false, ps.getMyNick(), hubIpPort, tth, outPartialInfo);
			ClientManager::getInstance()->send(cmd, from->getCID());
		} catch(...) {
			dcdebug("Partial search caught error\n");
		}
	}

}

void SearchManager::respond(const AdcCommand& adc, const CID& from, bool isUdpActive, const string& hubIpPort) {
	// Filter own searches
	if(from == ClientManager::getInstance()->getMe()->getCID())
		return;

	UserPtr p = ClientManager::getInstance()->findUser(from);
	if(!p)
		return;

	auto results = ShareManager::getInstance()->search(adc.getParameters(), isUdpActive ? std::max(10, SETTING(SEARCH_RESPONSES_ACTIVE)) : std::max(5, SETTING(SEARCH_RESPONSES_PASSIVE)));

	string token;

	adc.getParam("TO", 0, token);

	// TODO: don't send replies to passive users
	if(results.empty()) {
		string tth;
		if(!adc.getParam("TR", 0, tth))
			return;
			
		QueueItem::PartsInfo partialInfo;
		if(!QueueManager::getInstance()->handlePartialSearch(TTHValue(tth), partialInfo))
			return;
		
		AdcCommand cmd = toPSR(true, Util::emptyString, hubIpPort, tth, partialInfo);
		ClientManager::getInstance()->send(cmd, from);
		return;
	}

	for(SearchResultList::const_iterator i = results.begin(); i != results.end(); ++i) {
		AdcCommand cmd = (*i)->toRES(AdcCommand::TYPE_UDP);
		if(!token.empty())
			cmd.addParam("TO", token);
		ClientManager::getInstance()->send(cmd, from);
	}
}

string SearchManager::getPartsString(const QueueItem::PartsInfo& partsInfo) const {
	string ret;

	for(auto i = partsInfo.cbegin(); i < partsInfo.cend(); i+=2){
		ret += Util::toString(*i) + "," + Util::toString(*(i+1)) + ",";
	}

	return ret.substr(0, ret.size()-1);
}


AdcCommand SearchManager::toPSR(bool wantResponse, const string& myNick, const string& hubIpPort, const string& tth, const vector<uint16_t>& partialInfo) const {
	AdcCommand cmd(AdcCommand::CMD_PSR, AdcCommand::TYPE_UDP);
		
	if(!myNick.empty())
		cmd.addParam("NI", Text::utf8ToAcp(myNick));
		
	cmd.addParam("HI", hubIpPort);
	cmd.addParam("U4", Util::toString(wantResponse && ClientManager::getInstance()->isActive(hubIpPort) ? getPort() : 0));
	cmd.addParam("TR", tth);
	cmd.addParam("PC", Util::toString(partialInfo.size() / 2));
	cmd.addParam("PI", getPartsString(partialInfo));
	
	return cmd;
}

void SearchManager::on(SettingsManagerListener::Load, SimpleXML& aXml) noexcept {
	if(aXml.findChild("SearchHistory")) {
		aXml.stepIn();
		while(aXml.findChild("Search")) {
			addSearchToHistory(Text::toT(aXml.getChildData()));
		}
		aXml.stepOut();
	}	
}

void SearchManager::on(SettingsManagerListener::Save, SimpleXML& aXml) noexcept {
	aXml.addTag("SearchHistory");
	aXml.stepIn();
	{
		Lock l(cs);
		for(auto i = searchHistory.cbegin(); i != searchHistory.cend(); ++i) {
			aXml.addTag("Search", Text::fromT(*i));
		}
	}
	aXml.stepOut();
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
