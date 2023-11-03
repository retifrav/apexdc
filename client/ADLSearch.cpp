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

/*
 * Automatic Directory Listing Search
 * Henrik Engström, henrikengstrom at home se
 */

#include "stdinc.h"
#include "ADLSearch.h"
#include "QueueManager.h"
#include "ClientManager.h"

#include "File.h"
#include "RawManager.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"

namespace dcpp {

ADLSearch::ADLSearch() : 
	searchString("<Enter string>"), 
	isActive(true), 
	isAutoQueue(false), 
	sourceType(OnlyFile),
	minFileSize(-1), 
	maxFileSize(-1), 
	typeFileSize(SizeBytes), 
	destDir("ADLSearch"), 
	ddIndex(0),
	isForbidden(false), 
	raw(0),
	isCaseSensitive(true),
	adlsComment("none"), 
	isGlobal(false),
	adlsPriority(1)
{
}

ADLSearch::SourceType ADLSearch::StringToSourceType(const string& s) {
	if(stricmp(s.c_str(), "Filename") == 0) {
		return OnlyFile;
	} else if(stricmp(s.c_str(), "Directory") == 0) {
		return OnlyDirectory;
	} else if(stricmp(s.c_str(), "Full Path") == 0) {
		return FullPath;
	} else if(stricmp(s.c_str(), "TTH") == 0) {
		return TTHash;
	} else {
		return OnlyFile;
	}
}

string ADLSearch::SourceTypeToString(SourceType t) {
	switch(t) {
	default:
	case OnlyFile:		return "Filename";
	case OnlyDirectory:	return "Directory";
	case FullPath:		return "Full Path";
	case TTHash:		return "TTH";
	}
}

tstring ADLSearch::SourceTypeToDisplayString(SourceType t) {
	switch(t) {
	default:
	case OnlyFile:		return TSTRING(FILENAME);
	case OnlyDirectory:	return TSTRING(DIRECTORY);
	case FullPath:		return TSTRING(ADLS_FULL_PATH);
	case TTHash:		return Text::toT("TTH");
	}
}

ADLSearch::SizeType ADLSearch::StringToSizeType(const string& s) {
	if(stricmp(s.c_str(), "B") == 0) {
		return SizeBytes;
	} else if(stricmp(s.c_str(), "kB") == 0) {
		return SizeKiloBytes;
	} else if(stricmp(s.c_str(), "MB") == 0) {
		return SizeMegaBytes;
	} else if(stricmp(s.c_str(), "GB") == 0) {
		return SizeGigaBytes;
	} else {
		return SizeBytes;
	}
}

string ADLSearch::SizeTypeToString(SizeType t) {
	switch(t) {
	default:
	case SizeBytes:		return "B";
	case SizeKiloBytes:	return "kB";
	case SizeMegaBytes:	return "MB";
	case SizeGigaBytes:	return "GB";
	}
}

tstring ADLSearch::SizeTypeToDisplayString(ADLSearch::SizeType t) {
	switch(t) {
	default:
	case SizeBytes:		return CTSTRING(B);
	case SizeKiloBytes:	return CTSTRING(KB);
	case SizeMegaBytes:	return CTSTRING(MB);
	case SizeGigaBytes:	return CTSTRING(GB);
	}
}

int64_t ADLSearch::GetSizeBase() {
	switch(typeFileSize) {
	default:
	case SizeBytes:		return (int64_t)1;
	case SizeKiloBytes:	return (int64_t)1024;
	case SizeMegaBytes:	return (int64_t)1024 * (int64_t)1024;
	case SizeGigaBytes:	return (int64_t)1024 * (int64_t)1024 * (int64_t)1024;
	}
}

bool ADLSearch::isRegEx() const {
	return boost::get<boost::regex>(&v) != NULL;
}

void ADLSearch::setRegEx(bool b) {
	if(b)
		v = boost::regex();
	else
		v = StringSearch::List();
}

struct Prepare : boost::static_visitor<>, boost::noncopyable {
	Prepare(const string& s_, bool bCase_) : s(s_), bCase(bCase_) { }

	void operator()(StringSearch::List& stringSearches) const {
		// Prepare quick search of substrings
		stringSearches.clear();

		// Split into substrings
		StringTokenizer<string> st(s, ' ');
		for(auto i = st.getTokens().begin(), iend = st.getTokens().end(); i != iend; ++i) {
			if(!i->empty()) {
				// Add substring search
				stringSearches.push_back(StringSearch(bCase ? *i : Text::toLower(*i)));
			}
		}
	}

	void operator()(boost::regex& r) const {
		try {
			r.assign(s, bCase ? 0 : boost::regex_constants::icase);
		} catch(const std::runtime_error&) {
			LogManager::getInstance()->message(str(dcpp_fmt("Invalid ADL Search regular expression: %1%") % s), LogManager::LOG_WARNING);
		}
	}

private:
	const string& s;
	bool bCase;
};

void ADLSearch::prepare(StringMap& params) {
	boost::apply_visitor(Prepare(Util::formatParams(searchString, params, false), isCaseSensitive), v);
}

bool ADLSearch::matchesFile(const string& f, const string& fp, const string& root, int64_t size, bool noAdlSearch) {
	// Check status
	if(!isActive || (noAdlSearch && !isGlobal)) {
		return false;
	}

	if(sourceType == TTHash) {
		return root == searchString;
	}

	// Check size for files
	if(size >= 0 && (sourceType == OnlyFile || sourceType == FullPath)) {
		if(minFileSize >= 0 && size < minFileSize * GetSizeBase()) {
			// Too small
			return false;
		}
		if(maxFileSize >= 0 && size > maxFileSize * GetSizeBase()) {
			// Too large
			return false;
		}
	}

	// Do search
	switch(sourceType) {
	default:
	case OnlyDirectory:	return false;
	case OnlyFile:		return searchAll(f);
	case FullPath:		return searchAll(fp);
	}
}

bool ADLSearch::matchesDirectory(const string& d, bool noAdlSearch) {
	// Check status
	if(!isActive || (noAdlSearch && !isGlobal)) {
		return false;
	}
	if(sourceType != OnlyDirectory) {
		return false;
	}

	// Do search
	return searchAll(d);
}

struct SearchAll : boost::static_visitor<bool>, boost::noncopyable {
	SearchAll(const string& s_, bool bCase_) : s(s_), bCase(bCase_) { }

	bool operator()(StringSearch::List& stringSearches) const {
		// Match all substrings
		string str = bCase ? s : Text::toLower(s);
		for(auto i = stringSearches.begin(), iend = stringSearches.end(); i != iend; ++i) {
			if(!i->match(str)) {
				return false;
			}
		}
		return !stringSearches.empty();
	}

	bool operator()(boost::regex& r) const {
		try {
			return !r.empty() && boost::regex_search(s, r);
		} catch(const std::runtime_error&) {
			// most likely a stack overflow, ignore...
			return false;
		}
	}

private:
	const string& s;
	bool bCase;
};

bool ADLSearch::searchAll(const string& s) {
	return boost::apply_visitor(SearchAll(s, isCaseSensitive), v);
}

ADLSearchManager::ADLSearchManager() : user(UserPtr(), Util::emptyString) { 
	load(); 
}

ADLSearchManager::~ADLSearchManager() { 
	save(); 
}

void ADLSearchManager::load() {
	// Clear current
	collection.clear();

	// Settings migration
	Util::migrate(getConfigFile());

	// If there is nothing to load (new install) insert our default example
	if(!Util::fileExists(getConfigFile())) {
		ADLSearch search;
		search.searchString = "^(?=.*?\\b(pre.?teen|incest|lolita|r@ygold|rape|raped|underage|kiddy)\\b.*?\\b(sex|fuck|porn|xxx)\\b).+\\.(avi|mpg|mpeg|mov|jpg|bmp|gif|asf|wmv|dctmp|asf)$";
		search.sourceType = search.StringToSourceType("Filename");
		search.destDir = STRING(ADLS_DISCARD);
		search.adlsComment = "Filters illegal porn, this is an *example* only!";
		search.isActive = true;
		search.isGlobal = true;
		search.isCaseSensitive = false;
		search.setRegEx(true);
		collection.push_back(search);
	}


	// Load file as a string
	try {
		SimpleXML xml;
		xml.fromXML(File(getConfigFile(), File::READ, File::OPEN).read());

		if(xml.findChild("ADLSearch")) {
			xml.stepIn();

			// Predicted several groups of searches to be differentiated
			// in multiple categories. Not implemented yet.
			if(xml.findChild("SearchGroup")) {
				xml.stepIn();

				// Loop until no more searches found
				while(xml.findChild("Search")) {
					xml.stepIn();

					// Found another search, load it
					ADLSearch search;

					if(xml.findChild("SearchString")) {
						search.searchString = xml.getChildData();
						if(xml.getBoolChildAttrib("RegEx"))
							search.setRegEx(true);
					}
					if(xml.findChild("SourceType")) {
						search.sourceType = search.StringToSourceType(xml.getChildData());
					}
					if(xml.findChild("DestDirectory")) {
						search.destDir = xml.getChildData();
					}
					if(xml.findChild("AdlsComment")) {
						search.adlsComment = xml.getChildData();
					}
					if(xml.findChild("IsActive")) {
						search.isActive = (Util::toInt(xml.getChildData()) != 0);
					}
					if(xml.findChild("IsForbidden")) {
						search.isForbidden = (Util::toInt(xml.getChildData()) != 0);
					}
					if(xml.findChild("IsRegExp")) {
						search.setRegEx(Util::toInt(xml.getChildData()) != 0);
					} else xml.resetCurrentChild();
					if(xml.findChild("IsCaseSensitive")) {
						search.isCaseSensitive = (Util::toInt(xml.getChildData()) != 0);
					}
					if(xml.findChild("IsGlobal")) {
						search.isGlobal = (Util::toInt(xml.getChildData()) != 0);
					}
					if(xml.findChild("AdlsPriority")) {
						search.adlsPriority = Util::toInt(xml.getChildData());
					}
					if(xml.findChild("Raw")) {
						search.raw = RawManager::getInstance()->getValidAction(Util::toInt(xml.getChildData()));
					}
					if(xml.findChild("MaxSize")) {
						search.maxFileSize = Util::toInt64(xml.getChildData());
					}
					if(xml.findChild("MinSize")) {
						search.minFileSize = Util::toInt64(xml.getChildData());
					}
					if(xml.findChild("SizeType")) {
						search.typeFileSize = search.StringToSizeType(xml.getChildData());
					}
					if(xml.findChild("IsAutoQueue")) {
						search.isAutoQueue = (Util::toInt(xml.getChildData()) != 0);
					}

					// Add search to collection
					if(search.searchString.size() > 0) {
						collection.push_back(search);
					}

					// Go to next search
					xml.stepOut();
				}
			}
		}
	} 
	catch(const SimpleXMLException&) { } 
	catch(const FileException&) { }
}

void ADLSearchManager::save() {
	// Prepare xml string for saving
	try {
		SimpleXML xml;

		xml.addTag("ADLSearch");
		xml.stepIn();

		// Predicted several groups of searches to be differentiated
		// in multiple categories. Not implemented yet.
		xml.addTag("SearchGroup");
		xml.stepIn();

		// Save all	searches
		for(SearchCollection::iterator i = collection.begin(); i != collection.end(); ++i) {
			ADLSearch& search = *i;
			if(search.searchString.empty()) {
				continue;
			}
			xml.addTag("Search");
			xml.stepIn();

			xml.addTag("SearchString", search.searchString);
			xml.addChildAttrib("RegEx", search.isRegEx());
			xml.addTag("SourceType", search.SourceTypeToString(search.sourceType));
			xml.addTag("DestDirectory", search.destDir);
			xml.addTag("AdlsComment", search.adlsComment);

			xml.addTag("IsActive", search.isActive);
			xml.addTag("IsForbidden", search.isForbidden);

			xml.addTag("IsCaseSensitive", search.isCaseSensitive);
			xml.addTag("IsGlobal", search.isGlobal);
			xml.addTag("AdlsPriority", search.adlsPriority);
			xml.addTag("Raw", RawManager::getInstance()->getValidAction(search.raw));

			xml.addTag("MaxSize", search.maxFileSize);
			xml.addTag("MinSize", search.minFileSize);
			xml.addTag("SizeType", search.SizeTypeToString(search.typeFileSize));
			xml.addTag("IsAutoQueue", search.isAutoQueue);
			xml.stepOut();
		}

		xml.stepOut();

		xml.stepOut();

		// Save string to file			
		try {
			File fout(getConfigFile(), File::WRITE, File::CREATE | File::TRUNCATE);
			fout.write(SimpleXML::utf8Header);
			fout.write(xml.toXML());
			fout.close();
		} catch(const FileException&) {	}
	} catch(const SimpleXMLException&) { }
}

void ADLSearchManager::matchesFile(DestDirList& destDirVector, DirectoryListing::File *currentFile, string& fullPath) {
	// Add to any substructure being stored
	for(DestDirList::iterator id = destDirVector.begin(); id != destDirVector.end(); ++id) {
		if(id->subdir != NULL) {
			DirectoryListing::File *copyFile = new DirectoryListing::File(*currentFile, true);
			dcassert(id->subdir->getAdls());

			id->subdir->files.insert(copyFile);
		}
		id->fileAdded = false;	// Prepare for next stage
	}

	// Prepare to match searches
	if(currentFile->getName().size() < 1) {
		return;
	}

	string filePath = fullPath + "\\" + currentFile->getName();
	// Match searches
	for(SearchCollection::iterator is = collection.begin(); is != collection.end(); ++is) {
		if(destDirVector[is->ddIndex].fileAdded) {
			continue;
		}
		if(is->matchesFile(currentFile->getName(), filePath, currentFile->getTTH().toBase32(), currentFile->getSize(), noAdlSearch)) {
			DirectoryListing::File *copyFile = new DirectoryListing::File(*currentFile, true);
			if(is->isForbidden) {
				copyFile->setAdlsRaw(is->raw);
				copyFile->setAdlsComment(is->adlsComment);
				copyFile->setAdlsPriority(is->adlsPriority);
			}
			destDirVector[is->ddIndex].dir->files.insert(copyFile);
			destDirVector[is->ddIndex].fileAdded = true;

			if(is->isAutoQueue && !is->isForbidden) {
				try {
					QueueManager::getInstance()->add(FavoriteManager::getInstance()->getDownloadDirectory(Util::getFileExt(currentFile->getName())) + currentFile->getName(),
						currentFile->getSize(), currentFile->getTTH(), getUser());
				} catch(const Exception&) {	}
			}

			if(breakOnFirst) {
				// Found a match, search no more
				break;
			}
		}
	}
}

void ADLSearchManager::matchesDirectory(DestDirList& destDirVector, DirectoryListing::Directory* currentDir, string& fullPath) {
	// Add to any substructure being stored
	for(DestDirList::iterator id = destDirVector.begin(); id != destDirVector.end(); ++id) {
		if(id->subdir != NULL) {
			DirectoryListing::Directory* newDir = 
				new DirectoryListing::AdlDirectory(fullPath, id->subdir, currentDir->getName());
			id->subdir->directories.insert(newDir);
			id->subdir = newDir;
		}
	}

	// Prepare to match searches
	if(currentDir->getName().size() < 1) {
		return;
	}

	// Match searches
	for(SearchCollection::iterator is = collection.begin(); is != collection.end(); ++is) {
		if(destDirVector[is->ddIndex].subdir != NULL) {
			continue;
		}
		if(is->matchesDirectory(currentDir->getName(), noAdlSearch)) {
			destDirVector[is->ddIndex].subdir = 
				new DirectoryListing::AdlDirectory(fullPath, destDirVector[is->ddIndex].dir, currentDir->getName());
			destDirVector[is->ddIndex].dir->directories.insert(destDirVector[is->ddIndex].subdir);
			if(breakOnFirst) {
				// Found a match, search no more
				break;
			}
		}
	}
}

void ADLSearchManager::stepUpDirectory(DestDirList& destDirVector) {
	for(DestDirList::iterator id = destDirVector.begin(); id != destDirVector.end(); ++id) {
		if(id->subdir != NULL) {
			id->subdir = id->subdir->getParent();
			if(id->subdir == id->dir) {
				id->subdir = NULL;
			}
		}
	}
}

void ADLSearchManager::prepareDestinationDirectories(DestDirList& destDirVector, DirectoryListing::Directory* root, StringMap& params) {
	// Load default destination directory (index = 0)
	destDirVector.clear();
	vector<DestDir>::iterator id = destDirVector.insert(destDirVector.end(), DestDir());
	id->name = "ADLSearch";
	id->dir  = new DirectoryListing::Directory(root, "<<<" + id->name + ">>>", true, true);

	// Scan all loaded searches
	for(SearchCollection::iterator is = collection.begin(); is != collection.end(); ++is) {
		// Check empty destination directory
		if(is->destDir.size() == 0) {
			// Set to default
			is->ddIndex = 0;
			continue;
		}

		// Check if exists
		bool isNew = true;
		long ddIndex = 0;
		for(id = destDirVector.begin(); id != destDirVector.end(); ++id, ++ddIndex) {
			if(stricmp(is->destDir.c_str(), id->name.c_str()) == 0) {
				// Already exists, reuse index
				is->ddIndex = ddIndex;
				isNew = false;
				break;
			}
		}

		if(isNew) {
			// Add new destination directory
			id = destDirVector.insert(destDirVector.end(), DestDir());
			id->name = is->destDir;
			id->dir  = new DirectoryListing::Directory(root, "<<<" + id->name + ">>>", true, true);
			is->ddIndex = ddIndex;
		}
	}
	// Prepare all searches
	for(SearchCollection::iterator ip = collection.begin(); ip != collection.end(); ++ip) {
		ip->prepare(params);
	}
}

void ADLSearchManager::finalizeDestinationDirectories(DestDirList& destDirVector, DirectoryListing::Directory* root) {
	string szDiscard("<<<" + STRING(ADLS_DISCARD) + ">>>");

	// Add non-empty destination directories to the top level
	for(vector<DestDir>::iterator id = destDirVector.begin(); id != destDirVector.end(); ++id) {
		if(id->dir->files.size() == 0 && id->dir->directories.size() == 0) {
			delete (id->dir);
		} else if(stricmp(id->dir->getName(), szDiscard) == 0) {
			delete (id->dir);
		} else {
			root->directories.insert(id->dir);
		}
	}		
}

void ADLSearchManager::matchListing(DirectoryListing& aDirList) {
	StringMap params;
	params["userNI"] = ClientManager::getInstance()->getNicks(aDirList.getHintedUser())[0];
	params["userCID"] = aDirList.getUser()->getCID().toBase32();

	setUser(aDirList.getHintedUser());

	auto root = aDirList.getRoot();

	DestDirList destDirs;
	prepareDestinationDirectories(destDirs, aDirList.getRoot(), params);
	setBreakOnFirst(BOOLSETTING(ADLS_BREAK_ON_FIRST));

	const FavoriteHubEntry* fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Util::toString(ClientManager::getInstance()->getHubs(aDirList.getHintedUser().user->getCID(), aDirList.getHintedUser().hint)));
	setNoAdlSearch(fhe ? fhe->getNoAdlSearch() : false);

	string path(aDirList.getRoot()->getName());
	matchRecurse(destDirs, aDirList, root, path);

	finalizeDestinationDirectories(destDirs, root);
}

void ADLSearchManager::matchRecurse(DestDirList &aDestList, DirectoryListing& filelist, DirectoryListing::Directory* aDir, string &aPath) {
	for(auto& dirIt: aDir->directories) {
		if(filelist.getAbort()) { throw Exception(); }
		string tmpPath = aPath + "\\" + dirIt->getName();
		matchesDirectory(aDestList, dirIt, tmpPath);
		matchRecurse(aDestList, filelist, dirIt, tmpPath);
	}
	for(auto& fileIt: aDir->files) {
		if(filelist.getAbort()) { throw Exception(); }
		matchesFile(aDestList, fileIt, aPath);
	}
	stepUpDirectory(aDestList);
}

string ADLSearchManager::getConfigFile() {
	 return Util::getPath(Util::PATH_USER_CONFIG) + "ADLSearch.xml";
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
