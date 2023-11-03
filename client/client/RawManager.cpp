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

#include "stdinc.h"
#include "RawManager.h"

#include <utility>

#include "ResourceManager.h"
#include "version.h"
#include "SimpleXML.h"
#include "ClientManager.h"

namespace dcpp {

using std::swap;

void RawManager::saveActionRaws() {
	try {
		SimpleXML xml;
		xml.addTag("ActionRaws");
		xml.addChildAttrib("Version", string(VERSIONSTRING));
		xml.stepIn();

		for(Action::Iter l = action.begin(); l != action.end(); ++l) {
			xml.addTag("Action");
			xml.addChildAttrib("ID", Util::toString(l->second->getActionId()));
			xml.addChildAttrib("Name", l->second->getName());
			xml.addChildAttrib("Active", Util::toString(l->second->getActive()));
			xml.stepIn();
			for(Action::RawIter j = l->second->raw.begin(); j != l->second->raw.end(); ++j) {
				xml.addTag("Raw");
				xml.addChildAttrib("ID", Util::toString(j->getRawId()));
				xml.addChildAttrib("Name", j->getName());
				xml.addChildAttrib("Raw", j->getRaw());
				xml.addChildAttrib("Time", Util::toString(j->getTime()));
				xml.addChildAttrib("Active", Util::toString(j->getActive()));
			}
			xml.stepOut();
		}
		xml.stepOut();

		string fname = Util::getPath(Util::PATH_USER_CONFIG) + "Raws.xml";

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);

	} catch(const Exception& e) {
		dcdebug("RawManager::saveActionRaws: %s\n", e.getError().c_str());
	}
}

void RawManager::loadActionRaws(SimpleXML* aXml) {
	aXml->resetCurrentChild();

	while(aXml->findChild("Action")) {
		int actionId(aXml->getIntChildAttrib("ID"));
		string name(aXml->getChildAttrib("Name"));
		bool active(getVersion().empty() ? aXml->getBoolChildAttrib("Actif") : aXml->getBoolChildAttrib("Active"));
		addAction(actionId, name, active);
		aXml->stepIn();
		while(aXml->findChild("Raw")) {
			int rawId(aXml->getIntChildAttrib("ID"));
			string nameRaw(aXml->getChildAttrib("Name"));
			string raw(aXml->getChildAttrib("Raw"));
			string time(aXml->getChildAttrib("Time"));
			bool activeRaw(getVersion().empty() ? aXml->getBoolChildAttrib("Actif") : aXml->getBoolChildAttrib("Active"));
			addRaw(actionId, rawId, nameRaw, raw, Util::toInt(time), activeRaw);
		}
		aXml->stepOut();
	}
}

void RawManager::loadActionRaws() {
	try {
		Util::migrate(Util::getPath(Util::PATH_USER_CONFIG) + "Raws.xml");

		SimpleXML xml;
		xml.fromXML(File(Util::getPath(Util::PATH_USER_CONFIG) + "Raws.xml", File::READ, File::OPEN).read());
		
		if(xml.findChild("ActionRaws")) {
			setVersion(xml.getChildAttrib("Version"));
			xml.stepIn();
			loadActionRaws(&xml);
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("RawManager::loadActionRaws: %s\n", e.getError().c_str());
	}
}

int RawManager::addAction(int actionId, const string& name, bool active) {
	if(name.empty())
		throw Exception(STRING(NO_NAME_SPECIFIED));

	Lock l(act);

	for(Action::Iter i = action.begin(); i != action.end(); ++i) {
		if(i->second->getName() == name)
			throw Exception(STRING(ACTION_EXISTS));
	}

	while(actionId == 0) {
		actionId = Util::rand(1, 2147483647);
		for(Action::Iter i = action.begin(); i != action.end(); ++i) {
			if(i->second->getActionId() == actionId) {
				actionId = 0;
			}
		}
	}
	int id = lastAction++;

	action.insert(make_pair(id, new Action(actionId, name, active)));

	return id;
}

void RawManager::renameAction(const string& oName, const string& nName) {
	if(oName.empty() || nName.empty())
		throw Exception(STRING(NO_NAME_SPECIFIED));

	Lock l(act);

	for(Action::Iter i = action.begin(); i != action.end(); ++i) {
		if(i->second->getName() == nName)
			throw Exception(STRING(ACTION_EXISTS));
	}
	for(Action::Iter i = action.begin(); i != action.end(); ++i) {
		if(i->second->getName() == oName)
			i->second->setName(nName);
	}
}

void RawManager::setActiveAction(int id, bool active) {
	Lock l(act);
	Action::Iter i;
	if((i = action.find(id)) != action.end()) {
		i->second->setActive(active);
	}
}

bool RawManager::getActiveActionId(int actionId)  {
	Lock l(act);
	for(Action::Iter i = action.begin(); i != action.end(); ++i) {
		if(i->second->getActionId() == actionId) {
			return i->second->getActive();
		}
	}
	return false;
}

void RawManager::removeAction(int id) {
	Lock l(act);
	Action::List::iterator i;
	if((i = action.find(id)) != action.end()) {
		delete i->second;
		action.erase(i);
	}
}

int RawManager::getValidAction(int actionId) {
	Lock l(act);
	for(Action::Iter i = action.begin(); i != action.end(); ++i) {
		if(i->second->getActionId() == actionId)
			return i->second->getActionId();
	}
	return 0;
}

int RawManager::getActionId(int id) {
	Lock l(act);
	Action::Iter i;
	if((i = action.find(id)) != action.end()) {
		return i->second->getActionId();
	}
	return 0;
}

tstring RawManager::getNameActionId(int actionId) {
	Lock l(act);
	for(Action::Iter i = action.begin(); i != action.end(); ++i) {
		if(i->second->getActionId() == actionId)
			return Text::toT(i->second->getName());
	}
	return TSTRING(NO_ACTION);
}

Action::RawList RawManager::getRawList(int id) {
	Lock l(act);

	Action::Iter i;
	Action::RawList lst;
	if((i = action.find(id)) != action.end()) {
		lst = i->second->raw;
	}
	return lst;
}

Action::RawList RawManager::getRawListActionId(int actionId) {
	Lock l(act);
	
	Action::RawList lst;
	for(Action::Iter i = action.begin(); i != action.end(); ++i) {
		if(i->second->getActionId() == actionId)
			lst = i->second->raw;
	}
	return lst;
}

void RawManager::addRaw(int actionId, int rawId, const string& name, const string& raw, int time, bool active) {
	if(name.empty())
		return;

	Lock l(act);

	for(Action::Iter i = action.begin(); i != action.end(); ++i) {
		if(i->second->getActionId() == actionId) {

			for(Action::RawIter j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
				if(j->getName() == name)
					return;
			}

			while(rawId == 0) {
				rawId = Util::rand(1, 2147483647);
				for(Action::RawIter j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
					while(j->getRawId() == rawId) {
						rawId = 0;
					}
				}
			}

			i->second->raw.push_back(
				Action::Raw(
				i->second->lastRaw++,
				rawId,
				name,
				raw,
				time,
				active
				)
			);
			return;
		}
	}
}

Action::Raw RawManager::addRaw(int id, const string& name, const string& raw, int time) {
	if(name.empty())
		throw Exception(STRING(NO_NAME_SPECIFIED));

	Lock l(act);

	Action::Iter i;
	if((i = action.find(id)) != action.end()) {
		for(Action::RawIter j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
			if(j->getName() == name)
				throw Exception(STRING(RAW_EXISTS));
		}

		int rawId = 0;
		while(rawId == 0) {
			rawId = Util::rand(1, 2147483647);
			for(Action::RawIter j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
				while(j->getRawId() == rawId) {
					rawId = 0;
				}
			}
		}

		i->second->raw.push_back(
			Action::Raw(
			i->second->lastRaw++,
			rawId,
			name,
			raw,
			time,
			true
			)
		);
		return i->second->raw.back();
	}
	return Action::Raw(0, 0, Util::emptyString, Util::emptyString, 0, false);
}

void RawManager::changeRaw(int id, const string& oName, const string& nName, const string& raw, int time) {
	if(oName.empty() || nName.empty())
		throw Exception(STRING(NO_NAME_SPECIFIED));

	Lock l(act);

	Action::Iter i;
	if((i = action.find(id)) != action.end()) {
		if(oName != nName) {
			for(Action::RawIter j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
				if(j->getName() == nName)
					throw Exception(STRING(RAW_EXISTS));
			}
		}

		for(Action::RawList::iterator j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
			if(j->getName() == oName) {
				j->setName(nName);
				j->setRaw(raw);
				j->setTime(time);
				break;
			}
		}
	}
}

void RawManager::getRawItem(int id, int idRaw, Action::Raw& raw, bool favHub/* = false*/) {
	Lock l(act);
	Action::Iter i;
	if((i = action.find(id)) != action.end()) {
		for(Action::RawIter j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
			if (favHub) {
				if(j->getRawId() == idRaw) {
					raw = *j;
					break;
				}
			} else {
				if(j->getId() == idRaw) {
					raw = *j;
					break;
				}
			}
		}
	}
}

void RawManager::setActiveRaw(int id, int idRaw, bool active) {
	Lock l(act);
	Action::Iter i;
	if((i = action.find(id)) != action.end()) {
		for(Action::RawList::iterator j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
			if(j->getId() == idRaw) {
				j->setActive(active);
				break;
			}
		}
	}
}

void RawManager::removeRaw(int id, int idRaw) {
	Lock l(act);
	Action::Iter i;
	if((i = action.find(id)) != action.end()) {
		for(Action::RawList::iterator j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
			if(j->getId() == idRaw) {
				i->second->raw.erase(j);
				break;
			}
		}
	}
}

bool RawManager::moveRaw(int id, int idRaw, int pos) {
	dcassert(pos == -1 || pos == 1);
	Lock l(act);
	Action::Iter i;
	if((i = action.find(id)) != action.end()) {
		for(Action::RawList::iterator j = i->second->raw.begin(); j != i->second->raw.end(); ++j) {
			if(j->getId() == idRaw) {
				swap(*j, *(j + pos));
				return true;
			}
		}
	}
	return false;
}

void RawManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	Lock l(act);
	for(IterRaw i = raw.begin(); i != raw.end(); ++i) {
		if(aTick >= i->first) {
			ClientManager::getInstance()->sendRawCommand(i->second.first, i->second.second);
			raw.erase(i);
		}
	}
}

}