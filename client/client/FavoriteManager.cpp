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
#include "FavoriteManager.h"

#include "ClientManager.h"
#include "ResourceManager.h"
#include "CryptoManager.h"
#include "RawManager.h"

#include "HttpManager.h"
#include "HttpConnection.h"
#include "StringTokenizer.h"
#include "SimpleXML.h"
#include "UserCommand.h"
#include "BZUtils.h"
#include "FilteredFile.h"
#include "version.h"

namespace dcpp {

using std::make_pair;
using std::swap;

FavoriteManager::FavoriteManager() : version(0), lastId(0), useHttp(false), running(false), c(NULL), lastServer(0), listType(TYPE_NORMAL), dontSave(false) {
	SettingsManager::getInstance()->addListener(this);
	HttpManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);

	File::ensureDirectory(Util::getHubListsPath());

	addBlacklist("adchublist.com", "Domain used for spam purposes.");
	addBlacklist("hublist.org", "Domain used for spam purposes.");
	addBlacklist("hubtracker.com", "Domain lost to unknown owners advertising dubious pharmaceuticals.");
	addBlacklist("openhublist.org", "Domain used for spam purposes.");
	addBlacklist("dchublist.com", "Redirection to the new domain fails. To access this hublist add its new address <https://www.te-home.net/?do=hublist&get=hublist.xml.bz2> instead.");
	addBlacklist("hublista.hu", "Server discontinued, domain may lost to unknown owners.");

	addBlacklist("adcfun.com", "Domain lost to unknown owners.");
	addBlacklist("techgeeks.geekgalaxy.com", "Domain lost to unknown owners.");
}

FavoriteManager::~FavoriteManager() {
	for_each(favoriteHubs.begin(), favoriteHubs.end(), DeleteFunction());
	for_each(recentHubs.begin(), recentHubs.end(), DeleteFunction());
	for_each(previewApplications.begin(), previewApplications.end(), DeleteFunction());
}

void FavoriteManager::shutdown() {
	ClientManager::getInstance()->removeListener(this);
	HttpManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
}

UserCommand FavoriteManager::addUserCommand(int type, int ctx, Flags::MaskType flags, const string& name, const string& command, const string& to, const string& hub) {
	// No dupes, add it...
	Lock l(cs);
	userCommands.push_back(UserCommand(lastId++, type, ctx, flags, name, command, to, hub));
	UserCommand& uc = userCommands.back();
	if(!uc.isSet(UserCommand::FLAG_NOSAVE)) 
		save();
	return userCommands.back();
}

bool FavoriteManager::getUserCommand(int cid, UserCommand& uc) {
	Lock l(cs);
	for(UserCommand::List::const_iterator i = userCommands.begin(); i != userCommands.end(); ++i) {
		if(i->getId() == cid) {
			uc = *i;
			return true;
		}
	}
	return false;
}

bool FavoriteManager::moveUserCommand(int cid, int pos) {
	dcassert(pos == -1 || pos == 1);
	Lock l(cs);
	for(UserCommand::List::iterator i = userCommands.begin(); i != userCommands.end(); ++i) {
		if(i->getId() == cid) {
			swap(*i, *(i + pos));
			return true;
		}
	}
	return false;
}

void FavoriteManager::updateUserCommand(const UserCommand& uc) {
	bool nosave = true;
	Lock l(cs);
	for(UserCommand::List::iterator i = userCommands.begin(); i != userCommands.end(); ++i) {
		if(i->getId() == uc.getId()) {
			*i = uc;
			nosave = uc.isSet(UserCommand::FLAG_NOSAVE);
			break;
		}
	}
	if(!nosave)
		save();
}

int FavoriteManager::findUserCommand(const string& aName, const string& aUrl) {
	Lock l(cs);
	for(UserCommand::List::iterator i = userCommands.begin(); i != userCommands.end(); ++i) {
		if(i->getName() == aName && i->getHub() == aUrl) {
			return i->getId();
		}
	}
	return -1;
}

void FavoriteManager::removeUserCommand(int cid) {
	bool nosave = true;
	Lock l(cs);
	for(UserCommand::List::iterator i = userCommands.begin(); i != userCommands.end(); ++i) {
		if(i->getId() == cid) {
			nosave = i->isSet(UserCommand::FLAG_NOSAVE);
			userCommands.erase(i);
			break;
		}
	}
	if(!nosave)
		save();
}
void FavoriteManager::removeUserCommand(const string& srv) {
	Lock l(cs);
	for(UserCommand::List::iterator i = userCommands.begin(); i != userCommands.end(); ) {
		if((i->getHub() == srv) && i->isSet(UserCommand::FLAG_NOSAVE)) {
			i = userCommands.erase(i);
		} else {
			++i;
		}
	}
}

void FavoriteManager::removeHubUserCommands(int ctx, const string& hub) {
	Lock l(cs);
	for(UserCommand::List::iterator i = userCommands.begin(); i != userCommands.end(); ) {
		if(i->getHub() == hub && i->isSet(UserCommand::FLAG_NOSAVE) && i->getCtx() & ctx) {
			i = userCommands.erase(i);
		} else {
			++i;
		}
	}
}

void FavoriteManager::addFavoriteUser(const UserPtr& aUser) { 
	Lock l(cs);
	if(users.find(aUser->getCID()) == users.end()) {
		StringList urls = ClientManager::getInstance()->getHubs(aUser->getCID(), Util::emptyString);
		StringList nicks = ClientManager::getInstance()->getNicks(aUser->getCID(), Util::emptyString);
        
		/// @todo make this an error probably...
		if(urls.empty())
			urls.push_back(Util::emptyString);
		if(nicks.empty())
			nicks.push_back(Util::emptyString);

		// fixme: default description can be the user's current known description from the hub
		FavoriteMap::const_iterator i = users.insert(make_pair(aUser->getCID(), FavoriteUser(aUser, nicks[0], urls[0]))).first;
		fire(FavoriteManagerListener::UserAdded(), i->second);
		save();
	}
}

void FavoriteManager::removeFavoriteUser(const UserPtr& aUser) {
	Lock l(cs);
	FavoriteMap::iterator i = users.find(aUser->getCID());
	if(i != users.end()) {
		fire(FavoriteManagerListener::UserRemoved(), i->second);
		users.erase(i);
		save();
	}
}

void FavoriteManager::addIgnoredUser(const UserPtr& aUser) { 
	Lock l(cs);
	if(ignoredUsers.find(aUser->getCID()) == ignoredUsers.end()) {
		StringList urls = ClientManager::getInstance()->getHubs(aUser->getCID(), Util::emptyString);
		StringList nicks = ClientManager::getInstance()->getNicks(aUser->getCID(), Util::emptyString);
        
		/// @todo make this an error probably...
		if(urls.empty())
			urls.push_back(Util::emptyString);
		if(nicks.empty())
			nicks.push_back(Util::emptyString);

		FavoriteMap::iterator i = ignoredUsers.insert(make_pair(aUser->getCID(), FavoriteUser(aUser, nicks[0], urls[0]))).first;
		i->second.setFlag(FavoriteUser::FLAG_IGNORED);
		fire(FavoriteManagerListener::UserAdded(), i->second);
		save();
	}
}

void FavoriteManager::removeIgnoredUser(const UserPtr& aUser) {
	Lock l(cs);
	FavoriteMap::iterator i = ignoredUsers.find(aUser->getCID());
	if(i != ignoredUsers.end()) {
		fire(FavoriteManagerListener::UserRemoved(), i->second);
		ignoredUsers.erase(i);
		save();
	}
}

std::string FavoriteManager::getUserURL(const UserPtr& aUser) const {
	Lock l(cs);
	FavoriteMap::const_iterator i = users.find(aUser->getCID());
	if(i != users.end()) {
		const FavoriteUser& fu = i->second;
		return fu.getUrl();
	}
	return Util::emptyString;
}

void FavoriteManager::addFavorite(const FavoriteHubEntry& aEntry) {
	FavoriteHubEntryList::const_iterator i = getFavoriteHub(aEntry.getServer());
	if(i != favoriteHubs.end()) {
		return;
	}
	auto f = new FavoriteHubEntry(aEntry);
	favoriteHubs.push_back(f);
	fire(FavoriteManagerListener::FavoriteAdded(), f);
	save();
}

void FavoriteManager::updateFavorite(const FavoriteHubEntry* entry) {
	FavoriteHubEntryList::const_iterator i = getFavoriteHub(entry->getServer());
	if(i == favoriteHubs.end())
		return;
		
	fire(FavoriteManagerListener::FavoriteUpdated(), entry);
	save();
}

void FavoriteManager::removeFavorite(const FavoriteHubEntry* entry) {
	FavoriteHubEntryList::iterator i = find(favoriteHubs.begin(), favoriteHubs.end(), entry);
	if(i == favoriteHubs.end()) {
		return;
	}

	fire(FavoriteManagerListener::FavoriteRemoved(), entry);
	favoriteHubs.erase(i);
	delete entry;
	save();
}

bool FavoriteManager::isFavoriteHub(const std::string& url) {
	FavoriteHubEntryList::const_iterator i = getFavoriteHub(url);
	if(i != favoriteHubs.end()) {
		return true;
	}
	return false;
}

bool FavoriteManager::addFavoriteDir(const string& aDirectory, const string& aName, const string& aExt){
	string path = aDirectory;

	if( path[ path.length() -1 ] != PATH_SEPARATOR )
		path += PATH_SEPARATOR;

	for(FavDirIter i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i) {
		if((strnicmp(path, i->dir, i->dir.length()) == 0) && (strnicmp(path, i->dir, path.length()) == 0)) {
			return false;
		}
		if(stricmp(aName, i->name) == 0) {
			return false;
		}
		if(!aExt.empty() && stricmp(aExt, i->ext) == 0) {
			return false;
		}
	}
	FavoriteDirectory favDir = { aDirectory, aExt, aName };
	favoriteDirs.push_back(favDir);
	save();
	return true;
}

bool FavoriteManager::removeFavoriteDir(const string& aName) {
	string d(aName);

	if(d[d.length() - 1] != PATH_SEPARATOR)
		d += PATH_SEPARATOR;

	for(FavDirIter j = favoriteDirs.begin(); j != favoriteDirs.end(); ++j) {
		if(stricmp(j->dir.c_str(), d.c_str()) == 0) {
			favoriteDirs.erase(j);
			save();
			return true;
		}
	}
	return false;
}

bool FavoriteManager::renameFavoriteDir(const string& aName, const string& anotherName) {

	for(FavDirIter j = favoriteDirs.begin(); j != favoriteDirs.end(); ++j) {
		if(stricmp(j->name.c_str(), aName.c_str()) == 0) {
			j->name = anotherName;
			save();
			return true;
		}
	}
	return false;
}

bool FavoriteManager::updateFavoriteDir(const string& aName, const FavoriteDirectory& dir) {

	for(FavDirIter j = favoriteDirs.begin(); j != favoriteDirs.end(); ++j) {
		if(stricmp(j->name.c_str(), aName.c_str()) == 0) {
			j->dir = dir.dir;
			j->ext = dir.ext;
			j->name = dir.name;
			save();
			return true;
		}
	}
	return false;
}

string FavoriteManager::getDownloadDirectory(const string& ext) {
	if(ext.size() > 1) {
		for(FavDirIter i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i) {
			StringList tok = StringTokenizer<string>(i->ext, ';').getTokens();
			for(StringIter j = tok.begin(); j != tok.end(); ++j) {
				if(stricmp(ext.substr(1).c_str(), (*j).c_str()) == 0) 
					return i->dir;			
			}
		}
	}
	return SETTING(DOWNLOAD_DIRECTORY);
}

void FavoriteManager::addRecent(const RecentHubEntry& aEntry) {
	RecentHubEntry::Iter i = getRecentHub(aEntry.getServer());
	if(i != recentHubs.end()) {
		return;
	}
	RecentHubEntry* f = new RecentHubEntry(aEntry);
	recentHubs.push_back(f);
	fire(FavoriteManagerListener::RecentAdded(), f);
	recentsave();
}

void FavoriteManager::removeRecent(const RecentHubEntry* entry) {
	RecentHubEntry::List::iterator i = find(recentHubs.begin(), recentHubs.end(), entry);
	if(i == recentHubs.end()) {
		return;
	}
		
	fire(FavoriteManagerListener::RecentRemoved(), entry);
	recentHubs.erase(i);
	delete entry;
	recentsave();
}

void FavoriteManager::updateRecent(const RecentHubEntry* entry) {
	RecentHubEntry::Iter i = find(recentHubs.begin(), recentHubs.end(), entry);
	if(i == recentHubs.end()) {
		return;
	}
		
	fire(FavoriteManagerListener::RecentUpdated(), entry);
	recentsave();
}

class XmlListLoader : public SimpleXMLReader::CallBack {
public:
	XmlListLoader(HubEntryList& lst) : publicHubs(lst) { }
	~XmlListLoader() { }
	void startTag(const string& name, StringPairList& attribs, bool) {
		if(name == "Hub") {
			const string& name = getAttrib(attribs, "Name", 0);
			const string& server = getAttrib(attribs, "Address", 1);
			const string& description = getAttrib(attribs, "Description", 2);
			const string& users = getAttrib(attribs, "Users", 3);
			const string& country = getAttrib(attribs, "Country", 4);
			const string& shared = getAttrib(attribs, "Shared", 5);
			const string& minShare = getAttrib(attribs, "Minshare", 5);
			const string& minSlots = getAttrib(attribs, "Minslots", 5);
			const string& maxHubs = getAttrib(attribs, "Maxhubs", 5);
			const string& maxUsers = getAttrib(attribs, "Maxusers", 5);
			const string& reliability = getAttrib(attribs, "Reliability", 5);
			const string& rating = getAttrib(attribs, "Rating", 5);
			publicHubs.push_back(HubEntry(name, server, description, users, country, shared, minShare, minSlots, maxHubs, maxUsers, reliability, rating));
		}
	}

private:
	HubEntryList& publicHubs;
};

bool FavoriteManager::onHttpFinished(const string& buf) noexcept {
	MemoryInputStream mis(buf);
	bool success = true;

	Lock l(cs);
	HubEntryList& list = publicListMatrix[publicListServer];
	list.clear();

	try {
		XmlListLoader loader(list);

		if((listType == TYPE_BZIP2) && (!buf.empty())) {
			FilteredInputStream<UnBZFilter, false> f(&mis);
			SimpleXMLReader(&loader).parse(f);
		} else {
			SimpleXMLReader(&loader).parse(mis);
		}
	} catch(const Exception&) {
		success = false;
		fire(FavoriteManagerListener::Corrupted(), useHttp ? publicListServer : Util::emptyString);
	}

	if(useHttp) {
		try {
			File f(Util::getHubListsPath() + Util::validateFileName(publicListServer), File::WRITE, File::CREATE | File::TRUNCATE);
			f.write(buf);
			f.close();
		} catch(const FileException&) { }
	}
	
	return success;
}

void FavoriteManager::save() {
	if(dontSave)
		return;

	Lock l(cs);
	try {
		SimpleXML xml;

		xml.addTag("Favorites");
		xml.addChildAttrib("Version", COMPATIBLE_VERSIONID);
		xml.stepIn();

		xml.addTag("Hubs");
		xml.stepIn();

		for(FavHubGroups::const_iterator i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i) {
			xml.addTag("Group");
			xml.addChildAttrib("Name", i->first);
			xml.addChildAttrib("Private", i->second.priv);
		}

		for(FavoriteHubEntryList::const_iterator i = favoriteHubs.begin(), iend = favoriteHubs.end(); i != iend; ++i) {
			xml.addTag("Hub");
			xml.addChildAttrib("Name", (*i)->getName());
			xml.addChildAttrib("Connect", (*i)->getConnect());
			xml.addChildAttrib("Description", (*i)->getDescription());
			xml.addChildAttrib("Nick", (*i)->getNick(false));
			xml.addChildAttrib("Password", (*i)->getPassword());
			xml.addChildAttrib("Server", (*i)->getServer());
			xml.addChildAttrib("UserDescription", (*i)->getUserDescription());
			xml.addChildAttrib("AwayMsg", (*i)->getAwayMsg());
			xml.addChildAttrib("Email", (*i)->getEmail());
			xml.addChildAttrib("Encoding", (*i)->getEncoding());
			xml.addChildAttrib("ChatUserSplit", (*i)->getChatUserSplit());
			xml.addChildAttrib("StealthMode", (*i)->getStealth());
			xml.addChildAttrib("HideShare", (*i)->getHideShare()); // Hide Share Mod
			xml.addChildAttrib("ShowJoins", (*i)->getShowJoins()); // Show joins
			xml.addChildAttrib("ExclChecks", (*i)->getExclChecks()); // Excl. from client checking
			xml.addChildAttrib("NoAdlSearch", (*i)->getNoAdlSearch()); // Don't perform ADLSearches in this hub
			xml.addChildAttrib("LogChat", (*i)->getLogChat()); // Log chat
			xml.addChildAttrib("MiniTab", (*i)->getMiniTab()); // Mini Tab
			xml.addChildAttrib("AutoOpenOpChat", (*i)->getAutoOpenOpChat()); // Auto-open OP Chat
			xml.addChildAttrib("UserListState", (*i)->getUserListState());
			xml.addChildAttrib("HubFrameOrder",	(*i)->getHeaderOrder());
			xml.addChildAttrib("HubFrameWidths", (*i)->getHeaderWidths());
			xml.addChildAttrib("HubFrameVisible", (*i)->getHeaderVisible());
			xml.addChildAttrib("Mode", Util::toString((*i)->getMode()));
			xml.addChildAttrib("IP", (*i)->getIP());
			xml.addChildAttrib("SearchInterval", Util::toString((*i)->getSearchInterval()));
			xml.addChildAttrib("Group", (*i)->getGroup());
			xml.addChildAttrib("HubChats", (*i)->getHubChats());
			xml.addChildAttrib("ProtectedPrefixes", (*i)->getProtectedPrefixes());

			xml.stepIn();
			for(FavoriteHubEntry::Action::Iter a = (*i)->action.begin(); a != (*i)->action.end(); ++a) {
				if(RawManager::getInstance()->getValidAction(a->first)) {
					string raw = Util::emptyString;
					for(FavoriteHubEntry::Action::RawIter j = a->second->raw.begin(); j != a->second->raw.end(); ++j) {
						if(!raw.empty())
							raw += ",";
						raw += Util::toString(j->getRawId());
					}
					if(!raw.empty() || a->second->getActive()) {
						xml.addTag("Action");
						xml.addChildAttrib("ID", a->first);
						xml.addChildAttrib("Active", Util::toString(a->second->getActive()));
						xml.addChildAttrib("Raw", raw);
					}
				}
			}
			xml.stepOut();
		}

		xml.stepOut();

		xml.addTag("Users");
		xml.stepIn();
		for(FavoriteMap::const_iterator i = users.begin(), iend = users.end(); i != iend; ++i) {
			xml.addTag("User");
			xml.addChildAttrib("LastSeen", i->second.getLastSeen());
			xml.addChildAttrib("GrantSlot", i->second.isSet(FavoriteUser::FLAG_GRANTSLOT));
			xml.addChildAttrib("SuperUser", i->second.isSet(FavoriteUser::FLAG_SUPERUSER));
			xml.addChildAttrib("UserDescription", i->second.getDescription());
			xml.addChildAttrib("Nick", i->second.getNick());
			xml.addChildAttrib("URL", i->second.getUrl());
			xml.addChildAttrib("CID", i->first.toBase32());
		}
		xml.stepOut();

		xml.addTag("IgnoredUsers");
		xml.stepIn();
		for(FavoriteMap::const_iterator i = ignoredUsers.begin(), iend = ignoredUsers.end(); i != iend; ++i) {
			xml.addTag("User");
			xml.addChildAttrib("LastSeen", i->second.getLastSeen());
			xml.addChildAttrib("UserDescription", i->second.getDescription());
			xml.addChildAttrib("Nick", i->second.getNick());
			xml.addChildAttrib("URL", i->second.getUrl());
			xml.addChildAttrib("CID", i->first.toBase32());
		}
		xml.stepOut();

		xml.addTag("UserCommands");
		xml.stepIn();
		for(UserCommand::List::const_iterator i = userCommands.begin(), iend = userCommands.end(); i != iend; ++i) {
			if(!i->isSet(UserCommand::FLAG_NOSAVE)) {
				xml.addTag("UserCommand");
				xml.addChildAttrib("Type", i->getType());
				xml.addChildAttrib("Context", i->getCtx());
				xml.addChildAttrib("Name", i->getName());
				xml.addChildAttrib("Command", i->getCommand());
				xml.addChildAttrib("To", i->getTo());
				xml.addChildAttrib("Hub", i->getHub());
			}
		}
		xml.stepOut();

		//Favorite download to dirs
		xml.addTag("FavoriteDirs");
		xml.stepIn();
		FavDirList spl = getFavoriteDirs();
		for(FavDirIter i = spl.begin(), iend = spl.end(); i != iend; ++i) {
			xml.addTag("Directory", i->dir);
			xml.addChildAttrib("Name", i->name);
			xml.addChildAttrib("Extensions", i->ext);
		}
		xml.stepOut();

		xml.stepOut();

		string fname = getConfigFile();

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);

	} catch(const Exception& e) {
		dcdebug("FavoriteManager::save: %s\n", e.getError().c_str());
	}
}

void FavoriteManager::recentsave() {
	try {
		SimpleXML xml;

		xml.addTag("Recents");
		xml.stepIn();

		xml.addTag("Hubs");
		xml.stepIn();

		for(RecentHubEntry::Iter i = recentHubs.begin(); i != recentHubs.end(); ++i) {
			xml.addTag("Hub");
			xml.addChildAttrib("Name", (*i)->getName());
			xml.addChildAttrib("Description", (*i)->getDescription());
			xml.addChildAttrib("Users", (*i)->getUsers());
			xml.addChildAttrib("Shared", (*i)->getShared());
			xml.addChildAttrib("Server", (*i)->getServer());
		}

		xml.stepOut();
		xml.stepOut();
		
		string fname = Util::getPath(Util::PATH_USER_CONFIG) + "Recents.xml";

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("FavoriteManager::recentsave: %s\n", e.getError().c_str());
	}
}

void FavoriteManager::load() {

	// Add NMDC standard op commands
	static const char kickstr[] = 
		"$To: %[userNI] From: %[myNI] $<%[myNI]> You are being kicked because: %[kickline:Reason]|<%[myNI]> is kicking %[userNI] because: %[kickline:Reason]|$Kick %[userNI]|";
	addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_CHAT | UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE, 
		STRING(KICK_USER), kickstr, "", "op");
	static const char kickfilestr[] = 
		"$To: %[userNI] From: %[myNI] $<%[myNI]> You are being kicked because: %[kickline:Reason] %[fileFN]|<%[myNI]> is kicking %[userNI] because: %[kickline:Reason] %[fileFN]|$Kick %[userNI]|";
	addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE, 
		STRING(KICK_USER_FILE), kickfilestr, "", "op");
	static const char redirstr[] =
		"$OpForceMove $Who:%[userNI]$Where:%[line:Target Server]$Msg:%[line:Message]|";
	addUserCommand(UserCommand::TYPE_RAW_ONCE, UserCommand::CONTEXT_CHAT | UserCommand::CONTEXT_SEARCH, UserCommand::FLAG_NOSAVE, 
		STRING(REDIRECT_USER), redirstr, "", "op");

	try {
		Util::migrate(getConfigFile());

		SimpleXML xml;
		xml.fromXML(File(getConfigFile(), File::READ, File::OPEN).read());
		
		if(xml.findChild("Favorites")) {
			version = Util::toInt64(xml.getChildAttrib("Version", "0"));

			xml.stepIn();
			load(xml);
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("FavoriteManager::load: %s\n", e.getError().c_str());
	}

	/*// If this is fresh install insert our preinstalled hubs
	if(favoriteHubs.empty()) {
		auto e = new FavoriteHubEntry();
		e->setName("Tech-Geeks Online - Official ApexDC++ Support Hub");
		e->setConnect(true);
		e->setDescription("Tech-Geeks Online - Official ApexDC++ Support Hub");
		e->setNick(Util::emptyString);
		e->setPassword(Util::emptyString);
		e->setUserDescription(Util::emptyString);
		e->setServer("adc://techgeeks.geekgalaxy.com:1411");
		e->setEncoding(Text::utf8);
		favoriteHubs.push_back(e);

		e = new FavoriteHubEntry();
		e->setName("The Ascendent Network - Primary Network Array");
		e->setConnect(true);
		e->setDescription("The Ascendent Network - Primary Network Array");
		e->setNick(Util::emptyString);
		e->setPassword(Util::emptyString);
		e->setUserDescription(Util::emptyString);
		e->setServer("ascendentnetwork.net-freaks.com:6666");
		favoriteHubs.push_back(e);
	}

	// If needed, update old installations to ADC
	auto preset = getFavoriteHubEntry("techgeeks.geekgalaxy.com:1411");
	if (version < 1984 && preset) {
		preset->setServer("adc://techgeeks.geekgalaxy.com:1411");
		preset->setEncoding(Text::utf8);
	}*/

	try {
		Util::migrate(Util::getPath(Util::PATH_USER_CONFIG) + "Recents.xml");
		
		SimpleXML xml;
		xml.fromXML(File(Util::getPath(Util::PATH_USER_CONFIG) + "Recents.xml", File::READ, File::OPEN).read());
		
		if(xml.findChild("Recents")) {
			xml.stepIn();
			recentload(xml);
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("FavoriteManager::recentload: %s\n", e.getError().c_str());
	}
}

void FavoriteManager::load(SimpleXML& aXml) {
	dontSave = true;

	aXml.resetCurrentChild();
	if(aXml.findChild("Hubs")) {
		aXml.stepIn();

		while(aXml.findChild("Group")) {
			string name = aXml.getChildAttrib("Name");
			if(name.empty())
				continue;
			FavHubGroupProperties props = { aXml.getBoolChildAttrib("Private") };
			favHubGroups[name] = props;
		}

		aXml.resetCurrentChild();
		while(aXml.findChild("Hub")) {
			auto e = new FavoriteHubEntry();
			e->setName(aXml.getChildAttrib("Name"));
			e->setConnect(aXml.getBoolChildAttrib("Connect"));
			e->setDescription(aXml.getChildAttrib("Description"));
			e->setNick(aXml.getChildAttrib("Nick"));
			e->setPassword(aXml.getChildAttrib("Password"));
			e->setServer(aXml.getChildAttrib("Server"));
			e->setUserDescription(aXml.getChildAttrib("UserDescription"));
			e->setAwayMsg(aXml.getChildAttrib("AwayMsg"));
			e->setEmail(aXml.getChildAttrib("Email"));
			e->setEncoding(aXml.getChildAttrib("Encoding"));
			e->setChatUserSplit(aXml.getIntChildAttrib("ChatUserSplit"));
			e->setStealth(aXml.getBoolChildAttrib("StealthMode"));
			e->setHideShare(aXml.getBoolChildAttrib("HideShare")); // Hide Share Mod
			e->setShowJoins(aXml.getBoolChildAttrib("ShowJoins")); // Show joins
			e->setExclChecks(aXml.getBoolChildAttrib("ExclChecks")); // Excl. from client checking
			e->setNoAdlSearch(aXml.getBoolChildAttrib("NoAdlSearch")); // Don't perform ADLSearches in this hub
			e->setLogChat(aXml.getBoolChildAttrib("LogChat")); // Log chat
			e->setMiniTab(aXml.getBoolChildAttrib("MiniTab")); // Mini Tab
			e->setAutoOpenOpChat(aXml.getBoolChildAttrib("AutoOpenOpChat")); // Auto-open OP Chat
			e->setUserListState(aXml.getBoolChildAttrib("UserListState"));
			e->setHeaderOrder(aXml.getChildAttrib("HubFrameOrder", SETTING(HUBFRAME_ORDER)));
			e->setHeaderWidths(aXml.getChildAttrib("HubFrameWidths", SETTING(HUBFRAME_WIDTHS)));
			e->setHeaderVisible(aXml.getChildAttrib("HubFrameVisible", SETTING(HUBFRAME_VISIBLE)));
			e->setMode(Util::toInt(aXml.getChildAttrib("Mode")));
			e->setIP(aXml.getChildAttrib("IP"));
			e->setSearchInterval(Util::toUInt32(aXml.getChildAttrib("SearchInterval")));
			e->setGroup(aXml.getChildAttrib("Group"));
			e->setHubChats(aXml.getChildAttrib("HubChats"));
			e->setProtectedPrefixes(aXml.getChildAttrib("ProtectedPrefixes"));

			aXml.stepIn();
			while(aXml.findChild("Action")) {
				int actionId(aXml.getIntChildAttrib("ID"));
				bool active(RawManager::getInstance()->getVersion().empty() ? aXml.getBoolChildAttrib("Actif") : aXml.getBoolChildAttrib("Active"));
				string raw(aXml.getChildAttrib("Raw"));
				if(RawManager::getInstance()->getValidAction(actionId))
					e->action.insert(make_pair(actionId, new FavoriteHubEntry::Action(active, raw)));
			}
			aXml.stepOut();
			favoriteHubs.push_back(e);
		}

		aXml.stepOut();
	}
	aXml.resetCurrentChild();
	if(aXml.findChild("Users")) {
		aXml.stepIn();
		while(aXml.findChild("User")) {
			UserPtr u;
			const string& cid = aXml.getChildAttrib("CID");
			const string& nick = aXml.getChildAttrib("Nick");
			const string& hubUrl = aXml.getChildAttrib("URL");

			if(cid.length() != 39) {
				if(nick.empty() || hubUrl.empty())
					continue;
				u = ClientManager::getInstance()->getUser(nick, hubUrl);
			} else {
				u = ClientManager::getInstance()->getUser(CID(cid));
			}
			FavoriteMap::iterator i = users.insert(make_pair(u->getCID(), FavoriteUser(u, nick, hubUrl))).first;

			if(aXml.getBoolChildAttrib("GrantSlot"))
				i->second.setFlag(FavoriteUser::FLAG_GRANTSLOT);
			if(aXml.getBoolChildAttrib("SuperUser"))
				i->second.setFlag(FavoriteUser::FLAG_SUPERUSER);

			i->second.setLastSeen((uint32_t)aXml.getIntChildAttrib("LastSeen"));
			i->second.setDescription(aXml.getChildAttrib("UserDescription"));

		}
		aXml.stepOut();
	}

	aXml.resetCurrentChild();
	if(aXml.findChild("IgnoredUsers")) {
		aXml.stepIn();
		while(aXml.findChild("User")) {
			UserPtr u;
			const string& cid = aXml.getChildAttrib("CID");
			const string& nick = aXml.getChildAttrib("Nick");
			const string& hubUrl = aXml.getChildAttrib("URL");

			if(cid.length() != 39) {
				if(nick.empty() || hubUrl.empty())
					continue;
				u = ClientManager::getInstance()->getUser(nick, hubUrl);
			} else {
				u = ClientManager::getInstance()->getUser(CID(cid));
			}
			FavoriteMap::iterator i = ignoredUsers.insert(make_pair(u->getCID(), FavoriteUser(u, nick, hubUrl))).first;

			i->second.setFlag(FavoriteUser::FLAG_IGNORED);
			i->second.setLastSeen((uint32_t)aXml.getIntChildAttrib("LastSeen"));
			i->second.setDescription(aXml.getChildAttrib("UserDescription"));
		}
		aXml.stepOut();
	}

	aXml.resetCurrentChild();
	if(aXml.findChild("UserCommands")) {
		aXml.stepIn();
		while(aXml.findChild("UserCommand")) {
			addUserCommand(aXml.getIntChildAttrib("Type"), aXml.getIntChildAttrib("Context"), 0, aXml.getChildAttrib("Name"),
				aXml.getChildAttrib("Command"), aXml.getChildAttrib("To"), aXml.getChildAttrib("Hub"));
		}
		aXml.stepOut();
	}

	//Favorite download to dirs
	aXml.resetCurrentChild();
	if(aXml.findChild("FavoriteDirs")) {
		aXml.stepIn();
		while(aXml.findChild("Directory")) {
			string virt = aXml.getChildAttrib("Name");
			string ext = aXml.getChildAttrib("Extensions");
			string d(aXml.getChildData());
			FavoriteManager::getInstance()->addFavoriteDir(d, virt, ext);
		}
		aXml.stepOut();
	}

	dontSave = false;
}

void FavoriteManager::userUpdated(const OnlineUser& info) {
	Lock l(cs);
	FavoriteMap::iterator i = users.find(info.getUser()->getCID());
	if(i != users.end()) {
		FavoriteUser& fu = i->second;
		fu.update(info);
		save();
	}
}

FavoriteHubEntry* FavoriteManager::getFavoriteHubEntry(const string& aServer) const {
	for(FavoriteHubEntryList::const_iterator i = favoriteHubs.begin(), iend = favoriteHubs.end(); i != iend; ++i) {
		FavoriteHubEntry* hub = *i;
		if(stricmp(hub->getServer(), aServer) == 0) {
			return hub;
		}
	}
	return NULL;
}
	
FavoriteHubEntryList FavoriteManager::getFavoriteHubs(const string& group) const {
	FavoriteHubEntryList ret;
	for(FavoriteHubEntryList::const_iterator i = favoriteHubs.begin(), iend = favoriteHubs.end(); i != iend; ++i)
		if((*i)->getGroup() == group)
			ret.push_back(*i);
	return ret;
}

bool FavoriteManager::isPrivate(const string& url) const {
	if(url.empty())
		return false;

	FavoriteHubEntry* fav = getFavoriteHubEntry(url);
	if(fav) {
		const string& name = fav->getGroup();
		if(!name.empty()) {
			FavHubGroups::const_iterator group = favHubGroups.find(name);
			if(group != favHubGroups.end())
				return group->second.priv;
		}
	}
	return false;
}

bool FavoriteManager::hasSlot(const UserPtr& aUser) const { 
	Lock l(cs);
	FavoriteMap::const_iterator i = users.find(aUser->getCID());
	if(i == users.end())
		return false;
	return i->second.isSet(FavoriteUser::FLAG_GRANTSLOT);
}

void FavoriteManager::setSuperUser(const UserPtr& aUser, bool superUser) {
        Lock l(cs);
        FavoriteMap::iterator i = users.find(aUser->getCID());
        if(i == users.end())
                return;
		if(superUser) {
                i->second.setFlag(FavoriteUser::FLAG_SUPERUSER);
		} else {
                i->second.unsetFlag(FavoriteUser::FLAG_SUPERUSER);
		}
        save();
}

time_t FavoriteManager::getLastSeen(const UserPtr& aUser) const { 
	Lock l(cs);
	FavoriteMap::const_iterator i = users.find(aUser->getCID());
	if(i == users.end())
		return 0;
	return i->second.getLastSeen();
}

void FavoriteManager::setAutoGrant(const UserPtr& aUser, bool grant) {
	Lock l(cs);
	FavoriteMap::iterator i = users.find(aUser->getCID());
	if(i == users.end())
		return;
	if(grant)
		i->second.setFlag(FavoriteUser::FLAG_GRANTSLOT);
	else
		i->second.unsetFlag(FavoriteUser::FLAG_GRANTSLOT);
	save();
}
void FavoriteManager::setUserDescription(const UserPtr& aUser, const string& description) {
	Lock l(cs);
	FavoriteMap::iterator i = users.find(aUser->getCID());
	if(i == users.end())
		return;
	i->second.setDescription(description);
	save();
}

void FavoriteManager::setIgnoredUserDescription(const UserPtr& aUser, const string& description) {
	Lock l(cs);
	FavoriteMap::iterator i = ignoredUsers.find(aUser->getCID());
	if(i == ignoredUsers.end())
		return;
	i->second.setDescription(description);
	save();
}

void FavoriteManager::recentload(SimpleXML& aXml) {
	aXml.resetCurrentChild();
	if(aXml.findChild("Hubs")) {
		aXml.stepIn();
		while(aXml.findChild("Hub")) {
			RecentHubEntry* e = new RecentHubEntry();
			e->setName(aXml.getChildAttrib("Name"));
			e->setDescription(aXml.getChildAttrib("Description"));
			e->setUsers(aXml.getChildAttrib("Users"));
			e->setShared(aXml.getChildAttrib("Shared"));
			e->setServer(aXml.getChildAttrib("Server"));
			recentHubs.push_back(e);
		}
		aXml.stepOut();
	}
}

StringList FavoriteManager::getHubLists() {
	StringTokenizer<string> lists(SETTING(HUBLIST_SERVERS), ';');
	return lists.getTokens();
}

const string& FavoriteManager::blacklisted(const string& domain) const {
	if(domain.empty())
		return Util::emptyString;

	// get the host
	string server, server_short = Util::emptyString;
	uint16_t port = 0;
	string file, query, proto, fragment;
	Util::decodeUrl(domain, proto, server, port, file, query, fragment);
	// only keep the last 2 words (example.com)
	size_t pos = server.rfind('.');
	if(pos == string::npos || pos == 0 || pos >= server.size() - 2)
		return Util::emptyString;
	pos = server.rfind('.', pos - 1);
	if(pos != string::npos)
		server_short = server.substr(pos + 1);

	// first look only at host match, then check subdomains
	StringMap::const_iterator i = blacklist.find(server_short);
	if(i != blacklist.end())
		return i->second;
	i = blacklist.find(server);
	if(i != blacklist.end())
		return i->second;

	return Util::emptyString;
}

void FavoriteManager::addBlacklist(const string& domain, const string& reason) {
	if(domain.empty() || reason.empty())
 		return;
	blacklist[domain] = reason;
}

FavoriteHubEntryList::const_iterator FavoriteManager::getFavoriteHub(const string& aServer) const {
	for(FavoriteHubEntryList::const_iterator i = favoriteHubs.begin(); i != favoriteHubs.end(); ++i) {
		if(stricmp((*i)->getServer(), aServer) == 0) {
			return i;
		}
	}
	return favoriteHubs.end();
}

RecentHubEntry::Iter FavoriteManager::getRecentHub(const string& aServer) const {
	for(RecentHubEntry::Iter i = recentHubs.begin(); i != recentHubs.end(); ++i) {
		if(stricmp((*i)->getServer(), aServer) == 0) {
			return i;
		}
	}
	return recentHubs.end();
}

void FavoriteManager::setHubList(int aHubList) {
	lastServer = aHubList;
	refresh();
}

void FavoriteManager::refresh(bool forceDownload /* = false */) {
	StringList sl = getHubLists();
	if(sl.empty())
		return;

	publicListServer = sl[(lastServer) % sl.size()];
	if(strnicmp(publicListServer.c_str(), "http", 4) != 0) {
		lastServer++;
		return;
	}

	if(!forceDownload) {
		string path = Util::getHubListsPath() + Util::validateFileName(publicListServer);
		if(File::getSize(path) > 0) {
			useHttp = false;
			string buf, fileDate;
			{
				Lock l(cs);
				publicListMatrix[publicListServer].clear();
			}
			listType = (stricmp(path.substr(path.size() - 4), ".bz2") == 0) ? TYPE_BZIP2 : TYPE_NORMAL;
			try {
				File cached(path, File::READ, File::OPEN);
				buf = cached.read();
				char buf[20];
				time_t fd = cached.getLastModified();
				if (strftime(buf, 20, "%x", localtime(&fd))) {
					fileDate = string(buf);
				}
			} catch(const FileException&) { }
			if(!buf.empty()) {
				if (onHttpFinished(buf)) {
					fire(FavoriteManagerListener::LoadedFromCache(), publicListServer, fileDate);
				}		
				return;
			}
		}
	}

	if(!running) {
		useHttp = true;
		{
			Lock l(cs);
			publicListMatrix[publicListServer].clear();
		}
		c = HttpManager::getInstance()->download(publicListServer);
		running = true;
	}
}

UserCommand::List FavoriteManager::getUserCommands(int ctx, const StringList& hubs, bool& op) {
	vector<bool> isOp(hubs.size());

	for(size_t i = 0; i < hubs.size(); ++i) {
		if(ClientManager::getInstance()->isOp(ClientManager::getInstance()->getMe(), hubs[i])) {
			isOp[i] = true;
			op = true; // ugly hack
		}
	}

	Lock l(cs);
	UserCommand::List lst;
	for(UserCommand::List::iterator i = userCommands.begin(); i != userCommands.end(); ++i) {
		UserCommand& uc = *i;
		if(!(uc.getCtx() & ctx)) {
			continue;
		}

		for(size_t j = 0; j < hubs.size(); ++j) {
			const string& hub = hubs[j];
			bool hubAdc = hub.compare(0, 6, "adc://") == 0 || hub.compare(0, 7, "adcs://") == 0;
			bool commandAdc = uc.getHub().compare(0, 6, "adc://") == 0 || uc.getHub().compare(0, 7, "adcs://") == 0;
			if(hubAdc && commandAdc) {
				if((uc.getHub() == "adc://" || uc.getHub() == "adcs://") ||
					((uc.getHub() == "adc://op" || uc.getHub() == "adcs://op") && isOp[j]) ||
					(uc.getHub() == hub) )
				{
					lst.push_back(*i);
					break;
				}
			} else if((!hubAdc && !commandAdc) || uc.isChat()) {
				if((uc.getHub().length() == 0) || 
					(uc.getHub() == "op" && isOp[j]) ||
					(uc.getHub() == hub) )
				{
					lst.push_back(*i);
					break;
				}
			}
		}
	}
	return lst;
}

// HttpManagerListener
void FavoriteManager::on(HttpManagerListener::Added, HttpConnection* c) noexcept {
	if(c != this->c) { return; }
	fire(FavoriteManagerListener::DownloadStarting(), c->getUrl());
}

void FavoriteManager::on(HttpManagerListener::Failed, HttpConnection* c, const string& str) noexcept {
	if(c != this->c) { return; }
	lastServer++;
	running = false;
	if(useHttp) {
		fire(FavoriteManagerListener::DownloadFailed(), str);
	}
}

void FavoriteManager::on(HttpManagerListener::Complete, HttpConnection* c, OutputStream* stream) noexcept {
	if(c != this->c) { return; }
	bool parseSuccess = false;
	if(useHttp) {
		if(c->getMimeType() == "application/x-bzip2")
			listType = TYPE_BZIP2;
		parseSuccess = onHttpFinished(static_cast<StringOutputStream*>(stream)->getString());
	}	
	running = false;
	if(parseSuccess) {
		fire(FavoriteManagerListener::DownloadFinished(), c->getUrl());
	}
}

void FavoriteManager::on(UserUpdated, const OnlineUser& user) noexcept {
	userUpdated(user);
}
void FavoriteManager::on(UserDisconnected, const UserPtr& user) noexcept {
	bool isFav = false;
	{
		Lock l(cs);
		FavoriteMap::iterator i = users.find(user->getCID());
		if(i != users.end()) {
			isFav = true;
			i->second.setLastSeen(GET_TIME());
			save();
		}
	}
	if(isFav)
		fire(FavoriteManagerListener::StatusChanged(), user);
}

void FavoriteManager::on(UserConnected, const UserPtr& user) noexcept {
	bool isFav = false;
	{
		Lock l(cs);
		FavoriteMap::const_iterator i = users.find(user->getCID());
		if(i != users.end()) {
			isFav = true;
		}
	}
	if(isFav)
		fire(FavoriteManagerListener::StatusChanged(), user);
}

void FavoriteManager::previewload(SimpleXML& aXml){
	aXml.resetCurrentChild();
	if(aXml.findChild("PreviewApps")) {
		aXml.stepIn();
		while(aXml.findChild("Application")) {					
			addPreviewApp(aXml.getChildAttrib("Name"), aXml.getChildAttrib("Application"), 
				aXml.getChildAttrib("Arguments"), aXml.getChildAttrib("Extension"));			
		}
		aXml.stepOut();
	}	
}

void FavoriteManager::previewsave(SimpleXML& aXml){
	aXml.addTag("PreviewApps");
	aXml.stepIn();
	for(PreviewApplication::Iter i = previewApplications.begin(); i != previewApplications.end(); ++i) {
		aXml.addTag("Application");
		aXml.addChildAttrib("Name", (*i)->getName());
		aXml.addChildAttrib("Application", (*i)->getApplication());
		aXml.addChildAttrib("Arguments", (*i)->getArguments());
		aXml.addChildAttrib("Extension", (*i)->getExtension());
	}
	aXml.stepOut();
}

bool FavoriteManager::getActiveAction(FavoriteHubEntry* entry, int actionId) {
	FavoriteHubEntry::Iter h = find(favoriteHubs.begin(), favoriteHubs.end(), entry);
	if(h == favoriteHubs.end()) {
		return false;
	}

	FavoriteHubEntry::Action::Iter i;
	if((i = (*h)->action.find(actionId)) != (*h)->action.end()) {
		return i->second->getActive();
	}
	(*h)->action.insert(make_pair(actionId, new FavoriteHubEntry::Action(false)));
	return false;
}

void FavoriteManager::setActiveAction(FavoriteHubEntry* entry, int actionId, bool active) {
	FavoriteHubEntry::Iter h = find(favoriteHubs.begin(), favoriteHubs.end(), entry);
	if(h == favoriteHubs.end()) {
		return;
	}

	FavoriteHubEntry::Action::Iter i;
	if((i = (*h)->action.find(actionId)) != (*h)->action.end()) {
		i->second->setActive(active);
		return;
	}
	if(active)
		(*h)->action.insert(make_pair(actionId, new FavoriteHubEntry::Action(true)));
}

bool FavoriteManager::getActiveRaw(FavoriteHubEntry* entry, int actionId, int rawId) {
	FavoriteHubEntry::Iter h = find(favoriteHubs.begin(), favoriteHubs.end(), entry);
	if(h == favoriteHubs.end()) {
		return false;
	}

	FavoriteHubEntry::Action::Iter i;
	if((i = (*h)->action.find(actionId)) != (*h)->action.end()) {
		for(FavoriteHubEntry::Action::RawIter j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
			if(j->getRawId() == rawId) {
				return true;
			}
		}
	}
	return false;
}

void FavoriteManager::setActiveRaw(FavoriteHubEntry* entry, int actionId, int rawId, bool active) {
	FavoriteHubEntry::Iter h = find(favoriteHubs.begin(), favoriteHubs.end(), entry);
	if(h == favoriteHubs.end()) {
		return;
	}

	FavoriteHubEntry::Action::Iter i;
	if((i = (*h)->action.find(actionId)) != (*h)->action.end()) {
		for(FavoriteHubEntry::Action::RawList::iterator j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
			if(j->getRawId() == rawId) {
				if(!active)
					i->second->raw.erase(j);
				return;
			}
		}
		if(active)
			i->second->raw.push_back(FavoriteHubEntry::Action::Raw(rawId));
		return;
	}
	if(active) {
		FavoriteHubEntry::Action* act = (*h)->action.insert(make_pair(actionId, new FavoriteHubEntry::Action(true))).first->second;
		act->raw.push_back(FavoriteHubEntry::Action::Raw(rawId));
	}
}

string FavoriteManager::getAwayMessage(const string& aServer, StringMap& params) {
	FavoriteHubEntry* hub = getFavoriteHubEntry(aServer);
	if(hub)
		return hub->getAwayMsg().empty() ? Util::getAwayMessage(params) : Util::formatParams(hub->getAwayMsg(), params, false);
	return Util::getAwayMessage(params);
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
