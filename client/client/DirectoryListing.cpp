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
#include "DirectoryListing.h"

#include "QueueManager.h"
#include "SearchManager.h"

#include "StringTokenizer.h"
#include "SimpleXML.h"
#include "FilteredFile.h"
#include "BZUtils.h"
#include "CryptoManager.h"
#include "ResourceManager.h"
#include "SimpleXMLReader.h"
#include "User.h"

namespace dcpp {

DirectoryListing::DirectoryListing(const HintedUser& aUser) : 
hintedUser(aUser),
abort(false),
root(new Directory(nullptr, Util::emptyString, false, false)) 
{
}

DirectoryListing::~DirectoryListing() {
	delete root;
}

UserPtr DirectoryListing::getUserFromFilename(const string& fileName) {
	// General file list name format: [username].[CID].[xml|xml.bz2]

	string name = Util::getFileName(fileName);

	// Strip off any extensions
	if(stricmp(name.c_str() + name.length() - 4, ".bz2") == 0) {
		name.erase(name.length() - 4);
	}

	if(stricmp(name.c_str() + name.length() - 4, ".xml") == 0) {
		name.erase(name.length() - 4);
	}

	// Find CID
	string::size_type i = name.rfind('.');
	if(i == string::npos) {
		return UserPtr();
	}

	size_t n = name.length() - (i + 1);
	// CID's always 39 chars long...
	if(n != 39)
		return UserPtr();

	CID cid(name.substr(i + 1));
	if(cid.isZero())
		return UserPtr();

	return ClientManager::getInstance()->getUser(cid);
}

void DirectoryListing::loadFile(const string& path) {
	string actualPath;
	if(dcpp::File::getSize(path + ".bz2") != -1) {
		actualPath = path + ".bz2";
	}

	// For now, we detect type by ending...
	auto ext = Util::getFileExt(actualPath.empty() ? path : actualPath);

	dcpp::File file(actualPath.empty() ? path : actualPath, dcpp::File::READ, dcpp::File::OPEN);

	if(stricmp(ext, ".bz2") == 0) {
		FilteredInputStream<UnBZFilter, false> f(&file);
		loadXML(f, false);
	} else if(stricmp(ext, ".xml") == 0) {
		loadXML(file, false);
	}
}

class ListLoader : public SimpleXMLReader::CallBack {
public:
	ListLoader(DirectoryListing* aList, DirectoryListing::Directory* root, bool aUpdating, const UserPtr& aUser) :
		list(aList),
		cur(root),
		base("/"),
		inListing(false),
		updating(aUpdating),
		user(aUser)
	{ 
	}

	~ListLoader() {
		// some clients forget the "Base" param...
		if(list->base.empty()) {
			list->base = base;
		}
	}

	void startTag(const string& name, StringPairList& attribs, bool simple);
	void endTag(const string& name);

	const string& getBase() const { return base; }

private:
	DirectoryListing* list;
	DirectoryListing::Directory* cur;
	UserPtr user;

	StringMap params;
	string base;
	bool inListing;
	bool updating;
};

string DirectoryListing::updateXML(const string& xml) {
	MemoryInputStream mis(xml);
	return loadXML(mis, true);
}

string DirectoryListing::loadXML(InputStream& is, bool updating) {
	ListLoader ll(this, getRoot(), updating, getUser());

	SimpleXMLReader(&ll).parse(is, SETTING(MAX_FILELIST_SIZE) ? (size_t)SETTING(MAX_FILELIST_SIZE)*1024*1024 : 0);

	return ll.getBase();
}

static const string sFileListing = "FileListing";
static const string sBase = "Base";
static const string sGenerator = "Generator";
static const string sDirectory = "Directory";
static const string sIncomplete = "Incomplete";
static const string sFile = "File";
static const string sName = "Name";
static const string sSize = "Size";
static const string sTTH = "TTH";

void ListLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	if(list->getAbort()) { throw Exception(); }

	if(inListing) {
		if(name == sFile) {
			const string& n = getAttrib(attribs, sName, 0);
			if(n.empty())
				return;

			const string& s = getAttrib(attribs, sSize, 1);
			if(s.empty())
				return;
			auto size = Util::toInt64(s);

			const string& h = getAttrib(attribs, sTTH, 2);
			if(h.empty())
				return;
			TTHValue tth(h); /// @todo verify validity?

			auto f = new DirectoryListing::File(cur, n, size, tth);
			auto insert = cur->files.insert(f);

			if(!insert.second) {
				// the file was already there
				delete f;
				if(updating) {
					// partial file list
					f = *insert.first;
					f->setName(n); // the casing might have changed
					f->setSize(size);
					f->setTTH(tth);
				} else {
					// duplicates are forbidden in complete file lists
					throw Exception(_("Duplicate item in the file list"));
				}
			}

		} else if(name == sDirectory) {
			const string& n = getAttrib(attribs, sName, 0);
			if(n.empty()) {
				throw SimpleXMLException(_("Directory missing name attribute"));
			}

			bool incomp = getAttrib(attribs, sIncomplete, 1) == "1";

			auto d = new DirectoryListing::Directory(cur, n, false, !incomp);
			auto insert = cur->directories.insert(d);

			if(!insert.second) {
				// the dir was already there
				delete d;
				if(updating) {
					// partial file list
					d = *insert.first;
					if(!d->getComplete())
						d->setComplete(!incomp);
				} else {
					// duplicates are forbidden in complete file lists
					throw Exception(_("Duplicate item in the file list"));
				}
			}

			cur = d;

			if(simple) {
				// To handle <Directory Name="..." />
				endTag(name);
			}
		}
	} else if(name == sFileListing) {
		const string& b = getAttrib(attribs, sBase, 2);
		if(b.size() >= 1 && b[0] == '/' && b[b.size()-1] == '/') {
			base = b;
			if(list->base.empty() || base.size() < list->base.size()) {
				list->base = base;
			}
		}
		StringList sl = StringTokenizer<string>(base.substr(1), '/').getTokens();
		for(auto& i: sl) {
			DirectoryListing::Directory* d = nullptr;
			for(auto j: cur->directories) {
				if(j->getName() == i) {
					d = j;
					break;
				}
			}
			if(!d) {
				d = new DirectoryListing::Directory(cur, i, false, false);
				cur->directories.insert(d);
			}
			cur = d;
		}
		cur->setComplete(true);
		inListing = true;

		string generator = getAttrib(attribs, sGenerator, 2);
		ClientManager::getInstance()->setGenerator(user, generator);

		if(simple) {
			// To handle <Directory Name="..." />
			endTag(name);
		}
	}
}

void ListLoader::endTag(const string& name) {
	if(inListing) {
		if(name == sDirectory) {
			cur = cur->getParent();
		} else if(name == sFileListing) {
			// cur should be root now...
			inListing = false;
		}
	}
}

string DirectoryListing::getPath(const Directory* d) const {
	if(d == root)
		return "";

	string dir;
	dir.reserve(128);
	dir.append(d->getName());
	dir.append(1, '\\');

	Directory* cur = d->getParent();
	while(cur!=root) {
		dir.insert(0, cur->getName() + '\\');
		cur = cur->getParent();
	}
	return dir;
}

void DirectoryListing::download(Directory* aDir, const string& aTarget, QueueItem::Priority prio) {
	string target = (aDir == getRoot()) ? aTarget : aTarget + aDir->getName() + PATH_SEPARATOR;

	if(!aDir->getComplete()) {
		// Folder is not completed (partial list?), so we need to download it first
		QueueManager::getInstance()->addDirectory(getPath(aDir), hintedUser, aTarget, prio);
	} else {	
		// First, recurse over the directories
		for(auto& j: aDir->directories) {
			download(j, target, prio);
		}
		// Then add the files
		for(auto file: aDir->files) {
			try {
				download(file, target + file->getName(), false, prio);
			} catch(const QueueException&) {
				// Catch it here to allow parts of directories to be added...
			} catch(const FileException&) {
				//..
			}
		}
	}
}

void DirectoryListing::download(const string& aDir, const string& aTarget, QueueItem::Priority prio) {
	dcassert(aDir.size() > 2);
	dcassert(aDir[aDir.size() - 1] == '\\'); // This should not be PATH_SEPARATOR
	Directory* d = find(aDir, getRoot());
	if(d != NULL)
		download(d, aTarget, prio);
}

void DirectoryListing::download(File* aFile, const string& aTarget, bool view, QueueItem::Priority prio) {
	Flags::MaskType flags = (Flags::MaskType)(view ? (QueueItem::FLAG_TEXT | QueueItem::FLAG_CLIENT_VIEW) : 0);

	QueueManager::getInstance()->add(aTarget, aFile->getSize(), aFile->getTTH(), getHintedUser(), flags);

	if(prio != QueueItem::DEFAULT)
		QueueManager::getInstance()->setPriority(aTarget, prio);
}

DirectoryListing::Directory* DirectoryListing::find(const string& aName, Directory* current) {
	auto end = aName.find('\\');
	dcassert(end != string::npos);
	auto name = aName.substr(0, end);

	auto i = std::find(current->directories.begin(), current->directories.end(), name);
	if(i != current->directories.cend()) {
		if(end == (aName.size() - 1))
			return *i;
		else
			return find(aName.substr(end + 1), *i);
	}
	return nullptr;
}

DirectoryListing::Directory::~Directory() {
	std::for_each(directories.begin(), directories.end(), DeleteFunction());
	std::for_each(files.begin(), files.end(), DeleteFunction());
}

void DirectoryListing::Directory::filterList(DirectoryListing& dirList) {
		DirectoryListing::Directory* d = dirList.getRoot();

		TTHSet l;
		d->getHashList(l);
		filterList(l);
}

void DirectoryListing::Directory::filterList(DirectoryListing::Directory::TTHSet& l) {
	for(auto i = directories.begin(); i != directories.end();) {
		auto d = *i;

		d->filterList(l);

		if(d->directories.empty() && d->files.empty()) {
			i = directories.erase(i);
			delete d;
		} else {
			++i;
		}
	}

	for(auto i = files.begin(); i != files.end();) {
		auto f = *i;
		if(l.find(f->getTTH()) != l.end()) {
			i = files.erase(i);
			delete f;
		} else {
			++i;
		}
	}
}

void DirectoryListing::Directory::getHashList(DirectoryListing::Directory::TTHSet& l) {
	for(auto i: directories) i->getHashList(l);
	for(auto i: files) l.insert(i->getTTH());
}

int64_t DirectoryListing::Directory::getTotalSize(bool adl) {
	int64_t x = getSize();
	for(auto i: directories) {
		if(!(adl && i->getAdls()))
			x += i->getTotalSize(adls);
	}
	return x;
}

size_t DirectoryListing::Directory::getTotalFileCount(bool adl) {
	size_t x = getFileCount();
	for(auto i: directories) {
		if(!(adl && i->getAdls()))
			x += i->getTotalFileCount(adls);
	}
	return x;
}

decltype(DirectoryListing::Directory::files) DirectoryListing::getForbiddenFiles() {
	decltype(Directory::files) forbiddenList;
	for(auto i: root->directories) {
		if(i->getName().find(STRING(FORBIDDEN_FILES)) != string::npos) {			
			forbiddenList = i->files;
		}
	}
	return forbiddenList;
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
