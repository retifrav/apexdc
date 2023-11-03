/* 
 * Copyright (C) 2006-2011 Crise, crise<at>mail.berlios.de
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

#ifndef USER_MAP_H
#define USER_MAP_H

#include "QueueManager.h"
#include "Client.h"
#include "Thread.h"

namespace dcpp {

template<bool isADC, typename BaseMap>
class UserMap : public BaseMap 
{
public:
	UserMap() : checker(NULL) { };
	~UserMap() { stopChecking(); };

	void startCheckOnConnect(Client* client) {
		if((checker == NULL) && !client->getExclChecks()) {
			checker = new ThreadedCheck(client, this);
		} else return;

		if(!checker->isChecking()) {
			checker->setCheckOnConnect(true);
			checker->start();
		}
	}

	bool startChecking(Client* client) {
		if(checker == NULL) {
			checker = new ThreadedCheck(client, this);
		}

		if(!checker->isChecking()) {
			checker->start();
			return true;
		}

		return false;
	}

	void stopChecking() {
		if(checker != NULL) {
			if(checker->isChecking()) {
				checker->cancel();
				QueueManager::getInstance()->removeOfflineChecks();
			}

			delete checker;
			checker = NULL;
		}
	}

private:

	// ThreadedCheck
	class ThreadedCheck : public Thread
	{
		public:
			ThreadedCheck(Client* aClient, BaseMap* aUsers) : client(aClient), users(aUsers), keepChecking(false), checkOnConnect(false), stop(true) { };
			~ThreadedCheck() { join(); }

			bool isChecking() { return !stop; }

			void start() {
				stop = false;
				Thread::start();
			}

			void cancel() {
				keepChecking = false;
				checkOnConnect = false;
				stop = true;
			}

			GETSET(bool, checkOnConnect, CheckOnConnect);

		private:
			int run() {
				setThreadPriority(Thread::IDLE);
				if(checkOnConnect) { 
					sleep(SETTING(CHECK_DELAY) + 1000);
					checkOnConnect = false;
				}

				keepChecking = (client->isConnected() && client->isActive() && !stop);
				int sleepTime = SETTING(SLEEP_TIME);
				uint8_t secs = 0;

				while(keepChecking) {
					if(client->isConnected()) {
						int f = 0;

						QueueManager::getInstance()->lockedOperation([&f](const QueueItem::StringMap& queue) {
							for(auto i = queue.cbegin(); i != queue.cend(); ++i) {
								if(i->second->isSet(QueueItem::FLAG_USER_CHECK))
									++f;
							}
						});

						if(f < SETTING(MAX_FILELISTS)) {
							Lock l(client->cs);
							for(auto i = users->cbegin(); i != users->cend(); ++i) {
								OnlineUserPtr ou = i->second;
								if(!client->isConnected() || stop) break;
								if(!ou->isHidden() && (!isADC || !ou->getUser()->isSet(User::NO_ADC_1_0_PROTOCOL))) {
									if(ou->isCheckable() && ou->shouldCheckFileList() && !ou->getChecked()) {
										try {
											QueueManager::getInstance()->addList(HintedUser(ou->getUser(), client->getHubUrl()), QueueItem::FLAG_USER_CHECK);
											ou->getIdentity().set("FQ", "1");
											break;
										} catch(...) {
											dcdebug("Exception adding filelist %s\n", ou->getIdentity().getNick().c_str());
										}
									}
								}
							}
						}

						if(secs >= 60) {
							QueueManager::getInstance()->removeOfflineChecks();
							secs = 0;
						} else {
							++secs;
						}

						sleep(sleepTime);
					} else {
						sleep(10000);
					}
				}

				stop = true;
				return 0;
			}

			bool keepChecking;
			bool stop;

			Client* client;
			BaseMap* users;
	} *checker;
};

} // namespace dcpp

#endif // !defined(USER_MAP_H)