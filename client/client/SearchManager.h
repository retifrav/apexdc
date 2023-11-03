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

#ifndef DCPLUSPLUS_DCPP_SEARCH_MANAGER_H
#define DCPLUSPLUS_DCPP_SEARCH_MANAGER_H

#include "Socket.h"
#include "Thread.h"
#include "Semaphore.h"
#include "Singleton.h"

#include "SearchManagerListener.h"
#include "SettingsManager.h"
#include "TimerManager.h"
#include "ClientManager.h"
#include "ResourceManager.h"
#include "QueueItem.h"

#include "AdcCommand.h"

namespace dcpp {

class SocketException;

class SearchManager : public Speaker<SearchManagerListener>, public Singleton<SearchManager>, public Thread, private SettingsManagerListener
{
public:
	enum SizeModes {
		SIZE_DONTCARE = 0x00,
		SIZE_ATLEAST = 0x01,
		SIZE_ATMOST = 0x02,
		SIZE_EXACT = 0x03
	};

	enum TypeModes {
		TYPE_ANY = 0,
		TYPE_AUDIO,
		TYPE_COMPRESSED,
		TYPE_DOCUMENT,
		TYPE_EXECUTABLE,
		TYPE_PICTURE,
		TYPE_VIDEO,
		TYPE_DIRECTORY,
		TYPE_TTH,
		TYPE_LAST
	};
private:
	static const char* types[TYPE_LAST];
public:
	static const char* getTypeStr(int type);

	bool addSearchToHistory(const tstring& search);
	std::set<tstring>& getSearchHistory();
	void setSearchHistory(const std::set<tstring>& list);
	void clearSearchHistory();

	void search(const string& aName, int64_t aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, void* aOwner = NULL);
	void search(const string& aName, const string& aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, void* aOwner = NULL) {
		search(aName, Util::toInt64(aSize), aTypeMode, aSizeMode, aToken, aOwner);
	}

	uint64_t search(StringList& who, const string& aName, int64_t aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, const StringList& aExtList, void* aOwner = NULL);
	uint64_t search(StringList& who, const string& aName, const string& aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, const StringList& aExtList, void* aOwner = NULL) {
		return search(who, aName, Util::toInt64(aSize), aTypeMode, aSizeMode, aToken, aExtList, aOwner);
 	}
	
	void respond(const AdcCommand& cmd, const CID& cid, bool isUdpActive, const string& hubIpPort);

	uint16_t getPort() const
	{
		return port;
	}

	void listen();
	void disconnect() noexcept;

	void onData(const string& data, const string& remoteIp = Util::emptyString);
	void onSR(const string& x, const string& remoteIP = Util::emptyString);
	void onRES(const AdcCommand& cmd, const UserPtr& from, const string& remoteIp = Util::emptyString);
	void onPSR(const AdcCommand& cmd, UserPtr from, const string& remoteIp = Util::emptyString);
	AdcCommand toPSR(bool wantResponse, const string& myNick, const string& hubIpPort, const string& tth, const vector<uint16_t>& partialInfo) const;

private:
	class UdpQueue: public Thread, private CommandHandler<UdpQueue> {
	public:
		UdpQueue() : stop(false) {}
		~UdpQueue() { shutdown(); }

		int run();

		void shutdown() {
			stop = true;
			s.signal();
		}

		void addResult(const string& buf, const string& ip) {
			{
				Lock l(cs);
				resultList.push_back(make_pair(buf, ip));
			}
			s.signal();
		}

	private:
		friend class CommandHandler<UdpQueue>;

		void handle(AdcCommand::RES, AdcCommand& c, const string& remoteIp) noexcept;
		void handle(AdcCommand::PSR, AdcCommand& c, const string& remoteIp) noexcept;

		// Ignore any other ADC commands for now
		template<typename T> void handle(T, AdcCommand&, const string&) { }

		CriticalSection cs;
		Semaphore s;
		
		deque<pair<string, string> > resultList;
		
		bool stop;
	} queue;

	CriticalSection cs;
	std::unique_ptr<Socket> socket;
	uint16_t port;
	bool stop;

	std::set<tstring> searchHistory;

	friend class Singleton<SearchManager>;

	SearchManager();
	~SearchManager();

	static std::string normalizeWhitespace(const std::string& aString);

	int run();

	string getPartsString(const QueueItem::PartsInfo& partsInfo) const;

	void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept;
	void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept;
};

} // namespace dcpp

#endif // !defined(SEARCH_MANAGER_H)

/**
 * @file
 * $Id$
 */
