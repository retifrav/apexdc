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

#ifndef DCPLUSPLUS_DCPP_HASH_MANAGER_H
#define DCPLUSPLUS_DCPP_HASH_MANAGER_H

#include <functional>
#include <map>

#include <boost/optional.hpp>

#include "Singleton.h"
#include "MerkleTree.h"
#include "Thread.h"
#include "CriticalSection.h"
#include "Semaphore.h"
#include "TimerManager.h"
#include "HashManagerListener.h"
#include "GetSet.h"

namespace dcpp {

using std::function;
using std::map;

using boost::optional;

STANDARD_EXCEPTION(HashException);

class HashLoader;

class HashManager : public Singleton<HashManager>, public Speaker<HashManagerListener>,
	private TimerManagerListener
{
public:

	/** We don't keep leaves for blocks smaller than this... */
	static const int64_t MIN_BLOCK_SIZE;

	HashManager() {
		TimerManager::getInstance()->addListener(this);
	}
	~HashManager() {
		TimerManager::getInstance()->removeListener(this);
		hasher.join();
	}

	/** Get the TTH root associated with the filename if its tree is current. */
	optional<TTHValue> getTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp) noexcept;

	void stopHashing(const string& baseDir) { hasher.stopHashing(baseDir); }
	void setPriority(Thread::Priority p) { hasher.setThreadPriority(p); }

	bool getTree(const TTHValue& root, TigerTree& tt);

	/** Return block size of the tree associated with root, or 0 if no such tree is in the store */
	int64_t getBlockSize(const TTHValue& root);

	void addTree(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tt) {
		hashDone(aFileName, aTimeStamp, tt, -1, -1);
	}
	void addTree(const TigerTree& tree) { Lock l(cs); store.addTree(tree); }

	void getStats(string& curFile, uint64_t& bytesLeft, size_t& filesLeft) const {
		hasher.getStats(curFile, bytesLeft, filesLeft);
	}

	/**
	 * Rebuild hash data file
	 */
	void rebuild() { hasher.scheduleRebuild(); }

	void startup(function<void (float)> progressF) { hasher.start(); store.load(progressF); }

	void shutdown() {
		hasher.shutdown();
		hasher.join();
		Lock l(cs);
		store.save();
	}

	struct HashPauser {
		HashPauser();
		~HashPauser();

	private:
		bool resume;
	};

	/// @return whether hashing was already paused
	bool pauseHashing() noexcept;
	void resumeHashing() noexcept;
	bool isHashingPaused() const noexcept;

private:
	class Hasher : public Thread {
	public:
		Hasher() : stop(false), running(false), paused(0), rebuild(false), currentSize(0) { }

		void hashFile(const string& fileName, int64_t size) noexcept;

		/// @return whether hashing was already paused
		bool pause() noexcept;
		void resume() noexcept;
		bool isPaused() const noexcept;

		void stopHashing(const string& baseDir);
		int run();
		void getStats(string& curFile, uint64_t& bytesLeft, size_t& filesLeft) const;
		void shutdown() { stop = true; if(paused) s.signal(); s.signal(); }
		void scheduleRebuild() { rebuild = true; if(paused) s.signal(); s.signal(); }

	private:
		// Case-sensitive (faster), it is rather unlikely that case changes, and if it does it's harmless.
		// map because it's sorted (to avoid random hash order that would create quite strange shares while hashing)
		map<string, int64_t> w;
		mutable CriticalSection cs;
		Semaphore s;

		bool stop;
		bool running;
		unsigned paused;
		bool rebuild;
		string currentFile;
		int64_t currentSize;

		void instantPause();
	};

	friend class Hasher;

	class HashStore {
	public:
		HashStore();
		void addFile(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, bool aUsed);

		void load(function<void (float)> progressF);
		void save();

		void rebuild();

		optional<TTHValue> getTTH(const string& aFileName, int64_t aSize, uint32_t aTimeStamp) noexcept;

		void addTree(const TigerTree& tt) noexcept;
		bool getTree(const TTHValue& root, TigerTree& tth);
		int64_t getBlockSize(const TTHValue& root) const;
		bool isDirty() { return dirty; }
	private:
		/** Root -> tree mapping info, we assume there's only one tree for each root (a collision would mean we've broken tiger...) */
		struct TreeInfo {
			TreeInfo() : size(0), index(0), blockSize(0) { }
			TreeInfo(int64_t aSize, int64_t aIndex, int64_t aBlockSize) : size(aSize), index(aIndex), blockSize(aBlockSize) { }

			GETSET(int64_t, size, Size);
			GETSET(int64_t, index, Index);
			GETSET(int64_t, blockSize, BlockSize);
		};

		/** File -> root mapping info */
		struct FileInfo {
		public:
			FileInfo(const string& aFileName, const TTHValue& aRoot, uint32_t aTimeStamp, bool aUsed) :
				fileName(aFileName), root(aRoot), timeStamp(aTimeStamp), used(aUsed) { }

			bool operator==(const string& name) { return name == fileName; }

			GETSET(string, fileName, FileName);
			GETSET(TTHValue, root, Root);
			GETSET(uint32_t, timeStamp, TimeStamp);
			GETSET(bool, used, Used);
		};

		friend class HashLoader;

		unordered_map<string, vector<FileInfo>> fileIndex;
		unordered_map<TTHValue, TreeInfo> treeIndex;
		unordered_map<string, vector<FileInfo>> legacyIndex;

		bool dirty;

		void createDataFile(const string& name);

		bool loadTree(File& dataFile, const TreeInfo& ti, const TTHValue& root, TigerTree& tt);
		int64_t saveTree(File& dataFile, const TigerTree& tt);

		static string getIndexFile();
		static string getDataFile();
	};

	friend class HashLoader;

	Hasher hasher;
	HashStore store;

	mutable CriticalSection cs;

	/** Single node tree where node = root, no storage in HashData.dat */
	static const int64_t SMALL_TREE = -1;

	void hashDone(const string& aFileName, uint32_t aTimeStamp, const TigerTree& tth, int64_t speed, int64_t size);

	void doRebuild() {
		Lock l(cs);
		store.rebuild();
	}

	void on(TimerManagerListener::Minute, uint64_t) noexcept {
		Lock l(cs);
		store.save();
	}
};

} // namespace dcpp

#endif // !defined(HASH_MANAGER_H)
