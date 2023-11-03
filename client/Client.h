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

#ifndef DCPLUSPLUS_DCPP_CLIENT_H
#define DCPLUSPLUS_DCPP_CLIENT_H

#include "compiler.h"

#include "forward.h"

#include "Speaker.h"
#include "BufferedSocketListener.h"
#include "TimerManager.h"
#include "ClientListener.h"
#include "DebugManager.h"
#include "SearchQueue.h"
#include "PluginEntity.h"

#include "atomic.h"

#include "OnlineUser.h"

namespace dcpp {

using std::max;

class ClientBase
{
public:
	
	ClientBase() : type(DIRECT_CONNECT) { }

	enum P2PType { DIRECT_CONNECT, ADC, NMDC, DHT };
	P2PType type;
	
	P2PType getType() const { return type; }

	virtual const string& getHubUrl() const = 0;
	virtual string getHubName() const = 0;
	virtual bool isOp() const = 0;
	virtual void connect(const OnlineUser& user, const string& token) = 0;
	virtual void privateMessage(const OnlineUserPtr& user, const string& aMessage, bool thirdPerson = false) = 0;
	
};

/** Yes, this should probably be called a Hub */
class Client : public ClientBase, public PluginEntity<HubData>, public Speaker<ClientListener>, public BufferedSocketListener, protected TimerManagerListener {
public:
	virtual void connect(const string& reference = Util::emptyString);
	virtual void disconnect(bool graceless);

	virtual void connect(const OnlineUser& user, const string& token) = 0;
	virtual void hubMessage(const string& aMessage, bool thirdPerson = false) = 0;
	virtual void privateMessage(const OnlineUserPtr& user, const string& aMessage, bool thirdPerson = false) = 0;
	virtual void sendUserCmd(const UserCommand& command, const StringMap& params) = 0;

	uint64_t search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, void* owner);
	void cancelSearch(void* aOwner) { searchQueue.cancelSearch(aOwner); }
	
	virtual void password(const string& pwd) = 0;
	virtual void info(bool force) = 0;

	virtual size_t getUserCount() const = 0;
	int64_t getAvailable() const { return availableBytes; };
	
	virtual void emulateCommand(const string& cmd) = 0;
	virtual void send(const AdcCommand& command) = 0;

	virtual string escape(string const& str) const { return str; }

	bool isConnected() const { return state != STATE_DISCONNECTED; }
	bool isReady() const { return state != STATE_CONNECTING && state != STATE_DISCONNECTED; }
	bool isSecure() const;
	bool isTrusted() const;
	string getCipherName() const;

	ByteVector getKeyprint() const;
	string getKeyprintString() const;

	bool isOp() const { return getMyIdentity().isOp(); }

	virtual void refreshUserList(bool) = 0;
	virtual void getUserList(OnlineUserList& list) const = 0;
	virtual OnlineUserPtr findUser(const string& aNick) const = 0;
	
	uint16_t getPort() const { return port; }
	const string& getAddress() const { return address; }

	const string& getIp() const { return ip; }
	string getIpPort() const { return getIp() + ':' + Util::toString(port); }
	string getLocalIp() const;

	void updated(const OnlineUserPtr& aUser);

	static int getTotalCounts() {
		return counts[COUNT_NORMAL] + counts[COUNT_REGISTERED] + counts[COUNT_OP];
	}

	static string getCounts() {
		char buf[128];
		return string(buf, snprintf(buf, sizeof(buf), "%ld/%ld/%ld",
				counts[COUNT_NORMAL].load(), counts[COUNT_REGISTERED].load(), counts[COUNT_OP].load()));
	}

	
	StringMap& escapeParams(StringMap& sm) {
		for(StringMapIter i = sm.begin(); i != sm.end(); ++i) {
			i->second = escape(i->second);
		}
		return sm;
	}
	
	void setSearchInterval(uint32_t aInterval) {
		// min interval is 10 seconds
		searchQueue.interval = max(aInterval + 2000, (uint32_t)(10 * 1000));
	}

	uint32_t getSearchInterval() const {
		return searchQueue.interval;
	}	

	void cheatMessage(const string& msg, uint32_t flag = 0) {
		fire(ClientListener::CheatMessage(), this, msg, flag);
	}
	void reportUser(const Identity& i) {
		fire(ClientListener::UserReport(), this, i);
	}
	
	void reconnect();
	void shutdown();
	void loadSettings(const string& aUrl, bool updateNick);
	bool isActive() const;

	void send(const string& aMessage) { send(aMessage.c_str(), aMessage.length()); }
	void send(const char* aMessage, size_t aLen);

	HubData* getPluginObject() noexcept;

	string getMyNick() const { return getMyIdentity().getNick(); }
	string getHubName() const { return getHubIdentity().getNick().empty() ? getHubUrl() : getHubIdentity().getNick(); }
	string getHubDescription() const { return getHubIdentity().getDescription(); }

	Identity& getHubIdentity() { return hubIdentity; }

	const string& getHubUrl() const { return hubUrl; }

	GETSET(Identity, myIdentity, MyIdentity);
	GETSET(Identity, hubIdentity, HubIdentity);

	GETSET(string, defpassword, Password);
	
	GETSET(string, currentNick, CurrentNick);
	GETSET(string, currentDescription, CurrentDescription);
	GETSET(string, currentEmail, CurrentEmail);

	GETSET(string, localHubName, LocalHubName);
	GETSET(string, localHubDescription, LocalHubDescription);

	GETSET(string, opChat, OpChat);
	GETSET(string, protectedPrefixes, ProtectedPrefixes);
	GETSET(string, favIp, FavIp);
	
	GETSET(string, hubChats, HubChats);

	GETSET(uint64_t, lastActivity, LastActivity);
	GETSET(uint32_t, reconnDelay, ReconnDelay);
	GETSET(uint32_t, uniqueId, UniqueId);
	
	GETSET(string*, encoding, Encoding);	
		
	GETSET(bool, registered, Registered);
	GETSET(bool, autoReconnect, AutoReconnect);
	GETSET(bool, stealth, Stealth);
	GETSET(bool, hideShare, HideShare);
	GETSET(bool, exclChecks, ExclChecks);
	GETSET(bool, autoOpenOpChat, AutoOpenOpChat);
	GETSET(bool, logChat, LogChat);

	virtual bool startChecking() = 0;
	virtual void stopChecking() = 0;
	
	mutable CriticalSection cs;

protected:
	friend class ClientManager;
	Client(const string& hubURL, char separator, bool secure_);
	virtual ~Client();

	// Flags for RDEX
	enum RedirectFlags {
		REDIRECT_MANUAL			= 0x00,
		REDIRECT_ADC			= 0x01,
		REDIRECT_ADCS			= 0x02,
		REDIRECT_NMDC			= 0x04,
		REDIRECT_NMDCS			= 0x08,
		REDIRECT_ALL			= REDIRECT_ADC | REDIRECT_ADCS | REDIRECT_NMDC | REDIRECT_NMDCS
	};

	uint32_t redirectFlags;

	enum CountType {
		COUNT_NORMAL,
		COUNT_REGISTERED,
		COUNT_OP,
		COUNT_UNCOUNTED,
	};

	static atomic<long> counts[COUNT_UNCOUNTED];

	enum States {
		STATE_CONNECTING,	///< Waiting for socket to connect
		STATE_PROTOCOL,		///< Protocol setup
		STATE_IDENTIFY,		///< Nick setup
		STATE_VERIFY,		///< Checking password
		STATE_NORMAL,		///< Running
		STATE_DISCONNECTED,	///< Nothing in particular
	} state;

	SearchQueue searchQueue;
	BufferedSocket* sock;

	int64_t availableBytes;

	void updateCounts(bool aRemove);
	void updateActivity() { lastActivity = GET_TICK(); }

	/** Reload details from favmanager or settings */
	void reloadSettings(bool updateNick) { loadSettings(hubUrl, updateNick); }

	virtual string checkNick(const string& nick) = 0;
	virtual void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList) = 0;

	const string selectRedirect(StringList& uriSet);

	// TimerManagerListener
	virtual void on(Second, uint64_t aTick) noexcept;
	// BufferedSocketListener
	virtual void on(Connecting) noexcept { fire(ClientListener::Connecting(), this); }
	virtual void on(Connected) noexcept;
	virtual void on(Line, const string& aLine) noexcept;
	virtual void on(Failed, const string&) noexcept;

private:

	Client(const Client&);
	Client& operator=(const Client&);

	string hubUrl;
	string address;
	string ip;
	string localIp;
	string keyprint;

	uint16_t port;
	char separator;
	bool secure;
	CountType countType;
};

} // namespace dcpp

#endif // !defined(CLIENT_H)

/**
 * @file
 * $Id$
 */
