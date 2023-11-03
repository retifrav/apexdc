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

#ifndef DCPLUSPLUS_DCPP_HUBENTRY_H_
#define DCPLUSPLUS_DCPP_HUBENTRY_H_

#include <string>

#include "SettingsManager.h"
#include "Util.h"
#include "StringTokenizer.h"

namespace dcpp {

using std::string;

class HubEntry {
public:
	typedef vector<HubEntry> List;
	
	HubEntry(const string& aName, const string& aServer, const string& aDescription, const string& aUsers) : 
	name(aName), server(aServer), description(aDescription), country(Util::emptyString), 
	rating(Util::emptyString), reliability(0.0), shared(0), minShare(0), users(Util::toInt(aUsers)), minSlots(0), maxHubs(0), maxUsers(0) { }

	HubEntry(const string& aName, const string& aServer, const string& aDescription, const string& aUsers, const string& aCountry,
		const string& aShared, const string& aMinShare, const string& aMinSlots, const string& aMaxHubs, const string& aMaxUsers,
		const string& aReliability, const string& aRating) :
		name(aName),
		server(aServer),
		description(aDescription),
		country(aCountry),
		rating(aRating),
		reliability((float)(Util::toFloat(aReliability) / 100.0)),
		shared(Util::toInt64(aShared)),
		minShare(Util::toInt64(aMinShare)),
		users(Util::toInt(aUsers)),
		minSlots(Util::toInt(aMinSlots)),
		maxHubs(Util::toInt(aMaxHubs)),
		maxUsers(Util::toInt(aMaxUsers)) { }

	HubEntry() { }

	HubEntry(const HubEntry& rhs) :
		name(rhs.name),
		server(rhs.server),
		description(rhs.description),
		country(rhs.country),
		rating(rhs.rating),
		reliability(rhs.reliability),
		shared(rhs.shared),
		minShare(rhs.minShare),
		users(rhs.users),
		minSlots(rhs.minSlots),
		maxHubs(rhs.maxHubs),
		maxUsers(rhs.maxUsers) { }

	~HubEntry() { }

	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, country, Country);
	GETSET(string, rating, Rating);
	GETSET(float, reliability, Reliability);
	GETSET(int64_t, shared, Shared);
	GETSET(int64_t, minShare, MinShare);
	GETSET(int, users, Users);
	GETSET(int, minSlots, MinSlots);
	GETSET(int, maxHubs, MaxHubs);
	GETSET(int, maxUsers, MaxUsers);
};

class FavoriteHubEntry {
public:
	typedef FavoriteHubEntry* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;

	FavoriteHubEntry() :
		encoding(Text::systemCharset),
		ip(Util::emptyString),
		searchInterval(SETTING(MINIMUM_SEARCH_INTERVAL)),
		mode(0),
		chatusersplit(0),
		connect(false),
		stealth(false),
		userliststate(true),
		hideShare(false),
		showJoins(false),
		exclChecks(false),
		noAdlSearch(false),
		logChat(false),
		miniTab(false),
		autoOpenOpChat(false) { }

	FavoriteHubEntry(const HubEntry& rhs) :
		name(rhs.getName()),
		server(rhs.getServer()),
		description(rhs.getDescription()),
		encoding(Text::systemCharset),
		ip(Util::emptyString),
		searchInterval(SETTING(MINIMUM_SEARCH_INTERVAL)),
		mode(0),
		chatusersplit(0),
		connect(false),
		stealth(false),
		userliststate(true),
		hideShare(false),
		showJoins(false),
		exclChecks(false),
		noAdlSearch(false),
		logChat(false),
		miniTab(false),
		autoOpenOpChat(false) { }

	FavoriteHubEntry(const FavoriteHubEntry& rhs) :
		userdescription(rhs.userdescription),
		awaymsg(rhs.awaymsg),
		email(rhs.email),
		name(rhs.getName()),
		server(rhs.getServer()),
		description(rhs.getDescription()),
		password(rhs.getPassword()),
		encoding(rhs.getEncoding()),
		ip(rhs.ip),
		hubChats(rhs.hubChats),
		protectedPrefixes(rhs.protectedPrefixes),
		searchInterval(rhs.searchInterval),
		mode(rhs.mode),
		chatusersplit(rhs.chatusersplit),
		connect(rhs.getConnect()),
		stealth(rhs.stealth),
		userliststate(rhs.userliststate),
		group(rhs.getGroup()),
		hideShare(rhs.hideShare),
		showJoins(rhs.showJoins),
		exclChecks(rhs.exclChecks),
		noAdlSearch(rhs.noAdlSearch),
		logChat(rhs.logChat),
		miniTab(rhs.miniTab),
		autoOpenOpChat(rhs.autoOpenOpChat),
		nick(rhs.nick) { }

	~FavoriteHubEntry() {
		for(Action::Iter i = action.begin(); i != action.end(); ++i) {
			delete i->second;
		}
	}
	
	const string& getNick(bool useDefault = true) const { 
		return (!nick.empty() || !useDefault) ? nick : SETTING(NICK);
	}

	void setNick(const string& aNick) { nick = aNick; }

	GETSET(string, userdescription, UserDescription);
	GETSET(string, awaymsg, AwayMsg);
	GETSET(string, email, Email);
	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, password, Password);
	GETSET(string, headerOrder, HeaderOrder);
	GETSET(string, headerWidths, HeaderWidths);
	GETSET(string, headerVisible, HeaderVisible);
	GETSET(string, encoding, Encoding);
	GETSET(string, ip, IP);
	GETSET(string, hubChats, HubChats);
	GETSET(string, protectedPrefixes, ProtectedPrefixes);
	GETSET(uint32_t, searchInterval, SearchInterval);
	GETSET(int, mode, Mode); // 0 = default, 1 = active, 2 = passive	
	GETSET(int, chatusersplit, ChatUserSplit);
	GETSET(bool, connect, Connect);	
	GETSET(bool, stealth, Stealth);
	GETSET(bool, userliststate, UserListState);
	GETSET(string, group, Group);	
	GETSET(bool, hideShare, HideShare); // Hide Share Mod
	GETSET(bool, showJoins, ShowJoins); // Show joins
	GETSET(bool, exclChecks, ExclChecks); // Excl. from client checking
	GETSET(bool, noAdlSearch, NoAdlSearch); // Don't perform ADLSearches in this hub
	GETSET(bool, logChat, LogChat); // Log chat
	GETSET(bool, miniTab, MiniTab); // Mini Tab
	GETSET(bool, autoOpenOpChat, AutoOpenOpChat); // Auto-open OP Chat 	
	
	struct Action {
		typedef Action* Ptr;
		typedef unordered_map<int, Ptr> List;
		typedef List::const_iterator Iter;

		Action(bool aActive, string aRaw = Util::emptyString) 
			noexcept : active(aActive) {
				if(!aRaw.empty()) {
					StringTokenizer<string> tok(aRaw, ',');
					StringList l = tok.getTokens();
					for(StringIter j = l.begin(); j != l.end(); ++j){
						if(Util::toInt(*j) != 0)
							raw.push_back(Raw(Util::toInt(*j)));
					}
				}
			};

		GETSET(bool, active, Active);
		
		struct Raw {
			Raw(int aRawId) 
				noexcept : rawId(aRawId) { };

			GETSET(int, rawId, RawId);
		};

		typedef vector<Raw> RawList;
		typedef RawList::const_iterator RawIter;

		RawList raw;
	};

	Action::List action;

private:
	string nick;
};

class RecentHubEntry {
public:
	typedef RecentHubEntry* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;

	~RecentHubEntry() { }
	
	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, users, Users);
	GETSET(string, shared, Shared);
};

}

#endif /*HUBENTRY_H_*/
