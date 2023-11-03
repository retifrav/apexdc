/* 
 * Copyright (C) 2005-2005 Virus27, Virus27@free.fr
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

#if !defined(__RAW_MANAGER_H)
#define __RAW_MANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <utility>

#include "typedefs.h"
#include "Singleton.h"
#include "Util.h"
#include "Client.h"
#include "HintedUser.h"
#include "TimerManager.h"

namespace dcpp {

using std::pair;
using std::make_pair;

class Action {
	public:
	typedef Action* Ptr;
	typedef unordered_map<int, Ptr> List;
	typedef List::const_iterator Iter;

	Action() : lastRaw(0), actionId(0) { };

	Action(int aActionId, const string& aName, bool aActive) 
		noexcept : actionId(aActionId), name(aName), active(aActive) { };

	GETSET(int, actionId, ActionId);
	GETSET(string, name, Name);
	GETSET(bool, active, Active);

	struct Raw {
		Raw() : id(0), rawId(0), time(0) { };

		Raw(int aId, int aRawId, const string& aName, const string& aRaw, int aTime, bool aActive) 
		noexcept : id(aId), rawId(aRawId), name(aName), raw(aRaw), time(aTime), active(aActive) { };

		Raw(const Raw& rhs) : id(rhs.id), rawId(rhs.rawId), name(rhs.name), raw(rhs.raw), time(rhs.time), active(rhs.active) { }

		Raw& operator=(const Raw& rhs) { 
			id = rhs.id;
			rawId = rhs.rawId;
			name = rhs.name;
			raw = rhs.raw;
			time = rhs.time;
			active = rhs.active;
			return *this;
		}

		GETSET(int, id, Id);
		GETSET(int, rawId, RawId);
		GETSET(string, name, Name);
		GETSET(string, raw, Raw);
		GETSET(int, time, Time);
		GETSET(bool, active, Active);
	};

	typedef vector<Raw> RawList;
	typedef RawList::const_iterator RawIter;

	RawList raw;
	int lastRaw;
};

class SimpleXML;

class RawManager : public Singleton<RawManager>, public TimerManagerListener
{
public:
	Action::List& getActionList() {
		Lock l(act);
		return action;
	}

	int addAction(int actionId, const string& name, bool active);
	void renameAction(const string& oName, const string& nName);
	void setActiveAction(int id, bool active);
	bool getActiveActionId(int actionId);
	void removeAction(int id);
	int getValidAction(int actionId);
	int getActionId(int id);
	tstring getNameActionId(int actionId);

	Action::RawList getRawList(int id);
	Action::RawList getRawListActionId(int actionId);
	void addRaw(int idAction, int rawId, const string& name, const string& raw, int time, bool active);
	Action::Raw addRaw(int id, const string& name, const string& raw, int time);
	void changeRaw(int id, const string& oName, const string& nName, const string& raw, int time);
	void getRawItem(int id, int idRaw, Action::Raw& raw, bool favHub = false);
	void setActiveRaw(int id, int idRaw, bool active);
	void removeRaw(int id, int idRaw);
	bool moveRaw(int id, int idRaw, int pos);

	void loadActionRaws();
	void saveActionRaws();

	void addRaw(uint64_t time, const HintedUser& aUser, const string& aRaw) {
		Lock l(act);
		raw.insert(make_pair(time, make_pair(aUser, aRaw)));
	}

	GETSET(string, version, Version);
private:
	Action::List action;
	int lastAction;

	CriticalSection act;

	friend class Singleton<RawManager>;
	
	RawManager() : lastAction(0) {
		loadActionRaws();
		TimerManager::getInstance()->addListener(this);
	}

	~RawManager() {
		TimerManager::getInstance()->removeListener(this);

		for(Action::Iter i = action.begin(); i != action.end(); ++i) {
			delete i->second;
		}
	}

	void loadActionRaws(SimpleXML* aXml);

	typedef map<uint64_t, pair<HintedUser, string> > ListRaw;
	typedef ListRaw::iterator IterRaw;
	ListRaw raw;

	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t aTick) noexcept;
};

class RawSelector {
protected:
	typedef unordered_map<int, int> ActionList;
	typedef ActionList::const_iterator Iter;

	void createList() {
		Action::List lst = RawManager::getInstance()->getActionList();

		int j = 0;
		idAction.insert(make_pair(j, j));
		for(Action::Iter i = lst.begin(); i != lst.end(); ++i) {
			idAction.insert(make_pair(++j, i->second->getActionId()));
		}
	}

	int getId(int actionId) {
		for(Iter i = idAction.begin(); i != idAction.end(); ++i) {
			if(i->second == actionId)
				return i->first;
		}
		return 0;
	}

	int getIdAction(int id) {
		Iter i;
		if((i = idAction.find(id)) != idAction.end()) {
			return i->second;
		}
		return 0;
	}

	ActionList idAction;
};

}

#endif // !defined(__RAW_MANAGER_H)
