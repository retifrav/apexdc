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

#ifndef DCPLUSPLUS_DCPP_DIRECTORY_LISTING_H
#define DCPLUSPLUS_DCPP_DIRECTORY_LISTING_H

#include <set>

#include <boost/noncopyable.hpp>

#include "forward.h"
#include "noexcept.h"

#include "HintedUser.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "Util.h"
#include "QueueItem.h"
#include "UserInfoBase.h"

namespace dcpp {

using std::set;

class ListLoader;

class DirectoryListing : boost::noncopyable, public UserInfoBase
{
public:
	class Directory;

	class File : public FastAlloc<File>, boost::noncopyable {
	public:
		typedef File* Ptr;

		File(Directory* aDir, const string& aName, int64_t aSize, const TTHValue& aTTH) noexcept :
			boost::noncopyable(),
			name(aName), size(aSize), parent(aDir), tthRoot(aTTH), adls(false)
		{
		}

		File(const File& rhs, bool _adls = false) :
			boost::noncopyable(),
			name(rhs.name), size(rhs.size), parent(rhs.parent), tthRoot(rhs.tthRoot), adls(_adls)
		{
		}

		GETSET(TTHValue, tthRoot, TTH);
		GETSET(string, name, Name);
		GETSET(int64_t, size, Size);
		GETSET(Directory*, parent, Parent);
		GETSET(bool, adls, Adls);
		GETSET(int, adlsRaw, AdlsRaw);
		GETSET(string, adlsComment, AdlsComment);
		GETSET(int, adlsPriority, AdlsPriority);
	};

	class Directory : public FastAlloc<Directory>, boost::noncopyable {
	public:
		typedef Directory* Ptr;

		typedef unordered_set<TTHValue> TTHSet;

		template<typename T> struct Less {
			bool operator()(typename T::Ptr a, typename T::Ptr b) const { return compare(a->getName(), b->getName()) < 0; }
		};

		set<Ptr, Less<Directory>> directories;
		set<File::Ptr, Less<File>> files;
		
		Directory(Directory* aParent, const string& aName, bool _adls, bool aComplete) :
			boost::noncopyable(),
			name(aName), parent(aParent), adls(_adls), complete(aComplete) { }
		
		virtual ~Directory();

		size_t getTotalFileCount(bool adls = false);		
		int64_t getTotalSize(bool adls = false);
		void filterList(DirectoryListing& dirList);
		void filterList(TTHSet& l);
		void getHashList(TTHSet& l);
		
		size_t getFileCount() { return files.size(); }
		
		int64_t getSize() {
			int64_t x = 0;
			for(auto& i: files) {
				x += i->getSize();
			}
			return x;
		}
		
		GETSET(string, name, Name);
		GETSET(Directory*, parent, Parent);		
		GETSET(bool, adls, Adls);		
		GETSET(bool, complete, Complete);
	};

	class AdlDirectory : public Directory {
	public:
		AdlDirectory(const string& aFullPath, Directory* aParent, const string& aName) : Directory(aParent, aName, true, true), fullPath(aFullPath) { }

		GETSET(string, fullPath, FullPath);
	};

	DirectoryListing(const HintedUser& aUser);
	~DirectoryListing();
	
	decltype(Directory::files) getForbiddenFiles();

	void loadFile(const string& path);

	string updateXML(const std::string&);
	string loadXML(InputStream& xml, bool updating);

	void download(const string& aDir, const string& aTarget, QueueItem::Priority prio = QueueItem::DEFAULT);
	void download(Directory* aDir, const string& aTarget, QueueItem::Priority prio = QueueItem::DEFAULT);
	void download(File* aFile, const string& aTarget, bool view, QueueItem::Priority prio = QueueItem::DEFAULT);

	string getPath(const Directory* d) const;
	string getPath(const File* f) const { return getPath(f->getParent()); }

	int64_t getTotalSize(bool adls = false) { return root->getTotalSize(adls); }
	size_t getTotalFileCount(bool adls = false) { return root->getTotalFileCount(adls); }

	const Directory* getRoot() const { return root; }
	Directory* getRoot() { return root; }

	static UserPtr getUserFromFilename(const string& fileName);
	
	const UserPtr& getUser() const { return hintedUser.user; }	
		
	GETSET(HintedUser, hintedUser, HintedUser);
	GETSET(bool, abort, Abort);
	
private:
	friend class ListLoader;

	Directory* root;
	string base;
		
	Directory* find(const string& aName, Directory* current);
};

inline bool operator==(DirectoryListing::Directory::Ptr a, const string& b) { return stricmp(a->getName(), b) == 0; }
inline bool operator==(DirectoryListing::File::Ptr a, const string& b) { return stricmp(a->getName(), b) == 0; }

} // namespace dcpp

#endif // !defined(DIRECTORY_LISTING_H)

/**
 * @file
 * $Id$
 */
