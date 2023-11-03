/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "ShareManager.h"

#include "ResourceManager.h"
#include "AdcHub.h"
#include "BZUtils.h"
#include "ClientManager.h"
#include "CryptoManager.h"
#include "Download.h"
#include "File.h"
#include "FilteredFile.h"
#include "LogManager.h"
#include "HashBloom.h"
#include "HashManager.h"
#include "QueueManager.h"
#include "RegEx.h"
#include "ScopedFunctor.h"
#include "SearchResult.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "Transfer.h"
#include "UserConnection.h"
#include "version.h"

#include "../dht/IndexManager.h"

#ifdef _WIN32
#include "w.h"
#include "shlobj.h"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>
#endif

#include <limits>

namespace dcpp {

using std::list;
using std::move;
using std::numeric_limits;

atomic_flag ShareManager::refreshing = ATOMIC_FLAG_INIT;

ShareManager::ShareManager() : hits(0), xmlListLen(0), bzXmlListLen(0),
	xmlDirty(true), forceXmlRefresh(true), refreshDirs(false), update(false), listN(0),
	lastXmlUpdate(0), lastFullUpdate(GET_TICK()), bloom(1<<20), sharedSize(0)
{
	SettingsManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	HashManager::getInstance()->addListener(this);
}

ShareManager::~ShareManager() {
	SettingsManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	HashManager::getInstance()->removeListener(this);

	join();

	if(bzXmlRef.get()) {
		bzXmlRef.reset();
		File::deleteFile(getBZXmlFile());
	}
}

ShareManager::Directory::Directory(const string& aName, const ShareManager::Directory::Ptr& aParent) :
	size(0),
	name(aName),
	parent(aParent.get())
{
}

const string& ShareManager::Directory::getRealName() const noexcept {
	return realName ? realName.get() : name;
}

string ShareManager::Directory::getADCPath() const noexcept {
	if(!getParent())
		return '/' + name + '/';
	return getParent()->getADCPath() + name + '/';
}

string ShareManager::Directory::getFullName() const noexcept {
	if(!getParent())
		return getName() + '\\';
	return getParent()->getFullName() + getName() + '\\';
}

string ShareManager::Directory::getRealPath(const std::string& path) const {
	if(getParent()) {
		return getParent()->getRealPath(getRealName() + PATH_SEPARATOR_STR + path);
	} else {
		return ShareManager::getInstance()->findRealRoot(getRealName(), path);
	}
}

string ShareManager::findRealRoot(const string& virtualRoot, const string& virtualPath) const {
	for(auto& i: shares) {
		if(stricmp(i.second, virtualRoot) == 0) {
			auto name = i.first + virtualPath;
			dcdebug("Matching %s\n", name.c_str());
			if(Util::fileExists(name)) {
				return name;
			}
		}
	}

	throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
}

int64_t ShareManager::Directory::getSize() const noexcept {
	int64_t tmp = size;
	for(auto& i: directories)
		tmp += i.second->getSize();
	return tmp;
}

size_t ShareManager::Directory::countFiles() const noexcept {
	size_t tmp = files.size();
	for(auto& i: directories)
		tmp += i.second->countFiles();
	return tmp;
}

string ShareManager::toVirtual(const TTHValue& tth) const {
	if(bzXmlRoot && tth == bzXmlRoot) {
		return Transfer::USER_LIST_NAME_BZ;
	} else if(xmlRoot && tth == xmlRoot) {
		return Transfer::USER_LIST_NAME;
	}

	Lock l(cs);
	auto i = tthIndex.find(tth);
	if(i != tthIndex.end()) {
		return i->second->getADCPath();
	} else {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}
}

string ShareManager::toReal(const string& virtualFile, bool hideShare) {
	return toRealWithSize(virtualFile, hideShare).first;
}

pair<string, int64_t> ShareManager::toRealWithSize(const string& virtualFile, bool hideShare) {
	Lock l(cs);

	if(virtualFile == "MyList.DcLst") {
		throw ShareException("NMDC-style lists no longer supported, please upgrade your client");
	}
	if(virtualFile == Transfer::USER_LIST_NAME_BZ || virtualFile == Transfer::USER_LIST_NAME) {
		generateXmlList();
		if(hideShare)
			return make_pair(Util::getPath(Util::PATH_USER_CONFIG) + "Emptyfiles.xml.bz2", 0);
		return make_pair(getBZXmlFile(), 0);
	}

	auto f = findFile(virtualFile);
	return make_pair(f.getRealPath(), f.getSize());
}

StringList ShareManager::getRealPaths(const string& virtualPath, bool hideShare) {
	if(virtualPath.empty())
		throw ShareException("empty virtual path");

	StringList ret;

	Lock l(cs);

	if(*(virtualPath.end() - 1) == '/') {
		// directory
		Directory::Ptr d = splitVirtual(virtualPath).first;

		// imitate Directory::getRealPath
		if(d->getParent()) {
			ret.push_back(d->getParent()->getRealPath(d->getName()));
		} else {
			for(auto& i: shares) {
				if(stricmp(i.second, d->getName()) == 0) {
					// remove the trailing path sep
					if(FileFindIter(i.first.substr(0, i.first.size() - 1)) != FileFindIter()) {
						ret.push_back(i.first);
					}
				}
			}
		}

	} else {
		// file
		ret.push_back(toReal(virtualPath, hideShare));
	}

	return ret;
}

optional<TTHValue> ShareManager::getTTH(const string& virtualFile) const {
	Lock l(cs);
	if(virtualFile == Transfer::USER_LIST_NAME_BZ) {
		return bzXmlRoot;
	} else if(virtualFile == Transfer::USER_LIST_NAME) {
		return xmlRoot;
	}

	return findFile(virtualFile).tth;
}

MemoryInputStream* ShareManager::getTree(const string& virtualFile) const {
	TigerTree tree;
	if(virtualFile.compare(0, 4, "TTH/") == 0) {
		if(!HashManager::getInstance()->getTree(TTHValue(virtualFile.substr(4)), tree))
			return nullptr;
	} else {
		try {
			auto tth = getTTH(virtualFile);
			if(!tth) { return nullptr; }
			HashManager::getInstance()->getTree(*tth, tree);
		} catch(const Exception&) {
			return nullptr;
		}
	}

	ByteVector buf = tree.getLeafData();
	return new MemoryInputStream(&buf[0], buf.size());
}

AdcCommand ShareManager::getFileInfo(const string& aFile) {
	if(aFile == Transfer::USER_LIST_NAME) {
		generateXmlList();
		if(!xmlRoot) {
			throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
		}

		AdcCommand cmd(AdcCommand::CMD_RES);
		cmd.addParam("FN", aFile);
		cmd.addParam("SI", Util::toString(xmlListLen));
		cmd.addParam("TR", xmlRoot->toBase32());
		return cmd;
	}

	if(aFile == Transfer::USER_LIST_NAME_BZ) {
		generateXmlList();
		if(!bzXmlRoot) {
			throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
		}

		AdcCommand cmd(AdcCommand::CMD_RES);
		cmd.addParam("FN", aFile);
		cmd.addParam("SI", Util::toString(bzXmlListLen));
		cmd.addParam("TR", bzXmlRoot->toBase32());
		return cmd;
	}

	if(aFile.compare(0, 4, "TTH/") != 0)
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);

	TTHValue val(aFile.substr(4));
	Lock l(cs);
	auto i = tthIndex.find(val);
	if(i == tthIndex.end()) {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}

	const Directory::File& f = *i->second;
	AdcCommand cmd(AdcCommand::CMD_RES);
	cmd.addParam("FN", f.getADCPath());
	cmd.addParam("SI", Util::toString(f.getSize()));
	cmd.addParam("TR", f.tth->toBase32());
	return cmd;
}

pair<ShareManager::Directory::Ptr, string> ShareManager::splitVirtual(const string& virtualPath) const {
	if(virtualPath.empty() || virtualPath[0] != '/') {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}

	auto i = virtualPath.find('/', 1);
	if(i == string::npos || i == 1) {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}

	auto dmi = directories.find(virtualPath.substr(1, i - 1));
	if(dmi == directories.end()) {
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	}

	auto d = dmi->second;

	auto j = i + 1;
	while((i = virtualPath.find('/', j)) != string::npos) {
		auto mi = d->directories.find(virtualPath.substr(j, i - j));
		j = i + 1;
		if(mi == d->directories.end())
			throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
		d = mi->second;
	}

	return make_pair(d, virtualPath.substr(j));
}

const ShareManager::Directory::File& ShareManager::findFile(const string& virtualFile) const {
	if(virtualFile.compare(0, 4, "TTH/") == 0) {
		auto i = tthIndex.find(TTHValue(virtualFile.substr(4)));
		if(i == tthIndex.end()) {
			throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
		}
		return *i->second;
	}

	auto v = splitVirtual(virtualFile);
	auto it = v.first->findFile(v.second);
	if(it == v.first->files.end())
		throw ShareException(UserConnection::FILE_NOT_AVAILABLE);
	return *it;
}

string ShareManager::validateVirtual(const string& aVirt) const noexcept {
	string tmp = aVirt;
	string::size_type idx = 0;

	while( (idx = tmp.find_first_of("\\/"), idx) != string::npos) {
		tmp[idx] = '_';
	}
	return tmp;
}

bool ShareManager::hasVirtual(const string& virtualName) const noexcept {
	Lock l(cs);
	return directories.find(virtualName) != directories.end();
}

void ShareManager::load(SimpleXML& aXml) {
	Lock l(cs);

	aXml.resetCurrentChild();
	if(aXml.findChild("Share")) {
		aXml.stepIn();
		while(aXml.findChild("Directory")) {
			string realPath = aXml.getChildData();
			if(realPath.empty()) {
				continue;
			}
			// make sure realPath ends with a PATH_SEPARATOR
			if(realPath[realPath.size() - 1] != PATH_SEPARATOR) {
				realPath += PATH_SEPARATOR;
			}

			if(!Util::fileExists(realPath))
				continue;

			const string& virtualName = aXml.getChildAttrib("Virtual");
			string vName = validateVirtual(virtualName.empty() ? Util::getLastDir(realPath) : virtualName);
			shares[move(realPath)] = vName;
			if(directories.find(vName) == directories.end()) {
				directories[vName] = Directory::create(vName);
			}
		}
		aXml.stepOut();
	}
	if(aXml.findChild("NoShare")) {
		aXml.stepIn();
		while(aXml.findChild("Directory"))
			notShared.emplace_back(aXml.getChildData());

		aXml.stepOut();
	}
}

void ShareManager::save(SimpleXML& aXml) {
	Lock l(cs);

	aXml.addTag("Share");
	aXml.stepIn();
	for(auto& i: shares) {
		aXml.addTag("Directory", i.first);
		aXml.addChildAttrib("Virtual", i.second);
	}
	aXml.stepOut();
	aXml.addTag("NoShare");
	aXml.stepIn();
	for(auto& i: notShared) {
		aXml.addTag("Directory", i);
	}
	aXml.stepOut();
}

void ShareManager::addDirectory(const string& realPath, const string& virtualName) {
	if(!Util::fileExists(realPath))
		throw ShareException(STRING(NO_DIRECTORY_SPECIFIED));

	if(realPath.empty() || virtualName.empty()) {
		throw ShareException(STRING(NO_DIRECTORY_SPECIFIED));
	}

	if (!checkHidden(realPath)) {
		throw ShareException(STRING(DIRECTORY_IS_HIDDEN));
	}

	if(stricmp(SETTING(TEMP_DOWNLOAD_DIRECTORY), realPath) == 0) {
		throw ShareException(STRING(DONT_SHARE_TEMP_DIRECTORY));
	}

#ifdef _WIN32
	// don't share Windows directory
	TCHAR path[MAX_PATH];
	::SHGetFolderPath(NULL, CSIDL_WINDOWS, NULL, SHGFP_TYPE_CURRENT, path);
	if(strnicmp(realPath, Text::fromT((tstring)path) + PATH_SEPARATOR, _tcslen(path) + 1) == 0) {
		char buf[MAX_PATH];
		snprintf(buf, sizeof(buf), CSTRING(CHECK_FORBIDDEN), realPath.c_str());
		throw ShareException(buf);
	}
#endif

	list<string> removeMap;
	{
		Lock l(cs);

		for(auto& i: shares) {
			if(strnicmp(realPath, i.first, i.first.length()) == 0) {
				// Trying to share an already shared directory
				removeMap.emplace_front(i.first);
			} else if(strnicmp(realPath, i.first, realPath.length()) == 0) {
				// Trying to share a parent directory
				removeMap.emplace_front(i.first);	
			}
		}
	}

	for(auto& i: removeMap) {
		removeDirectory(i);
	}

	HashManager::HashPauser pauser;	

	auto dp = buildTree(realPath, boost::none);

	string vName = validateVirtual(virtualName);
	dp->setName(vName);

	Lock l(cs);

	shares[realPath] = move(vName);

	merge(dp, realPath);

	rebuildIndices();
	setDirty();
}

void ShareManager::merge(const Directory::Ptr& directory, const string& realPath) {
	auto i = directories.find(directory->getName());
	if(i != directories.end()) {
		dcdebug("Merging directory <%s> into %s\n", realPath.c_str(), directory->getName().c_str());
		i->second->merge(directory, realPath);

	} else {
		dcdebug("Adding new directory %s\n", directory->getName().c_str());
		directories[directory->getName()] = directory;
	}
}

void ShareManager::Directory::merge(const Directory::Ptr& source, const string& realPath) {
	// merge directories
	for(auto& i: source->directories) {
		auto subSource = i.second;

		auto ti = directories.find(subSource->getName());
		if(ti == directories.end()) {
			// the directory doesn't exist; create it.
			directories.emplace(subSource->getName(), subSource);
			subSource->parent = this;

			auto f = findFile(subSource->getName());
			if(f != files.end()) {
				// we have a file that has the same name as the dir being merged; rename it.
				const_cast<File&>(*f).validateName(Util::getFilePath(f->getRealPath()));
			}

		} else {
			// the directory was already existing; merge into it.
			auto subTarget = ti->second;
			subTarget->merge(subSource, realPath + subSource->getName() + PATH_SEPARATOR);
		}
	}

	// merge files
	for(auto& i: source->files) {
		auto& file = const_cast<File&>(i);

		file.setParent(this);
		file.validateName(realPath);

		files.insert(move(file));
	}
}

bool ShareManager::Directory::nameInUse(const string& name) const {
	return findFile(name) != files.end() || directories.find(name) != directories.end();
}

void ShareManager::Directory::File::validateName(const string& sourcePath) {
	if(parent->nameInUse(name)) {
		uint32_t num = 0;
		string base = name, ext, vname;
		auto dot = base.rfind('.');
		if(dot != string::npos) {
			ext = base.substr(dot);
			base.erase(dot);
		}
		do {
			++num;
			vname = base + " (" + Util::toString(num) + ")" + ext;
		} while(parent->nameInUse(vname));
		dcdebug("Renaming duplicate <%s> to <%s>\n", name.c_str(), vname.c_str());
		realPath = sourcePath + move(name);
		name = move(vname);
	} else {
		realPath.reset();
	}
}

void ShareManager::removeDirectory(const string& realPath) {
	if(realPath.empty())
		return;

	HashManager::getInstance()->stopHashing(realPath);

	Lock l(cs);

	auto i = shares.find(realPath);
	if(i == shares.end()) {
		return;
	}

	auto vName = i->second;
	directories.erase(vName);

	shares.erase(i);

	HashManager::HashPauser pauser;

	// Readd all directories with the same vName
	for(i = shares.begin(); i != shares.end(); ++i) {
		if(stricmp(i->second, vName) == 0 && checkHidden(i->first)) {
			auto dp = buildTree(i->first, boost::none);
			dp->setName(i->second);
			merge(dp, i->first);
		}
	}

	rebuildIndices();
	setDirty();
}

void ShareManager::renameDirectory(const string& realPath, const string& virtualName) {
	removeDirectory(realPath);
	addDirectory(realPath, virtualName);
}

int64_t ShareManager::getShareSize(const string& realPath) const noexcept {
	Lock l(cs);
	dcassert(realPath.size()>0);
	auto i = shares.find(realPath);

	if(i != shares.end()) {
		auto j = directories.find(i->second);
		if(j != directories.end()) {
			return j->second->getSize();
		}
	}
	return -1;
}

int64_t ShareManager::getShareSize() const noexcept {
	Lock l(cs);
	int64_t tmp = 0;
	for(auto& i: tthIndex) {
		tmp += i.second->getSize();
	}
	return tmp;
}

size_t ShareManager::getSharedFiles() const noexcept {
	Lock l(cs);
	return tthIndex.size();
}

namespace {

string fileSizeString(int64_t size)
{
	return size == -1 ? STRING(NA) : Util::formatBytes(size);
}

}	// unnamed namespace

ShareManager::Directory::Ptr ShareManager::buildTree(const string& realPath, optional<const string&> dirName, const Directory::Ptr& parent) {
	auto dir = Directory::create(dirName ? dirName.get() : Util::getLastDir(realPath), parent);

	auto lastFileIter = dir->files.begin();

#ifdef _WIN32
	for(FileFindIter i(realPath + "*"), end; i != end; ++i) {
#else
	//the fileiter just searches directorys for now, not sure if more
	//will be needed later
	for(FileFindIter i(realPath), end; i != end; ++i) {
#endif
		auto name = i->getFileName();

		if(name.empty()) {
			LogManager::getInstance()->message(str(F_("Invalid file name found while hashing folder %1%") % Util::addBrackets(realPath)), LogManager::LOG_WARNING);
			continue;
		}

		if(name == "." || name == "..")
			continue;
		if(!BOOLSETTING(SHARE_HIDDEN) && i->isHidden())
			continue;
		if(i->isLink())
 			continue;

		if(Wildcard::match(name, SETTING(SKIPLIST_SHARE), ';'))
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
				(nameLen >= 28 && name.find("download") == 0 && name.rfind(".dat") == nameLen - 4 && Util::toInt64(name.substr(8, 16)))) {		//kazaa temps
					LogManager::getInstance()->message(STRING(FORBIDDEN_FILE_NOT_SHARED) + name +
																  " (" + STRING(SIZE) + ": " + fileSizeString(File::getSize(realPath + name)) + ") (" +
																  STRING(DIRECTORY) + ": \"" + realPath + "\")",
																  LogManager::LOG_WARNING);
					continue;
			}
		}
 
		if(i->isDirectory()) {
			auto newRealPath = realPath + name + PATH_SEPARATOR;

			// don't share unfinished downloads or exclusions
			if(newRealPath == SETTING(TEMP_DOWNLOAD_DIRECTORY) || !shareFolder(newRealPath)) { continue; }

#ifdef _WIN32
			// don't share Windows directory
			TCHAR path[MAX_PATH];
			::SHGetFolderPath(NULL, CSIDL_WINDOWS, NULL, SHGFP_TYPE_CURRENT, path);
			if(strnicmp(newRealPath, Text::fromT((tstring)path) + PATH_SEPARATOR, _tcslen(path) + 1) == 0) { continue; }
#endif

			auto virtualName = name;
			if(dir->nameInUse(virtualName)) {
				uint32_t num = 0;
				do {
					++num;
					virtualName = name + " (" + Util::toString(num) + ")";
				} while(dir->nameInUse(virtualName));
			}

			dir->directories[virtualName] = buildTree(newRealPath, virtualName, dir);

			if(virtualName != name) {
				dir->directories[virtualName]->setRealName(move(name));
			}

		} else {
			// Not a directory, assume it's a file...make sure we're not sharing the settings file...
			if( (stricmp(name.c_str(), "DCPlusPlus.xml") == 0) || 
				(stricmp(name.c_str(), "Favorites.xml") == 0) ||
				(stricmp(Util::getFileExt(name).c_str(), ".dctmp") == 0) ||
				(stricmp(Util::getFileExt(name).c_str(), ".antifrag") == 0) ) { continue; }

			auto size = i->getSize();
			auto fileName = realPath + name;

			// don't share the private key file
			if(fileName == SETTING(TLS_PRIVATE_KEY_FILE)) { continue; }

			Directory::File f(name, size, dir,
				HashManager::getInstance()->getTTH(fileName, size, i->getLastWriteTime()));
			f.validateName(realPath);
			lastFileIter = dir->files.insert(lastFileIter, move(f));
		}
	}

	return dir;
}

bool ShareManager::checkHidden(const string& realPath) const {
	FileFindIter ff = FileFindIter(realPath.substr(0, realPath.size() - 1));

	if (ff != FileFindIter()) {
		return (BOOLSETTING(SHARE_HIDDEN) || !ff->isHidden());
	}

	return true;
}

void ShareManager::updateIndices(Directory& dir) {
	bloom.add(Text::toLower(dir.getName()));

	for(auto& i: dir.directories) {
		updateIndices(*i.second);
	}

	dir.size = 0;

	for(auto i = dir.files.begin(); i != dir.files.end(); ) {
		updateIndices(dir, i++);
	}
}

void ShareManager::rebuildIndices() {
	sharedSize = 0;
	tthIndex.clear();
	bloom.clear();

	for(auto& i: directories) {
		updateIndices(*i.second);
	}
}

void ShareManager::updateIndices(Directory& dir, const decltype(std::declval<Directory>().files.begin())& i) {
	const Directory::File& f = *i;

	if(!f.tth) {
		return;
	}

	auto j = tthIndex.find(*f.tth);
	if(j == tthIndex.end()) {
		dir.size += f.getSize();
		sharedSize += f.getSize();
	}

	tthIndex[*f.tth] = &f;
	bloom.add(Text::toLower(f.getName()));

	dht::IndexManager* im = dht::IndexManager::getInstance();
	if(im && im->isTimeForPublishing())
		im->publishFile(*f.tth, f.getSize());
}

void ShareManager::refresh(bool dirs, bool aUpdate, bool block, function<void (float)> progressF) noexcept {
	if(refreshing.test_and_set()) {
		LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_IN_PROGRESS), LogManager::LOG_WARNING);
		return;
	}

	update = aUpdate;
	refreshDirs = dirs;

	join();

	if(block) {
		runRefresh(progressF);

	} else {
		try {
			start();
			setThreadPriority(Thread::LOW);
		} catch(const ThreadException& e) {
			LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_FAILED) + " " + e.getError(), LogManager::LOG_ERROR);
		}
	}
}

StringPairList ShareManager::getDirectories() const noexcept {
	Lock l(cs);
	StringPairList ret;
	for(auto& i: shares) {
		ret.emplace_back(i.second, i.first);
	}
	return ret;
}

int ShareManager::run() {
	runRefresh();
	return 0;
}

void ShareManager::runRefresh(function<void (float)> progressF) {
	auto dirs = getDirectories();
	// Don't need to refresh if no directories are shared
	if(dirs.empty())
		refreshDirs = false;

	if(refreshDirs) {
		HashManager::HashPauser pauser;

		LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_INITIATED), LogManager::LOG_INFO);

		sharedSize = 0;
		lastFullUpdate = GET_TICK();

		vector<pair<Directory::Ptr, string>> newDirs;

		float progressCounter = 0, dirCount = dirs.size();

		for(auto& i: dirs) {
			if(checkHidden(i.second)) {
				auto dp = buildTree(i.second, boost::none);
				dp->setName(i.first);
				newDirs.emplace_back(dp, i.second);
			}

			if(progressF) {
				progressF(++progressCounter / dirCount);
			}
		}

		{
			Lock l(cs);
			directories.clear();

			for(auto& i: newDirs) {
				merge(i.first, i.second);
			}

			rebuildIndices();
		}
		refreshDirs = false;

		LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_FINISHED), LogManager::LOG_INFO);
	}

	if(update) {
		ClientManager::getInstance()->infoUpdated();
	}

	dht::IndexManager* im = dht::IndexManager::getInstance();
	if(im && im->isTimeForPublishing())
		im->setNextPublishing();
	
	refreshing.clear();
}

void ShareManager::getBloom(ByteVector& v, size_t k, size_t m, size_t h) const {
	dcdebug("Creating bloom filter, k=%u, m=%u, h=%u\n", k, m, h);
	Lock l(cs);

	HashBloom bloom;
	bloom.reset(k, m, h);
	for(auto& i: tthIndex) {
		bloom.add(i.first);
	}
	bloom.copy_to(v);
}

void ShareManager::generateXmlList() {
	Lock l(cs);
	if(forceXmlRefresh || (xmlDirty && (lastXmlUpdate + 15 * 60 * 1000 < GET_TICK() || lastXmlUpdate < lastFullUpdate))) {
		listN++;

		try {
			string tmp2;
			string indent;

			string newXmlName = Util::getPath(Util::PATH_USER_CONFIG) + "files" + Util::toString(listN) + ".xml.bz2";
			{
				File f(newXmlName, File::WRITE, File::TRUNCATE | File::CREATE);
				// We don't care about the leaves...
				CalcOutputStream<TTFilter<1024*1024*1024>, false> bzTree(&f);
				FilteredOutputStream<BZFilter, false> bzipper(&bzTree);
				CountOutputStream<false> count(&bzipper);
				CalcOutputStream<TTFilter<1024*1024*1024>, false> newXmlFile(&count);

				newXmlFile.write(SimpleXML::utf8Header);
				newXmlFile.write("<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"/\" Generator=\"" APPNAME " " VERSIONSTRING "\">\r\n");
				for(auto& i: directories) {
					i.second->toXml(newXmlFile, indent, tmp2, -1);
				}
				newXmlFile.write("</FileListing>");
				newXmlFile.flush();

				xmlListLen = count.getCount();

				newXmlFile.getFilter().getTree().finalize();
				bzTree.getFilter().getTree().finalize();

				xmlRoot = newXmlFile.getFilter().getTree().getRoot();
				bzXmlRoot = bzTree.getFilter().getTree().getRoot();
			}

			string emptyXmlName = Util::getPath(Util::PATH_USER_CONFIG) + "Emptyfiles.xml.bz2"; // Hide Share Mod
			if(!Util::fileExists(emptyXmlName)) {
				FilteredOutputStream<BZFilter, true> emptyXmlFile(new File(emptyXmlName, File::WRITE, File::TRUNCATE | File::CREATE));
				emptyXmlFile.write(SimpleXML::utf8Header);
				emptyXmlFile.write("<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"/\" Generator=\"DC++ " DCVERSIONSTRING "\">\r\n"); // Hide Share Mod
				emptyXmlFile.write("</FileListing>");
				emptyXmlFile.flush();
			}

			if(bzXmlRef.get()) {
				bzXmlRef.reset();
				File::deleteFile(getBZXmlFile());
			}

			try {
				File::renameFile(newXmlName, Util::getPath(Util::PATH_USER_CONFIG) + "files.xml.bz2");
				newXmlName = Util::getPath(Util::PATH_USER_CONFIG) + "files.xml.bz2";
			} catch(const FileException&) {
				// Ignore, this is for caching only...
			}
			bzXmlRef = unique_ptr<File>(new File(newXmlName, File::READ, File::OPEN));
			setBZXmlFile(newXmlName);
			bzXmlListLen = File::getSize(newXmlName);
			LogManager::getInstance()->message(str(F_("File list %1% generated") % Util::addBrackets(bzXmlFile)), LogManager::LOG_INFO);
		} catch(const Exception&) {
			// No new file lists...
		}

		xmlDirty = false;
		forceXmlRefresh = false;
		lastXmlUpdate = GET_TICK();
	}
}

MemoryInputStream* ShareManager::generatePartialList(const string& dir, bool recurse, bool hideShare) const {
	if(dir[0] != '/' || dir[dir.size()-1] != '/')
		return 0;

	if(hideShare) {
		string xml = SimpleXML::utf8Header;
		string tmp;
		xml += "<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"" + SimpleXML::escape(dir, tmp, true) + "\" Generator=\"" APPNAME " " VERSIONSTRING "\">\r\n";
		xml += "</FileListing>";

		return new MemoryInputStream(xml);
	}

	string xml = SimpleXML::utf8Header;
	string tmp;
	xml += "<FileListing Version=\"1\" CID=\"" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "\" Base=\"" + SimpleXML::escape(dir, tmp, true) + "\" Generator=\"" APPNAME " " VERSIONSTRING "\">\r\n";
	StringRefOutputStream sos(xml);
	string indent = "\t";

	Lock l(cs);
	if(dir == "/") {
		for(auto& i: directories) {
			tmp.clear();
			i.second->toXml(sos, indent, tmp, recurse ? -1 : 0);
		}
	} else {
		string::size_type i = 1, j = 1;

		Directory::Ptr root;

		bool first = true;
		while( (i = dir.find('/', j)) != string::npos) {
			if(i == j) {
				j++;
				continue;
			}

			if(first) {
				first = false;
				auto it = directories.find(dir.substr(j, i-j));
				if(it == directories.end())
					return 0;
				root = it->second;

			} else {
				auto it2 = root->directories.find(dir.substr(j, i-j));
				if(it2 == root->directories.end()) {
					return 0;
				}
				root = it2->second;
			}
			j = i + 1;
		}

		if(!root)
			return 0;

		for(auto& it2: root->directories) {
			it2.second->toXml(sos, indent, tmp, recurse ? -1 : 0);
		}
		root->filesToXml(sos, indent, tmp);
	}

	xml += "</FileListing>";
	return new MemoryInputStream(xml);
}

/* params for partial file lists - when any of these params is not satisfied, an incomplete dir is
returned rather than a full dir list. */
const int8_t maxLevel = 2;
const size_t maxItemsPerLevel[maxLevel] = { 16, 4 };

#define LITERAL(n) n, sizeof(n)-1
void ShareManager::Directory::toXml(OutputStream& xmlFile, string& indent, string& tmp2, int8_t level) const {
	xmlFile.write(indent);
	xmlFile.write(LITERAL("<Directory Name=\""));
	xmlFile.write(SimpleXML::escape(name, tmp2, true));

	if(level < 0 || (level < maxLevel && directories.size() + files.size() <= maxItemsPerLevel[level])) {
		xmlFile.write(LITERAL("\">\r\n"));

		indent += '\t';
		if(level >= 0)
			++level;

		for(auto& i: directories) {
			i.second->toXml(xmlFile, indent, tmp2, level);
		}
		filesToXml(xmlFile, indent, tmp2);

		if(level >= 0)
			--level;
		indent.erase(indent.length()-1);

		xmlFile.write(indent);
		xmlFile.write(LITERAL("</Directory>\r\n"));

	} else {
		if(directories.empty() && files.empty()) {
			xmlFile.write(LITERAL("\" />\r\n"));
		} else {
			xmlFile.write(LITERAL("\" Incomplete=\"1\" />\r\n"));
		}
	}
}

void ShareManager::Directory::filesToXml(OutputStream& xmlFile, string& indent, string& tmp2) const {
	for(auto& f: files) {
		if(!f.tth) { continue; }
		xmlFile.write(indent);
		xmlFile.write(LITERAL("<File Name=\""));
		xmlFile.write(SimpleXML::escape(f.getName(), tmp2, true));
		xmlFile.write(LITERAL("\" Size=\""));
		xmlFile.write(Util::toString(f.getSize()));
		xmlFile.write(LITERAL("\" TTH=\""));
		tmp2.clear();
		xmlFile.write(f.tth->toBase32(tmp2));
		xmlFile.write(LITERAL("\"/>\r\n"));
	}
}

ShareManager::SearchQuery::SearchQuery() :
	include(&includeInit),
	gt(0),
	lt(numeric_limits<int64_t>::max()),
	isDirectory(false)
{
}

namespace {
	inline uint16_t toCode(char a, char b) { return (uint16_t)a | ((uint16_t)b)<<8; }
}
ShareManager::SearchQuery::SearchQuery(const StringList& adcParams) :
	SearchQuery()
{
	for(auto& p: adcParams) {
		if(p.size() <= 2)
			continue;

		auto cmd = toCode(p[0], p[1]);
		if(toCode('T', 'R') == cmd) {
			root = TTHValue(p.substr(2));
			return;
		} else if(toCode('A', 'N') == cmd) {
			includeInit.emplace_back(p.substr(2));
		} else if(toCode('N', 'O') == cmd) {
			exclude.emplace_back(p.substr(2));
		} else if(toCode('E', 'X') == cmd) {
			ext.push_back(Text::toLower(p.substr(2)));
		} else if(toCode('G', 'R') == cmd) {
			auto exts = AdcHub::parseSearchExts(Util::toInt(p.substr(2)));
			ext.insert(ext.begin(), exts.begin(), exts.end());
		} else if(toCode('R', 'X') == cmd) {
			noExt.push_back(Text::toLower(p.substr(2)));
		} else if(toCode('G', 'E') == cmd) {
			gt = Util::toInt64(p.substr(2));
		} else if(toCode('L', 'E') == cmd) {
			lt = Util::toInt64(p.substr(2));
		} else if(toCode('E', 'Q') == cmd) {
			lt = gt = Util::toInt64(p.substr(2));
		} else if(toCode('T', 'Y') == cmd) {
			isDirectory = p[2] == '2';
		}
	}
}

ShareManager::SearchQuery::SearchQuery(const string& nmdcString, int searchType, int64_t size, int fileType) :
	SearchQuery()
{
	if(fileType == SearchManager::TYPE_TTH && nmdcString.compare(0, 4, "TTH:") == 0) {
		root = TTHValue(nmdcString.substr(4));

	} else {
		StringTokenizer<string> tok(Text::toLower(nmdcString), '$');
		for(auto& term: tok.getTokens()) {
			if(!term.empty()) {
				includeInit.emplace_back(term);
			}
		}

		if(searchType == SearchManager::SIZE_ATLEAST) {
			gt = size;
		} else if(searchType == SearchManager::SIZE_ATMOST) {
			lt = size;
		}

		switch(fileType) {
		case SearchManager::TYPE_AUDIO: ext = AdcHub::parseSearchExts(1 << 0); break;
		case SearchManager::TYPE_COMPRESSED: ext = AdcHub::parseSearchExts(1 << 1); break;
		case SearchManager::TYPE_DOCUMENT: ext = AdcHub::parseSearchExts(1 << 2); break;
		case SearchManager::TYPE_EXECUTABLE: ext = AdcHub::parseSearchExts(1 << 3); break;
		case SearchManager::TYPE_PICTURE: ext = AdcHub::parseSearchExts(1 << 4); break;
		case SearchManager::TYPE_VIDEO: ext = AdcHub::parseSearchExts(1 << 5); break;
		case SearchManager::TYPE_DIRECTORY: isDirectory = true; break;
		}
	}
}

bool ShareManager::SearchQuery::isExcluded(const string& str) {
	for(auto& i: exclude) {
		if(i.match(str))
			return true;
	}
	return false;
}

bool ShareManager::SearchQuery::hasExt(const string& name) {
	if(ext.empty())
		return true;
	if(!noExt.empty()) {
		ext = StringList(ext.begin(), set_difference(ext.begin(), ext.end(), noExt.begin(), noExt.end(), ext.begin()));
		noExt.clear();
	}
	auto fileExt = Util::getFileExt(name);
	return !fileExt.empty() && std::find(ext.cbegin(), ext.cend(), Text::toLower(fileExt.substr(1))) != ext.cend();
}

/**
 * Alright, the main point here is that when searching, a search string is most often found in
 * the filename, not directory name, so we want to make that case faster. Also, we want to
 * avoid changing StringLists unless we absolutely have to --> this should only be done if a string
 * has been matched in the directory name. This new stringlist should also be used in all descendants,
 * but not the parents...
 */
void ShareManager::Directory::search(SearchResultList& results, SearchQuery& query, size_t maxResults) const noexcept {
	if(query.isExcluded(name))
		return;

	// Find any matches in the directory name and removed matched terms from the query.
	unique_ptr<StringSearch::List> newTerms;

	for(auto& term: *query.include) {
		if(term.match(name)) {
			if(!newTerms) {
				newTerms.reset(new StringSearch::List(*query.include));
			}
			newTerms->erase(remove(newTerms->begin(), newTerms->end(), term), newTerms->end());
		}
	}

	auto const old = query.include;
	ScopedFunctor(([old, &query] { query.include = old; }));
	if(newTerms) {
		query.include = newTerms.get();
	}

	if(query.include->empty() && query.ext.empty() && query.gt == 0) {
		// We satisfied all the search words! Add the directory...
		/// @todo send the directory hash when we have one
		results.push_back(new SearchResult(SearchResult::TYPE_DIRECTORY, getSize(), getFullName(), TTHValue(string(39, 'A'))));
		ShareManager::getInstance()->addHits(1);
	}

	if(!query.isDirectory) {
		for(auto& i: files) {
			if(!i.tth) { continue; }

			// check the size
			if(!(i.getSize() >= query.gt)) {
				continue;
			} else if(!(i.getSize() <= query.lt)) {
				continue;
			}

			if(query.isExcluded(i.getName()))
				continue;

			// check if the name matches
			auto j = query.include->begin();
			for(; j != query.include->end() && j->match(i.getName()); ++j)
				;	// Empty
			if(j != query.include->end())
				continue;

			// check extensions
			if(!query.hasExt(i.getName()))
				continue;

			results.push_back(new SearchResult(SearchResult::TYPE_FILE, i.getSize(),
				getFullName() + i.getName(), *i.tth));
			ShareManager::getInstance()->addHits(1);

			if(results.size() >= maxResults) { return; }
		}
	}

	for(auto& dir: directories) {
		dir.second->search(results, query, maxResults);

		if(results.size() >= maxResults) { return; }
	}
}

SearchResultList ShareManager::search(SearchQuery&& query, size_t maxResults) noexcept {
	SearchResultList results;

	Lock l(cs);

	if(query.root) {
		auto i = tthIndex.find(*query.root);
		if(i != tthIndex.end()) {
			results.push_back(new SearchResult(SearchResult::TYPE_FILE, i->second->getSize(),
				i->second->getParent()->getFullName() + i->second->getName(), *i->second->tth));
			addHits(1);
		}
		return results;
	}

	for(auto& i: query.includeInit) {
		if(!bloom.match(i.getPattern()))
			return results;
	}

	for(auto& dir: directories) {
		dir.second->search(results, query, maxResults);

		if(results.size() >= maxResults) { return results; }
	}

	return results;
}

SearchResultList ShareManager::search(const StringList& adcParams, size_t maxResults) noexcept {
	return search(SearchQuery(adcParams), maxResults);
}

SearchResultList ShareManager::search(const string& nmdcString, int searchType, int64_t size, int fileType, size_t maxResults) noexcept {
	return search(SearchQuery(nmdcString, searchType, size, fileType), maxResults);
}

ShareManager::Directory::Ptr ShareManager::getDirectory(const string& realPath) noexcept {
	for(auto& mi: shares) {
		if(strnicmp(realPath, mi.first, mi.first.length()) == 0) {
			auto di = directories.find(mi.second);
			if(di == directories.end()) {
				return nullptr;
			}
			auto d = di->second;

			string::size_type i;
			string::size_type j = mi.first.length();
			while((i = realPath.find(PATH_SEPARATOR, j)) != string::npos) {
				auto dirName = realPath.substr(j, i - j);

				auto& subDirs = d->directories;
				d.reset();
				for(auto& subDir: subDirs) {
					if(subDir.second->getRealName() == dirName) {
						d = subDir.second;
						break;
					}
				}
				if(!d) {
					return nullptr;
				}

				j = i + 1;
			}
			return d;
		}
	}
	return nullptr;
}

optional<const ShareManager::Directory::File&> ShareManager::getFile(const string& realPath, Directory::Ptr d) noexcept {
	if(!d) {
		d = getDirectory(realPath);
		if(!d) {
			return boost::none;
		}
	}

	auto i = d->findFile(Util::getFileName(realPath));
	if(i == d->files.end()) {
		/* should never happen, but let's fail gracefully (maybe a synchro issue with a dir being
		removed during hashing...)... */
		dcdebug("ShareManager::getFile: the file <%s> could not be found, strange!\n", realPath.c_str());
		return boost::none;
	}

	if(i->realPath && i->realPath == realPath) {
		/* lucky! the base file already had a real path set (should never happen - it's only for
		dupes). */
		dcdebug("ShareManager::getFile: wtf, a non-renamed file has realPath set: <%s>\n", realPath.c_str());
		return *i;
	}

	/* see if the files sorted right before this one have a real path we are looking for. this is
	the most common case for dupes: "x (1).ext" is sorted before "x.ext". */
	auto real = i;
	if(real != d->files.begin()) {
		--real;
		while(real != d->files.end() && real->realPath) {
			if(real->realPath == realPath) {
				return *real;
			}
			--real;
		}
	}

	/* couldn't find it before the base file? maybe it's sorted after; could happen with files with
	no ext: "x (1)" is sorted after "x". */
	real = i;
	++real;
	while(real != d->files.end() && real->realPath) {
		if(real->realPath == realPath) {
			return *real;
		}
		++real;
	}

	/* most common case: no duplicate; just return the base file. */
	return *i;
}

void ShareManager::on(QueueManagerListener::FileMoved, const string& realPath) noexcept {
	if(BOOLSETTING(ADD_FINISHED_INSTANTLY)) {
		auto size = File::getSize(realPath);
		if(size == -1) {
			// looks like the file isn't actually there...
			return;
		}

		Lock l(cs);
		// Check if the finished download dir is supposed to be shared
		auto dir = getDirectory(realPath);
		if(dir) {
			Directory::File f(Util::getFileName(realPath), size, dir,
				HashManager::getInstance()->getTTH(realPath, size, 0));
			f.validateName(Util::getFilePath(realPath));
			dir->files.insert(move(f));
		}
	}
}

void ShareManager::on(HashManagerListener::TTHDone, const string& realPath, const TTHValue& root) noexcept {
	Lock l(cs);
	auto f = getFile(realPath);
	if(f) {
		if(f->tth && root != f->tth)
			tthIndex.erase(*f->tth);
		const_cast<Directory::File&>(*f).tth = root;
		tthIndex[*f->tth] = &f.get();

		setDirty();
		forceXmlRefresh = true;
	}
}

void ShareManager::on(TimerManagerListener::Minute, uint64_t tick) noexcept {
	if(SETTING(AUTO_REFRESH_TIME) > 0) {
		if(lastFullUpdate + SETTING(AUTO_REFRESH_TIME) * 60 * 1000 <= tick) {
			refresh(true, true);
		}
	}
}

bool ShareManager::shareFolder(const string& path, bool thoroughCheck /* = false */) const {
	if(thoroughCheck)
	{
		// check if it's part of the share before checking if it's in the exclusions
		bool result = false;
		for(const auto& i: shares)
		{
			// is it a perfect match
			if((path.size() == i.first.size()) && (stricmp(path, i.first) == 0))
				return true;

			if(path.size() > i.first.size()) // this might be a subfolder of a shared folder
			{
				string temp = path.substr(0, i.first.size());
				// if the left-hand side matches and there is a \ in the remainder then it is a subfolder
				if((stricmp(temp, i.first) == 0) && (path.find(PATH_SEPARATOR, i.first.size()) != string::npos))
				{
					result = true;
					break;
				}
			}
		}

		if(!result)
			return false;
	}

	// check if it's an excluded folder or a sub folder of an excluded folder
	for(const auto& i: notShared)
	{		
		if(stricmp(path, i) == 0)
			return false;

		if(thoroughCheck)
		{
			if(path.size() > i.size())
			{
				string temp = path.substr(0, i.size());
				if((stricmp(temp, i) == 0))
					return false;
			}
		}
	}
	return true;
}

int64_t ShareManager::addExcludeFolder(const string &path) {
	HashManager::getInstance()->stopHashing(path);

	// make sure this is a sub folder of a shared folder
	bool result = false;
	for(const auto& i: shares) 
	{
		if(path.size() > i.first.size())
		{
			string temp = path.substr(0, i.first.size());
			if(stricmp(temp, i.first) == 0)
			{
				result = true;
				break;
			}
		}
	}

	if(!result)
		return 0;

	// Make sure this not a subfolder of an already excluded folder
	for(const auto& i: notShared)
	{
		if(path.size() >= i.size())
		{
			string temp = path.substr(0, i.size());
			if(stricmp(temp, i) == 0)
				return 0;
		}
	}

	// remove all sub folder excludes
	int64_t bytesNotCounted = 0;
	for(auto j = notShared.begin(); j != notShared.end(); ++j)
	{
		if(path.size() < (*j).size())
		{
			string temp = (*j).substr(0, path.size());
			if(stricmp(temp, path) == 0)
			{
				bytesNotCounted += Util::getDirSize(*j);
				j = notShared.erase(j);
				j--;
			}
		}
	}

	// add it to the list
	notShared.emplace_back(path);

	int64_t bytesRemoved = Util::getDirSize(path);

	return (bytesRemoved - bytesNotCounted);
}

int64_t ShareManager::removeExcludeFolder(const string &path, bool returnSize /* = true */) {
	int64_t bytesAdded = 0;
	// remove all sub folder excludes
	for(auto j = notShared.begin(); j != notShared.end(); ++j)
	{
		if(path.size() <= (*j).size())
		{
			string temp = (*j).substr(0, path.size());
			if(stricmp(temp, path) == 0)
			{
				if(returnSize) // this needs to be false if the files don't exist anymore
					bytesAdded += Util::getDirSize(*j);

				j = notShared.erase(j);
				j--;
			}
		}
	}

	return bytesAdded;
}

} // namespace dcpp
