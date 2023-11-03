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
#include "Client.h"

#include "BufferedSocket.h"

#include "FavoriteManager.h"
#include "TimerManager.h"
#include "ResourceManager.h"
#include "ClientManager.h"
#include "UploadManager.h"
#include "ThrottleManager.h"
#include "PluginManager.h"

namespace dcpp {

atomic<long> Client::counts[COUNT_UNCOUNTED];

uint32_t idCounter = 0;

Client::Client(const string& hubURL, char separator_, bool secure_) : 
	myIdentity(ClientManager::getInstance()->getMe(), 0), uniqueId(++idCounter),
	reconnDelay(120), lastActivity(GET_TICK()), registered(false), autoReconnect(false),
	encoding(const_cast<string*>(&Text::systemCharset)), state(STATE_DISCONNECTED), sock(0),
	hubUrl(hubURL), port(0), separator(separator_),
	secure(secure_), countType(COUNT_UNCOUNTED), availableBytes(0), redirectFlags(REDIRECT_ALL)
{
	string file, proto, query, fragment;
	Util::decodeUrl(hubURL, proto, address, port, file, query, fragment);

	if(!query.empty()) {
		auto q = Util::decodeQuery(query);
		auto kp = q.find("kp");
		if(kp != q.end()) {
			keyprint = kp->second;
		}
	}

	TimerManager::getInstance()->addListener(this);
}

Client::~Client() {
	dcassert(!sock);
	
	// In case we were deleted before we Failed
	FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
	TimerManager::getInstance()->removeListener(this);
	updateCounts(true);
}

void Client::reconnect() {
	disconnect(true);
	setAutoReconnect(true);
	setReconnDelay(0);
}

void Client::shutdown() {
	if(sock) {
		BufferedSocket::putSocket(sock);
		sock = 0;
	}
}

void Client::loadSettings(const string& aUrl, bool updateNick) {
	const FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(aUrl);
	if(hub) {
		if(updateNick) {
			setCurrentNick(checkNick(hub->getNick(true)));
		}

		string speedDescription = Util::emptyString;
		if(!hub->getStealth() && type != ClientBase::ADC) {
			if(BOOLSETTING(SHOW_DESCRIPTION_LIMIT) && ThrottleManager::getInstance()->getUpLimit() != 0)
				speedDescription += "[L:" + Util::formatBytes(ThrottleManager::getInstance()->getUpLimit() * 1024) + "/s]";
		}
		if(!hub->getUserDescription().empty()) {
			setCurrentDescription(speedDescription + hub->getUserDescription());
		} else {
			setCurrentDescription(speedDescription + SETTING(DESCRIPTION));
		}
		if(!hub->getEmail().empty()) {
			setCurrentEmail(hub->getEmail());
		} else {
			setCurrentEmail(SETTING(EMAIL));
		}
		if(!hub->getName().empty())
			setLocalHubName(hub->getName());
		if(!hub->getDescription().empty())
			setLocalHubDescription(hub->getDescription());
		if(!hub->getPassword().empty())
			setPassword(hub->getPassword());
		setStealth((hub->getStealth() && type != ClientBase::ADC));
		setFavIp(hub->getIP());
		setHideShare(hub->getHideShare());
		setExclChecks(hub->getExclChecks());
		setAutoOpenOpChat(hub->getAutoOpenOpChat());
		setLogChat(hub->getLogChat() || BOOLSETTING(LOG_MAIN_CHAT));
		setOpChat(hub->getHubChats().find(";") ? hub->getHubChats().substr(0, hub->getHubChats().find(";")) : hub->getHubChats());
		setHubChats(hub->getHubChats());
		setProtectedPrefixes(hub->getProtectedPrefixes().empty() ? SETTING(PROT_USERS) : (SETTING(PROT_USERS).empty() ? hub->getProtectedPrefixes() : SETTING(PROT_USERS) + ";" + hub->getProtectedPrefixes()));
		
		if(!hub->getEncoding().empty())
			setEncoding(const_cast<string*>(&hub->getEncoding()));
		
		if(hub->getSearchInterval() < 10)
			setSearchInterval(SETTING(MINIMUM_SEARCH_INTERVAL) * 1000);
		else
			setSearchInterval(hub->getSearchInterval() * 1000);
	} else {
		if(updateNick) {
			setCurrentNick(checkNick(SETTING(NICK)));
		}
		setCurrentDescription(SETTING(DESCRIPTION));
		setCurrentEmail(SETTING(EMAIL));
		setStealth(false);
		setFavIp(Util::emptyString);
		setHideShare(false);
		setExclChecks(false);
		setAutoOpenOpChat(false);
		setLogChat(BOOLSETTING(LOG_MAIN_CHAT));
		setOpChat(Util::emptyString);
		setHubChats(Util::emptyString);
		setProtectedPrefixes(SETTING(PROT_USERS));
		setSearchInterval(SETTING(MINIMUM_SEARCH_INTERVAL) * 1000);
	}

	if(getCurrentDescription().length() > 35 && type == ClientBase::NMDC)
		setCurrentDescription(getCurrentDescription().substr(0, 35));
}

bool Client::isActive() const {
	return ClientManager::getInstance()->isActive(hubUrl);
}

void Client::connect(const string& reference /*= Util::emptyString*/) {
	if(sock)
		BufferedSocket::putSocket(sock);

	availableBytes = 0;

	setAutoReconnect(true);
	setReconnDelay(120 + Util::rand(0, 60));
	loadSettings(reference.empty() ? hubUrl : reference, true);
	setRegistered(false);
	setMyIdentity(Identity(ClientManager::getInstance()->getMe(), 0));
	setHubIdentity(Identity());

	if(!reference.empty() && type == ClientBase::NMDC) {
		// Going from ADC to NMDC's plain text passwords so do not auto send it, for this session.
		// Since reference is used for permanent redirects completely wiping it would be a bad idea.
		if(strnicmp("adc://", reference.c_str(), 6) == 0 || strnicmp("adcs://", reference.c_str(), 7) == 0) {
			fire(ClientListener::StatusMessage(), this, STRING(WARN_SECURE_PASSWORD_REVEAL), ClientListener::FLAG_NORMAL);
			setPassword(Util::emptyString);
			setAutoReconnect(false);
		}
	}

	state = STATE_CONNECTING;

	try {
		sock = BufferedSocket::getSocket(separator);
		sock->addListener(this);
		sock->connect(address, port, secure, BOOLSETTING(ALLOW_UNTRUSTED_HUBS), true, keyprint);
	} catch(const Exception& e) {
		shutdown();
		state = STATE_DISCONNECTED;
		fire(ClientListener::Failed(), this, e.getError());
	}
	updateActivity();
}

void Client::send(const char* aMessage, size_t aLen) {
	if(!isReady())
		return;

	if(PluginManager::getInstance()->runHook(HOOK_NETWORK_HUB_OUT, this, aMessage))
		return;

	updateActivity();
	sock->write(aMessage, aLen);
	COMMAND_DEBUG(aMessage, DebugManager::HUB_OUT, getIpPort());
}

HubData* Client::getPluginObject() noexcept {
	resetEntity();

	pod.url = pluginString(hubUrl);
	pod.ip = pluginString(ip);
	pod.object = this;
	pod.port = port;
	pod.protocol = (type == ClientBase::ADC) ? PROTOCOL_ADC : PROTOCOL_NMDC;
	pod.isOp = isOp() ? True : False;
	pod.isSecure = isSecure() ? True : False;

	return &pod;
}

const string Client::selectRedirect(StringList& uriSet) {
	string destination;

	// first partition to get a matching range, then choose at random
	auto findURI = [&uriSet](const string& proto, string& dest) {
		dest.clear();
		auto end = std::partition(uriSet.begin(), uriSet.end(), [&proto](const string& uri) { return strnicmp(uri.c_str(), proto.c_str(), proto.size()) == 0; });
		if(end != uriSet.begin())
			dest = uriSet[Util::rand(std::distance(uriSet.begin(), end))];
		return !dest.empty();
	};

	// Encrypted over unencrypted and ADC over NMDC
	if((redirectFlags & REDIRECT_ADCS) && findURI("adcs://", destination))
		return destination;
	if((redirectFlags & REDIRECT_ADC) && findURI("adc://", destination))
		return destination;
	if((redirectFlags & REDIRECT_NMDCS) && findURI("nmdcs://", destination))
		return destination;
	if((redirectFlags & REDIRECT_NMDC) && findURI("dchub://", destination))
		return destination;

	return destination;
}

void Client::on(Connected) noexcept {
	updateActivity(); 
	ip = sock->getIp();
	localIp = sock->getLocalIp();

	fire(ClientListener::Connected(), this);
	state = STATE_PROTOCOL;
}

void Client::on(Failed, const string& aLine) noexcept {
	state = STATE_DISCONNECTED;
	FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
	sock->removeListener(this);
	fire(ClientListener::Failed(), this, aLine);
}

void Client::disconnect(bool graceLess) {
	if(sock) 
		sock->disconnect(graceLess);
}

bool Client::isSecure() const {
	return isReady() && sock->isSecure();
}

bool Client::isTrusted() const {
	return isReady() && sock->isTrusted();
}

string Client::getCipherName() const {
	return isReady() ? sock->getCipherName() : Util::emptyString;
}

ByteVector Client::getKeyprint() const {
	return isReady() ? sock->getKeyprint() : ByteVector();
}

string Client::getKeyprintString() const {
	if(isReady() && sock->isSecure()) {
		auto kp = sock->getKeyprint();
		if(!kp.empty())
			return "SHA256/" + Encoder::toBase32(&kp[0], kp.size());
	}
	return Util::emptyString;
}

void Client::updateCounts(bool aRemove) {
	// We always remove the count and then add the correct one if requested...
	if(countType != COUNT_UNCOUNTED) {
		--counts[countType];
		countType = COUNT_UNCOUNTED;
	}

	if(!aRemove) {
		if(getMyIdentity().isOp()) {
			countType = COUNT_OP;
		} else if(getMyIdentity().isRegistered()) {
			countType = COUNT_REGISTERED;
		} else {
			countType = COUNT_NORMAL;
		}
		++counts[countType];
	}
}

void Client::updated(const OnlineUserPtr& aUser) {
	fire(ClientListener::UserUpdated(), this, aUser);
}

string Client::getLocalIp() const {
	// Favorite hub Ip
	if(!getFavIp().empty())
		return Socket::resolve(getFavIp());

	// Best case - the server detected it
	if((!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty()) && !getMyIdentity().getIp().empty()) {
		return getMyIdentity().getIp();
	}

	if(!SETTING(EXTERNAL_IP).empty()) {
		return Socket::resolve(SETTING(EXTERNAL_IP));
	}

	if(localIp.empty()) {
		return Util::getLocalIp();
	}

	return localIp;
}

uint64_t Client::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, void* owner){
	dcdebug("Queue search %s\n", aString.c_str());

	if(searchQueue.interval) {
		Search s;
		s.fileType = aFileType;
		s.size     = aSize;
		s.query    = aString;
		s.sizeType = aSizeMode;
		s.token    = aToken;
		s.exts	   = aExtList;
		s.owners.insert(owner);

		searchQueue.add(s);

		return searchQueue.getSearchTime(owner) - GET_TICK();
	}

	search(aSizeMode, aSize, aFileType , aString, aToken, aExtList);
	return 0;

}
 
void Client::on(Line, const string& aLine) noexcept {
	updateActivity();
	COMMAND_DEBUG(aLine, DebugManager::HUB_IN, getIpPort());
}

void Client::on(Second, uint64_t aTick) noexcept {
	if(state == STATE_DISCONNECTED && getAutoReconnect() && (aTick > (getLastActivity() + getReconnDelay() * 1000)) ) {
		// Try to reconnect...
		connect();
	}

	if(!searchQueue.interval) return;

	if(isConnected()){
		Search s;
		
		if(searchQueue.pop(s)){
			search(s.sizeType, s.size, s.fileType , s.query, s.token, s.exts);
		}
	}
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
