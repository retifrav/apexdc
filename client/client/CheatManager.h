/**
 * This file is a part of client manager.
 * It has been divided but shouldn't be used anywhere else.
 */

void sendRawCommand(const OnlineUser& ou, const int aRawCommand) {
	if(ou.getClientBase().isOp() && RawManager::getInstance()->getActiveActionId(aRawCommand)) {
		FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(ou.getClientBase().getHubUrl());
		if(hub) {
			if(FavoriteManager::getInstance()->getActiveAction(hub, aRawCommand)) {
				Action::RawList lst = RawManager::getInstance()->getRawListActionId(aRawCommand);

				uint64_t time = GET_TICK();
				for(Action::RawIter i = lst.begin(); i != lst.end(); ++i) {
					if(i->getActive()) {
						if(FavoriteManager::getInstance()->getActiveRaw(hub, aRawCommand, i->getRawId())) {
							if(BOOLSETTING(DELAYED_RAW_SENDING)) {
								time += (i->getTime() * 1000) + 1;
								if(!i->getRaw().empty())
									RawManager::getInstance()->addRaw(time, HintedUser(ou.getUser(), ou.getClient().getHubUrl()), i->getRaw());
							} else {
								sendRawCommand(HintedUser(ou.getUser(), ou.getClient().getHubUrl()), i->getRaw());
							}
						}
					}
				}
			}
		}
	}
}

void sendRawCommand(const HintedUser& aUser, const string& aRawCommand) {
	if (!aRawCommand.empty()) {
		StringMap ucParams;

		UserCommand uc = UserCommand(0, 0, 0, 0, "", aRawCommand, "", "");
		userCommand(aUser, uc, ucParams, true);
	}
}

void setListLength(const UserPtr& p, const string& listLen) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i != onlineUsers.end()) {
		i->second->getIdentity().set("LL", listLen);
	}
}

void fileListDisconnected(const UserPtr& p) {
	string report = Util::emptyString;
	bool remove = false;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
		if(i != onlineUsers.end() && i->second->getClientBase().type != ClientBase::DHT) {
			OnlineUser& ou = *i->second;
	
			int fileListDisconnects = Util::toInt(ou.getIdentity().get("FD")) + 1;
			ou.getIdentity().set("FD", Util::toString(fileListDisconnects));

			if(SETTING(ACCEPTED_DISCONNECTS) == 0)
				return;

			if(fileListDisconnects == SETTING(ACCEPTED_DISCONNECTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClientBase(), "Disconnected file list " + Util::toString(fileListDisconnects) + " times", false);
				sendRawCommand(ou, SETTING(DISCONNECT_RAW));
				if(!ou.getIdentity().get("FQ").empty()) {
					remove = true;
					ou.getIdentity().set("FQ", Util::emptyString);
				}
			}
		}
	}
	if(remove) {
		QueueManager::getInstance()->removeUserCheck(HintedUser(p, c->getHubUrl()));
	}
	if(c && !report.empty()) {
		c->cheatMessage(report);
	}
}

void connectionTimeout(const UserPtr& p) {
	string report = Util::emptyString;
	bool remove = false;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
		if(i != onlineUsers.end() && i->second->getClientBase().type != ClientBase::DHT) {
			OnlineUser& ou = *i->second;
	
			int connectionTimeouts = Util::toInt(ou.getIdentity().get("TO")) + 1;
			ou.getIdentity().set("TO", Util::toString(connectionTimeouts));
	
			if(SETTING(ACCEPTED_TIMEOUTS) == 0)
				return;
	
			if(connectionTimeouts == SETTING(ACCEPTED_TIMEOUTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClientBase(), "Connection timeout " + Util::toString(connectionTimeouts) + " times", false);
				sendRawCommand(ou, SETTING(TIMEOUT_RAW));
				if(!ou.getIdentity().get("FQ").empty()) {
					remove = true;
					ou.getIdentity().set("FQ", Util::emptyString);
				}
			}
		}
	}
	if(remove) {
		QueueManager::getInstance()->removeUserCheck(HintedUser(p, c->getHubUrl()));
	}
	if(c && !report.empty()) {
		c->cheatMessage(report);
	}
}

void checkCheating(const UserPtr& p, DirectoryListing* dl) {
	string report = Util::emptyString;
	uint32_t flag = 0;
	OnlineUserPtr ou = NULL;
	{
		Lock l(cs);

		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
		if(i == onlineUsers.end() || i->second->getClientBase().type == ClientBase::DHT)
			return;

		ou = i->second;

		int64_t statedSize = ou->getIdentity().getBytesShared();
		int64_t realSize = dl->getTotalSize();
	
		double multiplier = ((100+(double)SETTING(PERCENT_FAKE_SHARE_TOLERATED))/100); 
		int64_t sizeTolerated = (int64_t)(realSize*multiplier);
		string detectString = Util::emptyString;
		string inflationString = Util::emptyString;
		ou->getIdentity().set("RS", Util::toString(realSize));
		bool isFakeSharing = false;
	
		if(statedSize > sizeTolerated) {
			isFakeSharing = true;
		}

		if(isFakeSharing) {
			ou->getIdentity().set("FC", Util::toString(Util::toInt(ou->getIdentity().get("FC")) | Identity::BAD_LIST));
			detectString += STRING(CHECK_MISMATCHED_SHARE_SIZE) + " ";
			if(realSize == 0) {
				detectString += STRING(CHECK_0BYTE_SHARE);
			} else {
				double qwe = (double)((double)statedSize / (double)realSize);
				inflationString = STRING_F(CHECK_INFLATED, Util::toString(qwe).c_str());
				detectString += inflationString;
			}
			detectString += STRING(CHECK_SHOW_REAL_SHARE);

			report = ou->getIdentity().setCheat(ou->getClientBase(), detectString, false);
			sendRawCommand(*ou.get(), SETTING(FAKESHARE_RAW));
		}
		ou->getIdentity().set("FC", Util::toString(Util::toInt(ou->getIdentity().get("FC")) | Identity::CHECKED));
		ou->getIdentity().set("SF", Util::toString(dl->getTotalFileCount(true)));
		ou->getIdentity().set("FT", Util::toString(GET_TIME()));
		ou->getIdentity().set("FQ", Util::emptyString);

		// ADLSearch stuff, note: if user is fake sharing we skip this
		const auto forbiddenList = dl->getForbiddenFiles();
		if(!isFakeSharing && forbiddenList.size() > 0) {
			int priority = 0, raw = 0;
			int64_t totalsize = 0, size = 0;
			string comment, name, fullname, tth;

			for(auto i: forbiddenList) {
				if(i->getAdlsPriority() >= priority) {
					priority = i->getAdlsPriority();
					raw = i->getAdlsRaw();
					comment = i->getAdlsComment();
					name = i->getName();
					fullname = dl->getPath(i->getParent()) + i->getName();
					size = i->getSize();
					tth = i->getTTH().toBase32();
				}
				totalsize += i->getSize();
			}

			ou->getIdentity().set("AC", comment);
			ou->getIdentity().set("AI", name);
			ou->getIdentity().set("AP", fullname);
			ou->getIdentity().set("AS", Util::toString(size));
			ou->getIdentity().set("AF", Util::toString(totalsize));
			ou->getIdentity().set("AT", tth);
			ou->getIdentity().set("AL", Util::toString((int)forbiddenList.size()));

			report = ou->getIdentity().setCheat(ou->getClient(), CSTRING(CHECK_FORBIDDEN), false);
			if(raw != -1)
				sendRawCommand(*ou.get(), raw);
		}
		if(report.empty())
			report = ou->getIdentity().updateClientType(*ou, flag);
		else
			ou->getIdentity().updateClientType(*ou, flag);
	}
	ou->getClient().updated(ou);
	if(!report.empty())
		ou->getClient().cheatMessage(report, flag);
}

void setClientStatus(const UserPtr& p, const string& aCheatString, const int aRawCommand, bool aBadClient) {
	OnlineUserPtr ou = NULL;
	uint32_t flag = 0;
	string report = Util::emptyString;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
		if(i == onlineUsers.end() || i->second->getClientBase().type == ClientBase::DHT) 
			return;
		
		ou = i->second;
		report = ou->getIdentity().updateClientType(*ou, flag);

		if(!aCheatString.empty()) {
			report = ou->getIdentity().setCheat(ou->getClientBase(), aCheatString, aBadClient);
		}
		if(aRawCommand != -1)
			sendRawCommand(*ou.get(), aRawCommand);
	}
	ou->getClient().updated(ou);
	if(!report.empty())
		ou->getClient().cheatMessage(report, flag);
}

void setListSize(const UserPtr& p, int64_t aFileLength) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	i->second->getIdentity().set("LS", Util::toString(aFileLength));
}

void toggleChecks(bool aState) {
	Lock l(cs);

	for(auto i = clients.cbegin(); i != clients.cend(); ++i) {
		 if(!i->second->getExclChecks() && i->second->isOp()) {
			if(aState) {
				i->second->startChecking();
			} else {
				i->second->stopChecking();
			}
		}
	}
}

void setPkLock(const UserPtr& p, const string& aPk, const string& aLock) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	
	i->second->getIdentity().set("PK", aPk);
	i->second->getIdentity().set("LO", aLock);
}

void setSupports(const UserPtr& p, const string& aSupports) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	
	i->second->getIdentity().set("SU", aSupports);
}

void setGenerator(const UserPtr& p, const string& aGenerator) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	i->second->getIdentity().set("GE", aGenerator);
}

void setUnknownCommand(const UserPtr& p, const string& aUnknownCommand) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	i->second->getIdentity().set("UC", aUnknownCommand);
}

void reportUser(const HintedUser& user) {
	bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);
	string nick; string report;

	{
		Lock l(cs);
		OnlineUser* ou = findOnlineUser(user.user->getCID(), user.hint, priv);
		if(!ou || ou->getClientBase().type == ClientBase::DHT) 
			return;

		ou->getClient().reportUser(ou->getIdentity());
	}
}
