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
#include "ClientManager.h"

#include "ShareManager.h"
#include "SearchManager.h"
#include "ConnectionManager.h"
#include "CryptoManager.h"
#include "FavoriteManager.h"
#include "SimpleXML.h"
#include "UserCommand.h"
#include "ResourceManager.h"
#include "LogManager.h"
#include "SearchResult.h"
#include "FinishedManager.h"
#include "PluginManager.h"
#include "File.h"
#include "AdcHub.h"
#include "nmdchub.h"

#include <dht/DHT.h>

namespace dcpp {

Client* ClientManager::getClient(const string& aHubURL) {
	Client* c;
	if(strnicmp("adc://", aHubURL.c_str(), 6) == 0) {
		c = new AdcHub(aHubURL, false);
	} else if(strnicmp("adcs://", aHubURL.c_str(), 7) == 0) {
		c = new AdcHub(aHubURL, CryptoManager::getInstance()->TLSOk());
	} else if(strnicmp("nmdcs://", aHubURL.c_str(), 8) == 0) {
		c = new NmdcHub(aHubURL, CryptoManager::getInstance()->TLSOk());
	} else {
		c = new NmdcHub(aHubURL, false);
	}

	{
		Lock l(cs);
		clients.insert(make_pair(const_cast<string*>(&c->getHubUrl()), c));
	}

	c->addListener(this);

	return c;
}

void ClientManager::putClient(Client* aClient) {
	fire(ClientManagerListener::ClientDisconnected(), aClient);
	aClient->removeListeners();

	{
		Lock l(cs);
		clients.erase(const_cast<string*>(&aClient->getHubUrl()));
	}
	aClient->shutdown();
	delete aClient;
}

StringList ClientManager::getHubs(const CID& cid, const string& hintUrl) const {
	return getHubs(cid, hintUrl, FavoriteManager::getInstance()->isPrivate(hintUrl));
}

StringList ClientManager::getHubNames(const CID& cid, const string& hintUrl) const {
	return getHubNames(cid, hintUrl, FavoriteManager::getInstance()->isPrivate(hintUrl));
}

StringList ClientManager::getNicks(const CID& cid, const string& hintUrl) const {
	return getNicks(cid, hintUrl, FavoriteManager::getInstance()->isPrivate(hintUrl));
}

StringList ClientManager::getHubs(const CID& cid, const string& hintUrl, bool priv) const {
	Lock l(cs);
	StringList lst;
	if(!priv) {
		OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&cid));
		for(OnlineIterC i = op.first; i != op.second; ++i) {
			lst.push_back(i->second->getClientBase().getHubUrl());
		}
	} else {
		OnlineUser* u = findOnlineUserHint(cid, hintUrl);
		if(u)
			lst.push_back(u->getClientBase().getHubUrl());
	}
	return lst;
}

StringList ClientManager::getHubNames(const CID& cid, const string& hintUrl, bool priv) const {
	Lock l(cs);
	StringList lst;
	if(!priv) {
		OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&cid));
		for(OnlineIterC i = op.first; i != op.second; ++i) {
			lst.push_back(i->second->getClientBase().getHubName());		
		}
	} else {
		OnlineUser* u = findOnlineUserHint(cid, hintUrl);
		if(u)
			lst.push_back(u->getClientBase().getHubName());
	}
	return lst;
}

StringList ClientManager::getNicks(const CID& cid, const string& hintUrl, bool priv) const {
	Lock l(cs);
	StringSet ret;

	if(!priv) {
		OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&cid));
		for(OnlineIterC i = op.first; i != op.second; ++i) {
			ret.insert(i->second->getIdentity().getNick());
		}
	} else {
		OnlineUser* u = findOnlineUserHint(cid, hintUrl);
		if(u)
			ret.insert(u->getIdentity().getNick());
	}

	if(ret.empty()) {
		// offline
		NickMap::const_iterator i = nicks.find(const_cast<CID*>(&cid));
		if(i != nicks.end()) {
			ret.insert(i->second);
		} else {
			ret.insert('{' + cid.toBase32() + '}');
		}
	}

	return StringList(ret.begin(), ret.end());
}

string ClientManager::getField(const CID& cid, const string& hint, const char* field) const {
	Lock l(cs);

	OnlinePairC p;
	auto u = findOnlineUserHint(cid, hint, p);
	if(u) {
		auto value = u->getIdentity().get(field);
		if(!value.empty()) {
			return value;
		}
	}

	for(auto i = p.first; i != p.second; ++i) {
		auto value = i->second->getIdentity().get(field);
		if(!value.empty()) {
			return value;
		}
	}

	return Util::emptyString;
}

string ClientManager::getConnection(const CID& cid, int64_t& numeric) const {
	Lock l(cs);
	numeric = -1;
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&cid));
	if(i != onlineUsers.end()) {
		numeric = i->second->getIdentity().getConnectionSpeed();
		if(numeric == -1) {
			return i->second->getIdentity().getConnection();
		} else return Util::formatBytes(numeric) + "/s";
	}
	return STRING(OFFLINE);
}

bool ClientManager::isConnected(const string& aUrl) const {
	Lock l(cs);

	auto i = clients.find(const_cast<string*>(&aUrl));
	return i != clients.end();
}

string ClientManager::findHub(const string& ipPort) const {
	Lock l(cs);

	string url;
	for(auto i = clients.cbegin(); i != clients.cend(); ++i) {
		const Client* c = i->second;
		auto ipPortPair = NmdcHub::parseIpPort(ipPort);

		if(c->getIp() == ipPortPair.first) {
			// If exact match is found, return it
			if(c->getPort() == Util::toInt(ipPortPair.second))
				return c->getHubUrl();

			// Port is not always correct, so use this as a best guess...
			url = c->getHubUrl();
		}
	}

	return url;
}

const string& ClientManager::findHubEncoding(const string& aUrl) const {
	Lock l(cs);

	auto i = clients.find(const_cast<string*>(&aUrl));
	if(i != clients.end()) {
		return *(i->second->getEncoding());
	}
	return Text::systemCharset;
}

UserPtr ClientManager::findLegacyUser(const string& aNick) const noexcept {
	if (aNick.empty())
		return UserPtr();

	Lock l(cs);

	// this be slower now, but it's not called so often
	for(NickMap::const_iterator i = nicks.begin(); i != nicks.end(); ++i) {
		if(stricmp(i->second, aNick) == 0) {
			UserMap::const_iterator u = users.find(i->first);
			if(u != users.end() && u->second->getCID() == *i->first)
				return u->second;
		}
	}
	return UserPtr();
}

UserPtr ClientManager::getUser(const string& aNick, const string& aHubUrl) noexcept {
	CID cid = makeCid(aNick, aHubUrl);
	Lock l(cs);

	UserMap::const_iterator ui = users.find(const_cast<CID*>(&cid));
	if(ui != users.end()) {
		ui->second->setFlag(User::NMDC);
		return ui->second;
	}

	UserPtr p(new User(cid));
	p->setFlag(User::NMDC);
	users.insert(make_pair(const_cast<CID*>(&p->getCID()), p));

	return p;
}

UserPtr ClientManager::getUser(const CID& cid) noexcept {
	Lock l(cs);
	UserMap::const_iterator ui = users.find(const_cast<CID*>(&cid));
	if(ui != users.end()) {
		return ui->second;
	}

	UserPtr p(new User(cid));
	users.insert(make_pair(const_cast<CID*>(&p->getCID()), p));
	return p;
}

Client* ClientManager::getUserClient(const HintedUser& user) {
	bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);

	Lock l(cs);
	OnlineUser* u = findOnlineUser(user.user->getCID(), user.hint, priv);
	if(!u) return NULL;

	return (&u->getClient());
}

UserPtr ClientManager::findUser(const CID& cid) const noexcept {
	Lock l(cs);
	UserMap::const_iterator ui = users.find(const_cast<CID*>(&cid));
	if(ui != users.end()) {
		return ui->second;
	}
	return 0;
}

// deprecated
bool ClientManager::isOp(const UserPtr& user, const string& aHubUrl) const {
	Lock l(cs);
	OnlinePairC p = onlineUsers.equal_range(const_cast<CID*>(&user->getCID()));
	for(OnlineIterC i = p.first; i != p.second; ++i) {
		if(i->second->getClient().getHubUrl() == aHubUrl) {
			return i->second->getIdentity().isOp();
		}
	}
	return false;
}

bool ClientManager::isStealth(const string& aHubUrl) const {
	Lock l(cs);
	auto i = clients.find(const_cast<string*>(&aHubUrl));
	if(i != clients.end()) {
		return i->second->getStealth();
	}
	return false;
}

CID ClientManager::makeCid(const string& aNick, const string& aHubUrl) const noexcept {
	string n = Text::toLower(aNick);
	TigerHash th;
	th.update(n.c_str(), n.length());
	th.update(Text::toLower(aHubUrl).c_str(), aHubUrl.length());
	// Construct hybrid CID from the bits of the tiger hash - should be
	// fairly random, and hopefully low-collision
	return CID(th.finalize());
}

void ClientManager::putOnline(OnlineUser* ou) noexcept {
	{
		Lock l(cs);
		onlineUsers.insert(make_pair(const_cast<CID*>(&ou->getUser()->getCID()), ou));
	}
	
	if(!ou->getUser()->isOnline()) {
		ou->getUser()->setFlag(User::ONLINE);
		ou->getIdentity().set("LT", Util::toString(GET_TIME()));
		fire(ClientManagerListener::UserConnected(), ou->getUser());
	}
}

void ClientManager::putOffline(OnlineUser* ou, bool disconnect) noexcept {
	bool lastUser = false;
	{
		Lock l(cs);
		OnlinePair op = onlineUsers.equal_range(const_cast<CID*>(&ou->getUser()->getCID()));
		dcassert(op.first != op.second);
		for(OnlineIter i = op.first; i != op.second; ++i) {
			OnlineUser* ou2 = i->second;
			if(ou == ou2) {
				lastUser = (distance(op.first, op.second) == 1);
				onlineUsers.erase(i);
				break;
			}
		}
	}

	if(lastUser) {
		UserPtr& u = ou->getUser();
		u->unsetFlag(User::ONLINE);
		updateNick(*ou);
		if(disconnect)
			ConnectionManager::getInstance()->disconnect(u);
		fire(ClientManagerListener::UserDisconnected(), u);
	}
}

OnlineUser* ClientManager::findOnlineUserHint(const CID& cid, const string& hintUrl, OnlinePairC& p) const {
	p = onlineUsers.equal_range(const_cast<CID*>(&cid));
	if(p.first == p.second) // no user found with the given CID.
		return 0;

	if(!hintUrl.empty()) {
		for(auto i = p.first; i != p.second; ++i) {
			OnlineUser* u = i->second;
			if(u->getClientBase().getHubUrl() == hintUrl) {
				return u;
			}
		}
	}

	return 0;
}

OnlineUser* ClientManager::findOnlineUser(const HintedUser& user, bool priv) const {
	return findOnlineUser(user.user->getCID(), user.hint, priv);
}

OnlineUser* ClientManager::findOnlineUser(const CID& cid, const string& hintUrl, bool priv) const {
	OnlinePairC p;
	OnlineUser* u = findOnlineUserHint(cid, hintUrl, p);
	if(u) // found an exact match (CID + hint).
		return u;

	if(p.first == p.second) // no user found with the given CID.
		return 0;

	// if the hint hub is private, don't allow connecting to the same user from another hub.
	if(priv)
		return 0;

	// ok, hub not private, return a random user that matches the given CID but not the hint.
	return p.first->second;
}

void ClientManager::connect(const HintedUser& user, const string& token) {
	bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);

	Lock l(cs);
	OnlineUser* u = findOnlineUser(user, priv);

	if(u) {
		u->getClientBase().connect(*u, token);
	}
}

void ClientManager::privateMessage(const HintedUser& user, const string& msg, bool thirdPerson) {
	bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);

	Lock l(cs);
	OnlineUser* u = findOnlineUser(user, priv);
	
	if(u && !PluginManager::getInstance()->runHook(HOOK_CHAT_PM_OUT, u, msg)) {
		u->getClientBase().privateMessage(u, msg, thirdPerson);
	}
}

void ClientManager::userCommand(const HintedUser& user, const UserCommand& uc, StringMap& params, bool compatibility) {
	Lock l(cs);
	/** @todo we allow wrong hints for now ("false" param of findOnlineUser) because users
	 * extracted from search results don't always have a correct hint; see
	 * SearchManager::onRES(const AdcCommand& cmd, ...). when that is done, and SearchResults are
	 * switched to storing only reliable HintedUsers (found with the token of the ADC command),
	 * change this call to findOnlineUserHint. */
	OnlineUser* ou = findOnlineUser(user.user->getCID(), user.hint.empty() ? uc.getHub() : user.hint, false);
	if(!ou || ou->getClientBase().type == ClientBase::DHT)
		return;

	const string& opChat = ou->getClient().getOpChat();
	if(opChat.find("*") == string::npos && opChat.find("?") == string::npos)
		params["opchat"] = opChat;

	ou->getIdentity().getParams(params, "user", compatibility);
	ou->getClient().getHubIdentity().getParams(params, "hub", false);
	ou->getClient().getMyIdentity().getParams(params, "my", compatibility);
	ou->getClient().escapeParams(params);
	ou->getClient().sendUserCmd(uc, params);
}

void ClientManager::send(AdcCommand& cmd, const CID& cid) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&cid));
	if(i != onlineUsers.end()) {
		OnlineUser& u = *i->second;
		if(cmd.getType() == AdcCommand::TYPE_UDP && !u.getIdentity().isUdpActive()) {
			if(u.getUser()->isNMDC() || u.getClientBase().getType() == Client::DHT)
				return;
			cmd.setType(AdcCommand::TYPE_DIRECT);
			cmd.setTo(u.getIdentity().getSID());
			u.getClient().send(cmd);
		} else {
			sendUDP(u.getIdentity().getIp(), u.getIdentity().getUdpPort(), cmd.toString(getMe()->getCID()));
		}
	}
}

void ClientManager::sendUDP(const string& ip, const string& port, const string& data) {
	if(PluginManager::getInstance()->onUDP(true, ip, port, data))
		return;

	try {
		Socket udp;
		udp.writeTo(ip, static_cast<uint16_t>(Util::toInt(port)), data);
	} catch(const SocketException&) {
		dcdebug("Socket exception when sending UDP data to %s:%s\n", ip.c_str(), port.c_str());
	}
}

void ClientManager::infoUpdated() {
	Lock l(cs);
	for(auto i = clients.cbegin(); i != clients.cend(); ++i) {
		Client* c = i->second;
		if(c->isConnected()) {
			c->info(false);
		}
	}
}

void ClientManager::on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize, 
									int aFileType, const string& aString, bool isPassive) noexcept
{
	fire(ClientManagerListener::IncomingSearch(), aString);

	auto l = ShareManager::getInstance()->search(aString, aSearchType, aSize, aFileType, isPassive ? 5 : 10);
	if(l.size() > 0) {
		if(isPassive) {
			string name = aSeeker.substr(4);
			// Good, we have a passive seeker, those are easier...
			string str;
			for(SearchResultList::const_iterator i = l.begin(); i != l.end(); ++i) {
				const SearchResultPtr& sr = *i;
				str += sr->toSR(*aClient);
				str[str.length()-1] = 5;
				str += Text::fromUtf8(name, *(aClient->getEncoding()));
				str += '|';
			}
			
			if(str.size() > 0)
				aClient->send(str);
			
		} else {
			string ip, port;
			auto ipPortPair = NmdcHub::parseIpPort(aSeeker);

			ip = Socket::resolve(ipPortPair.first);
			port = ipPortPair.second;

			if(port.empty())
				port = "412";

			for(const auto& sr: l) {
				sendUDP(ip, port, sr->toSR(*aClient));
 			}
		}
	} else if(!isPassive && (aFileType == SearchManager::TYPE_TTH) && (aString.compare(0, 4, "TTH:") == 0)) {
		QueueItem::PartsInfo partialInfo;
		TTHValue aTTH(aString.substr(4));
		if(!QueueManager::getInstance()->handlePartialSearch(aTTH, partialInfo))
			return;
		
		string ip, file, proto, query, fragment;
		uint16_t port = 0;
		Util::decodeUrl(aSeeker, proto, ip, port, file, query, fragment);
		
		try {
			AdcCommand cmd = SearchManager::getInstance()->toPSR(true, aClient->getMyNick(), aClient->getIpPort(), aTTH.toBase32(), partialInfo);
			Socket s;
			s.writeTo(Socket::resolve(ip), port, cmd.toString(ClientManager::getInstance()->getMe()->getCID()));
		} catch(...) {
			dcdebug("Partial search caught error\n");		
		}
	}
}

void ClientManager::on(AdcSearch, const Client* c, const AdcCommand& adc, const CID& from) noexcept {
	bool isUdpActive = false;
	{
		Lock l(cs);
		
		OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&from));
		for(OnlineIterC i = op.first; i != op.second; ++i) {
			const OnlineUserPtr& u = i->second;
			if(&u->getClient() == c)
			{
				isUdpActive = u->getIdentity().isUdpActive();
				break;
			}
		}			
	}
	SearchManager::getInstance()->respond(adc, from, isUdpActive, c->getIpPort());
}

void ClientManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, void* aOwner) {
	if(BOOLSETTING(USE_DHT) && aFileType == SearchManager::TYPE_TTH)
		dht::DHT::getInstance()->findFile(aString);
		
	Lock l(cs);

	for(auto i = clients.cbegin(); i != clients.cend(); ++i) {
		Client* c = i->second;
		if(c->isConnected()) {
			c->search(aSizeMode, aSize, aFileType, aString, aToken, StringList() /*ExtList*/, aOwner);
		}
	}
}

uint64_t ClientManager::search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, void* aOwner) {
	if(BOOLSETTING(USE_DHT) && aFileType == SearchManager::TYPE_TTH)
		dht::DHT::getInstance()->findFile(aString, aToken);

	Lock l(cs);

	uint64_t estimateSearchSpan = 0;
	
	for(StringIter it = who.begin(); it != who.end(); ++it) {
		const string& client = *it;
		
		auto i = clients.find(const_cast<string*>(&client));
		if(i != clients.end() && i->second->isConnected()) {
			uint64_t ret = i->second->search(aSizeMode, aSize, aFileType, aString, aToken, aExtList, aOwner);
			estimateSearchSpan = max(estimateSearchSpan, ret);			
		}
	}
	
	return estimateSearchSpan;
}

void ClientManager::on(TimerManagerListener::Minute, uint64_t /*aTick*/) noexcept {
	Lock l(cs);

	// Collect some garbage...
	UserIter i = users.begin();
	while(i != users.end()) {
		if(i->second->unique()) {
			NickMap::iterator n = nicks.find(const_cast<CID*>(&i->second->getCID()));
			if(n != nicks.end()) nicks.erase(n);
			users.erase(i++);
		} else {
			++i;
		}
	}

	for(auto j = clients.cbegin(); j != clients.cend(); ++j) {
		j->second->info(false);
	}
}

UserPtr& ClientManager::getMe() {
	if(!me) {
		Lock l(cs);
		if(!me) {
			me = new User(getMyCID());
			users.insert(make_pair(const_cast<CID*>(&me->getCID()), me));
		}
	}
	return me;
}

const CID& ClientManager::getMyPID() {
	if(pid.isZero())
		pid = CID(SETTING(PRIVATE_ID));
	return pid;
}

CID ClientManager::getMyCID() {
	TigerHash tiger;
	tiger.update(getMyPID().data(), CID::SIZE);
	return CID(tiger.finalize());
}

void ClientManager::updateNick(const OnlineUser& user) noexcept {
	updateNick(user.getUser(), user.getIdentity().getNick());
}

void ClientManager::updateNick(const UserPtr& user, const string& nick) noexcept {
	if(!nick.empty()) {
		Lock l(cs);
		auto i = nicks.find(const_cast<CID*>(&user->getCID()));
		if(i == nicks.end()) {
			nicks[const_cast<CID*>(&user->getCID())] = nick;
		} else {
			i->second = nick;
		}
	}
}

string ClientManager::getMyNick(const string& hubUrl) const {
	Lock l(cs);
	auto i = clients.find(const_cast<string*>(&hubUrl));
	if(i != clients.end()) {
		return i->second->getMyIdentity().getNick();
	}
	return Util::emptyString;
}
	
int ClientManager::getMode(const string& aHubUrl) const {
	
	if(aHubUrl.empty()) 
		return SETTING(INCOMING_CONNECTIONS);

	int mode = 0;
	const FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(aHubUrl);
	if(hub) {
		switch(hub->getMode()) {
			case 1 :
				mode = SettingsManager::INCOMING_DIRECT;
				break;
			case 2 :
				mode = SettingsManager::INCOMING_FIREWALL_PASSIVE;
				break;
			default:
				mode = SETTING(INCOMING_CONNECTIONS);
		}
	} else {
		mode = SETTING(INCOMING_CONNECTIONS);
	}
	return mode;
}

void ClientManager::cancelSearch(void* aOwner) {
	Lock l(cs);

	for(auto i = clients.cbegin(); i != clients.cend(); ++i) {
		i->second->cancelSearch(aOwner);
	}
}

OnlineUserPtr ClientManager::findDHTNode(const CID& cid) const
{
	Lock l(cs);
	
	OnlinePairC op = onlineUsers.equal_range(const_cast<CID*>(&cid));
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		OnlineUser* ou = i->second;
		
		// user not in DHT, so don't bother with other hubs
		if(!ou->getUser()->isSet(User::DHT))
			break;

		if(ou->getClientBase().getType() == Client::DHT)
			return ou;
	}

	return NULL;
}

void ClientManager::on(Connected, const Client* c) noexcept {
	fire(ClientManagerListener::ClientConnected(), c);
}

void ClientManager::on(UserUpdated, const Client*, const OnlineUserPtr& user) noexcept {
	fire(ClientManagerListener::UserUpdated(), *user);
}

void ClientManager::on(UsersUpdated, const Client*, const OnlineUserList& l) noexcept {
	for(OnlineUserList::const_iterator i = l.begin(), iend = l.end(); i != iend; ++i) {
		updateNick(*(*i));
		fire(ClientManagerListener::UserUpdated(), *(*i)); 
	}
}

void ClientManager::on(HubUpdated, const Client* c) noexcept {
	fire(ClientManagerListener::ClientUpdated(), c);
}

void ClientManager::on(Failed, const Client* client, const string&) noexcept {
	fire(ClientManagerListener::ClientDisconnected(), client);
}

void ClientManager::on(HubUserCommand, const Client* client, int aType, int ctx, const string& name, const string& command) noexcept {
	if(BOOLSETTING(HUB_USER_COMMANDS)) {
		if(aType == UserCommand::TYPE_REMOVE) {
			int cmd = FavoriteManager::getInstance()->findUserCommand(name, client->getHubUrl());
			if(cmd != -1)
				FavoriteManager::getInstance()->removeUserCommand(cmd);
		} else if(aType == UserCommand::TYPE_CLEAR) {
 			FavoriteManager::getInstance()->removeHubUserCommands(ctx, client->getHubUrl());
 		} else {
			FavoriteManager::getInstance()->addUserCommand(aType, ctx, UserCommand::FLAG_NOSAVE, name, command, "", client->getHubUrl());
		}
	}
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
