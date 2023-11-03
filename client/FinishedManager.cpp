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
#include "FinishedManager.h"

#include "ClientManager.h"
#include "FinishedManagerListener.h"
#include "Download.h"
#include "Upload.h"
#include "QueueManager.h"
#include "UploadManager.h"

#include "LogManager.h"
#include "ResourceManager.h"
#include "SimpleXML.h"

#define MAX_HISTORY_ITEMS 20

namespace dcpp {

FinishedManager::FinishedManager() { 
	QueueManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
}
	
FinishedManager::~FinishedManager() {
	QueueManager::getInstance()->removeListener(this);
	UploadManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);

	Lock l(cs);
	for_each(downloads.begin(), downloads.end(), DeleteFunction());
	for_each(uploads.begin(), uploads.end(), DeleteFunction());
}

void FinishedManager::remove(FinishedItemPtr item, bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItem::List *listptr = upload ? &uploads : &downloads;
		FinishedItem::List::iterator it = find(listptr->begin(), listptr->end(), item);

		if(it != listptr->end())
			listptr->erase(it);
		else
			return;
	}
}
	
void FinishedManager::removeAll(bool upload /* = false */) {
	{
		Lock l(cs);
		FinishedItem::List *listptr = upload ? &uploads : &downloads;
		for_each(listptr->begin(), listptr->end(), DeleteFunction());
		listptr->clear();
	}
}

void FinishedManager::on(QueueManagerListener::Finished, const QueueItem* qi, const string&, const Download* d) noexcept
{
	bool isFile = !qi->isSet(QueueItem::FLAG_USER_LIST);
	if(isFile || (qi->isSet(QueueItem::FLAG_USER_LIST) && BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
		FinishedItemPtr item = new FinishedItem(qi->getTarget(), d->getHintedUser(), qi->getSize(), static_cast<int64_t>(d->getAverageSpeed()), GET_TIME(), qi->getTTH().toBase32());
		{
			Lock l(cs);
			downloads.push_back(item);
		}

		fire(FinishedManagerListener::AddedDl(), item);

		LogManager::getInstance()->message(STRING_F(FINISHED_DOWNLOAD, Util::getFileName(qi->getTarget()).c_str() % Util::toString(ClientManager::getInstance()->getNicks(d->getHintedUser())).c_str()), LogManager::LOG_INFO);
	}
}

void FinishedManager::on(UploadManagerListener::Complete, const Upload* u) noexcept
{
	if(u->getType() == Transfer::TYPE_FILE || (u->getType() == Transfer::TYPE_FULL_LIST && BOOLSETTING(LOG_FILELIST_TRANSFERS))) {
		FinishedItemPtr item = new FinishedItem(u->getPath(), u->getHintedUser(),	u->getSize(), static_cast<int64_t>(u->getAverageSpeed()), GET_TIME());
		{
			Lock l(cs);
			uploads.push_back(item);
		}

		fire(FinishedManagerListener::AddedUl(), item);

		LogManager::getInstance()->message(STRING_F(FINISHED_UPLOAD, Util::getFileName(u->getPath()).c_str() % Util::toString(ClientManager::getInstance()->getNicks(u->getHintedUser())).c_str()), LogManager::LOG_INFO);
	}
}

void FinishedManager::on(SettingsManagerListener::Load, SimpleXML& /*aXml*/) noexcept {
	try {
		Util::migrate(Util::getPath(Util::PATH_USER_CONFIG) + "FinishedTransfers.xml");

		SimpleXML xml;
		xml.fromXML(File(Util::getPath(Util::PATH_USER_CONFIG) + "FinishedTransfers.xml", File::READ, File::OPEN).read());

		xml.resetCurrentChild();
		xml.stepIn();

		if(xml.findChild("Downloads")) {
			xml.stepIn();

			while(xml.findChild("Item")) {
				if(xml.getChildAttrib("CID").length() != 39) continue;
				UserPtr user = ClientManager::getInstance()->getUser(CID(xml.getChildAttrib("CID")));
				if(xml.getBoolChildAttrib("IsNmdc")) user->setFlag(User::NMDC);
				ClientManager::getInstance()->updateNick(user, xml.getChildAttrib("User"));

				FinishedItemPtr item = new FinishedItem(xml.getChildAttrib("Target"), HintedUser(user, xml.getChildAttrib("Hub")),
					xml.getIntChildAttrib("Size"), xml.getIntChildAttrib("AvgSpeed"), xml.getIntChildAttrib("Time"));

				if(Util::fileExists(item->getTarget())) {
					Lock l(cs);
					downloads.push_back(item);
				} else delete item;
			}
			xml.resetCurrentChild();
			xml.stepOut();
		}

		if(xml.findChild("Uploads")) {
			xml.stepIn();

			while(xml.findChild("Item")) {
				if(xml.getChildAttrib("CID").length() != 39) continue;
				UserPtr user = ClientManager::getInstance()->getUser(CID(xml.getChildAttrib("CID")));
				if(xml.getBoolChildAttrib("IsNmdc")) user->setFlag(User::NMDC);
				ClientManager::getInstance()->updateNick(user, xml.getChildAttrib("User"));

				FinishedItemPtr item = new FinishedItem(xml.getChildAttrib("Target"), HintedUser(user, xml.getChildAttrib("Hub")),
					xml.getIntChildAttrib("Size"), xml.getIntChildAttrib("AvgSpeed"), xml.getIntChildAttrib("Time"));

				if(Util::fileExists(item->getTarget())) {
					Lock l(cs);
					uploads.push_back(item);
				} else delete item;
			}
			xml.resetCurrentChild();
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("FinishedManager::on(Load): %s\n", e.getError().c_str());
	}
}

void FinishedManager::on(SettingsManagerListener::Save, SimpleXML& /*aXml*/) noexcept {
	SimpleXML xml;
	xml.addTag("FinishedTransfers");
	xml.stepIn();

	Lock l(cs);

	xml.addTag("Downloads");
	xml.stepIn();

	for(auto i = downloads.cbegin() + (downloads.size() > MAX_HISTORY_ITEMS ? downloads.size() - MAX_HISTORY_ITEMS : 0); i != downloads.cend(); ++i) {
		if(Util::fileExists((*i)->getTarget()) && (*i)->getUser().user) {
			xml.addTag("Item");
			xml.addChildAttrib("Target", (*i)->getTarget());
			xml.addChildAttrib("User", ClientManager::getInstance()->getNicks((*i)->getUser())[0]);
			xml.addChildAttrib("CID", (*i)->getUser().user->getCID().toBase32());
			xml.addChildAttrib("IsNmdc", (*i)->getUser().user->isSet(User::NMDC));
			xml.addChildAttrib("Hub", (*i)->getUser().hint);
			xml.addChildAttrib("Size", (*i)->getSize());
			xml.addChildAttrib("AvgSpeed", (*i)->getAvgSpeed());
			xml.addChildAttrib("Time", (*i)->getTime());
		}
	}

	xml.stepOut();

	xml.addTag("Uploads");
	xml.stepIn();

	for(auto i = uploads.cbegin() + (uploads.size() > MAX_HISTORY_ITEMS ? uploads.size() - MAX_HISTORY_ITEMS : 0); i != uploads.cend(); ++i) {
		if(Util::fileExists((*i)->getTarget()) && (*i)->getUser().user) {
			xml.addTag("Item");
			xml.addChildAttrib("Target", (*i)->getTarget());
			xml.addChildAttrib("User", ClientManager::getInstance()->getNicks((*i)->getUser())[0]);
			xml.addChildAttrib("CID", (*i)->getUser().user->getCID().toBase32());
			xml.addChildAttrib("IsNmdc", (*i)->getUser().user->isSet(User::NMDC));
			xml.addChildAttrib("Hub", (*i)->getUser().hint);
			xml.addChildAttrib("Size", (*i)->getSize());
			xml.addChildAttrib("AvgSpeed", (*i)->getAvgSpeed());
			xml.addChildAttrib("Time", (*i)->getTime());
		}
	}

	xml.stepOut();
	xml.stepOut();

	try {
		string fname = Util::getPath(Util::PATH_USER_CONFIG) + "FinishedTransfers.xml";

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("FinishedManager::on(Save): %s\n", e.getError().c_str());
	}
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
