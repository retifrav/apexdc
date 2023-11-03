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

#ifndef DCPLUSPLUS_DCPP_FAVORITE_MANAGER_H
#define DCPLUSPLUS_DCPP_FAVORITE_MANAGER_H

#include "SettingsManager.h"

#include "HttpManagerListener.h"
#include "UserCommand.h"
#include "FavoriteUser.h"
#include "Singleton.h"
#include "ClientManagerListener.h"
#include "FavoriteManagerListener.h"
#include "StringTokenizer.h"
#include "HubEntry.h"
#include "FavHubGroup.h"
#include "User.h"

namespace dcpp {
	
class PreviewApplication {
public:
	typedef PreviewApplication* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;

	PreviewApplication() noexcept {}
	PreviewApplication(string n, string a, string r, string e) : name(n), application(a), arguments(r), extension(e) {};
	~PreviewApplication() { }

	GETSET(string, name, Name);
	GETSET(string, application, Application);
	GETSET(string, arguments, Arguments);
	GETSET(string, extension, Extension);
};

class SimpleXML;

/**
 * Public hub list, favorites (hub&user). Assumed to be called only by UI thread.
 */
class FavoriteManager : public Speaker<FavoriteManagerListener>, private HttpManagerListener, public Singleton<FavoriteManager>,
	private SettingsManagerListener, private ClientManagerListener
{
public:
	// Public Hubs
	enum HubTypes {
		TYPE_NORMAL,
		TYPE_BZIP2
	};
	StringList getHubLists();
	void setHubList(int aHubList);
	int getSelectedHubList() { return lastServer; }
	void refresh(bool forceDownload = false);
	HubTypes getHubListType() { return listType; }
	HubEntryList getPublicHubs() {
		Lock l(cs);
		return publicListMatrix[publicListServer];
	}
	bool isDownloading() { return (useHttp && running); }
	const string& getCurrentHubList() const { return publicListServer; }
	/// @return ref to the reason string the domain is blacklisted for; or empty string otherwise
	const string& blacklisted(const string& domain) const;
	/// @param domain a domain name with 2 words max, such as "example.com"
	void addBlacklist(const string& domain, const string& reason);

	// Favorite Users
	typedef unordered_map<CID, FavoriteUser> FavoriteMap;
	FavoriteMap getFavoriteUsers() { Lock l(cs); return users; }
	PreviewApplication::List& getPreviewApps() { return previewApplications; }

	void addFavoriteUser(const UserPtr& aUser);
	bool isFavoriteUser(const UserPtr& aUser) const { Lock l(cs); return users.find(aUser->getCID()) != users.end(); }
	void removeFavoriteUser(const UserPtr& aUser);

	bool hasSlot(const UserPtr& aUser) const;
	void setSuperUser(const UserPtr& aUser, bool superUser);
	void setUserDescription(const UserPtr& aUser, const string& description);
	void setAutoGrant(const UserPtr& aUser, bool grant);
	void userUpdated(const OnlineUser& info);
	time_t getLastSeen(const UserPtr& aUser) const;
	std::string getUserURL(const UserPtr& aUser) const;

	// Ignored users
	FavoriteMap getIgnoredUsers() const { Lock l(cs); return ignoredUsers; }

	void addIgnoredUser(const UserPtr& aUser);
	bool isIgnoredUser(const UserPtr& aUser) const { Lock l(cs); return ignoredUsers.find(aUser->getCID()) != ignoredUsers.end(); }
	void removeIgnoredUser(const UserPtr& aUser);

	void setIgnoredUserDescription(const UserPtr& aUser, const string& description);

	// Favorite Hubs
	FavoriteHubEntryList& getFavoriteHubs() { return favoriteHubs; }

	void addFavorite(const FavoriteHubEntry& aEntry);
	void updateFavorite(const FavoriteHubEntry* entry);
	void removeFavorite(const FavoriteHubEntry* entry);
	bool isFavoriteHub(const std::string& aUrl);
	FavoriteHubEntry* getFavoriteHubEntry(const string& aServer) const;

	// Favorite hub groups
	const FavHubGroups& getFavHubGroups() const { return favHubGroups; }
	void setFavHubGroups(const FavHubGroups& favHubGroups_) { favHubGroups = favHubGroups_; }

	FavoriteHubEntryList getFavoriteHubs(const string& group) const;
	bool isPrivate(const string& url) const;

	// Favorite Directories
	struct FavoriteDirectory {
		string dir, ext, name;
	};
	typedef vector<FavoriteDirectory> FavDirList;
	typedef FavDirList::iterator FavDirIter;
	bool addFavoriteDir(const string& aDirectory, const string& aName, const string& aExt);
	bool removeFavoriteDir(const string& aName);
	bool renameFavoriteDir(const string& aName, const string& anotherName);
	bool updateFavoriteDir(const string& aName, const FavoriteDirectory& dir);
	string getDownloadDirectory(const string& ext);
	FavDirList getFavoriteDirs() { return favoriteDirs; }

	// Recent Hubs
	RecentHubEntry::List& getRecentHubs() { return recentHubs; };

	void addRecent(const RecentHubEntry& aEntry);
	void removeRecent(const RecentHubEntry* entry);
	void updateRecent(const RecentHubEntry* entry);

	RecentHubEntry* getRecentHubEntry(const string& aServer) {
		for(RecentHubEntry::Iter i = recentHubs.begin(); i != recentHubs.end(); ++i) {
			RecentHubEntry* r = *i;
			if(stricmp(r->getServer(), aServer) == 0) {
				return r;
			}
		}
		return NULL;
	}

	PreviewApplication* addPreviewApp(string name, string application, string arguments, string extension){
		PreviewApplication* pa = new PreviewApplication(name, application, arguments, extension);
		previewApplications.push_back(pa);
		return pa;
	}

	PreviewApplication* removePreviewApp(unsigned int index){
		if(previewApplications.size() > index)
			previewApplications.erase(previewApplications.begin() + index);	
		return NULL;
	}

	PreviewApplication* getPreviewApp(unsigned int index, PreviewApplication &pa){
		if(previewApplications.size() > index)
			pa = *previewApplications[index];	
		return NULL;
	}
	
	PreviewApplication* updatePreviewApp(int index, PreviewApplication &pa){
		*previewApplications[index] = pa;
		return NULL;
	}

	void removeallRecent() {
		recentHubs.clear();
		recentsave();
	}

	bool getActiveAction(FavoriteHubEntry* entry, int actionId);
	void setActiveAction(FavoriteHubEntry* entry, int actionId, bool active);
	bool getActiveRaw(FavoriteHubEntry* entry, int actionId, int rawId);
	void setActiveRaw(FavoriteHubEntry* entry, int actionId, int rawId, bool active);

	string getAwayMessage(const string& aServer, StringMap& params);

	// User Commands
	UserCommand addUserCommand(int type, int ctx, Flags::MaskType flags, const string& name, const string& command, const string& to, const string& hub);
	bool getUserCommand(int cid, UserCommand& uc);
	int findUserCommand(const string& aName, const string& aUrl);
	bool moveUserCommand(int cid, int pos);
	void updateUserCommand(const UserCommand& uc);
	void removeUserCommand(int cid);
	void removeUserCommand(const string& srv);
	void removeHubUserCommands(int ctx, const string& hub);

	UserCommand::List getUserCommands() { Lock l(cs); return userCommands; }
	UserCommand::List getUserCommands(int ctx, const StringList& hub, bool& op);

	void load();
	void save();
	void recentsave();

	void shutdown();
	
private:
	FavoriteHubEntryList favoriteHubs;
	FavDirList favoriteDirs;
	FavHubGroups favHubGroups;
	RecentHubEntry::List recentHubs;
	PreviewApplication::List previewApplications;
	UserCommand::List userCommands;
	int64_t version;
	int lastId;

	FavoriteMap users, ignoredUsers;

	mutable CriticalSection cs;

	// Public Hubs
	typedef unordered_map<string, HubEntryList> PubListMap;
	PubListMap publicListMatrix;
	string publicListServer;
	bool useHttp, running;
	HttpConnection* c;
	int lastServer;
	HubTypes listType;
	StringMap blacklist;
	
	/** Used during loading to prevent saving. */
	bool dontSave;

	friend class Singleton<FavoriteManager>;
	
	FavoriteManager();
	~FavoriteManager();
	
	FavoriteHubEntryList::const_iterator getFavoriteHub(const string& aServer) const;
	RecentHubEntry::Iter getRecentHub(const string& aServer) const;

	// ClientManagerListener
	void on(UserUpdated, const OnlineUser& user) noexcept;
	void on(UserConnected, const UserPtr& user) noexcept;
	void on(UserDisconnected, const UserPtr& user) noexcept;

	// HttpManagerListener
	void on(HttpManagerListener::Added, HttpConnection* c) noexcept;
	void on(HttpManagerListener::Failed, HttpConnection* c, const string& str) noexcept;
	void on(HttpManagerListener::Complete, HttpConnection* c, OutputStream* stream) noexcept;

	bool onHttpFinished(const string& buf) noexcept;

	// SettingsManagerListener
	void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept {
		load(xml);
		recentload(xml);
		previewload(xml);
	}

	void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept {
		previewsave(xml);
	}

	void load(SimpleXML& aXml);
	void recentload(SimpleXML& aXml);
	void previewload(SimpleXML& aXml);
	void previewsave(SimpleXML& aXml);
	
	string getConfigFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "Favorites.xml"; }
};

} // namespace dcpp

#endif // !defined(FAVORITE_MANAGER_H)

/**
 * @file
 * $Id$
 */
