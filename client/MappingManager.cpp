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
#include "MappingManager.h"

#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "LogManager.h"
#include "SearchManager.h"
#include "WebServerManager.h"
#include "ScopedFunctor.h"
#include "version.h"
#include "format.h"

#include "../dht/DHT.h"

namespace dcpp {

using dht::DHT;

atomic_flag MappingManager::busy = ATOMIC_FLAG_INIT;

StringList MappingManager::getMappers() const {
	StringList ret;
	for(auto i = mappers.cbegin(), iend = mappers.cend(); i != iend; ++i)
		ret.push_back(i->first);
	return ret;
}

bool MappingManager::open() {
	if(getOpened())
		return false;

	if(mappers.empty()) {
		log(STRING(MAPPING_NO_IMPLEMENTATION), LogManager::LOG_WARNING);
		return false;
	}

	if(busy.test_and_set()) {
		log(STRING(MAPPING_ANOTHER_MAPPING), LogManager::LOG_WARNING);
		return false;
	} 

	start();

	return true;
}

void MappingManager::close() {
	TimerManager::getInstance()->removeListener(this);

	if(working.get()) {
		close(*working);
		working.reset();
	}
}

bool MappingManager::getOpened() const {
	return working.get() != NULL;
}

int MappingManager::run() {
	ScopedFunctor([this] { busy.clear(); });

	// cache these
	const unsigned short
		conn_port = ConnectionManager::getInstance()->getPort(),
		secure_port = ConnectionManager::getInstance()->getSecurePort(),
		search_port = SearchManager::getInstance()->getPort(),
		web_port = WebServerManager::getInstance()->getPort(),
		dht_port = DHT::getInstance()->getPort();

	if(renewal) {
		Mapper& mapper = *working;

		ScopedFunctor([&mapper] { mapper.uninit(); });
		if(!mapper.init()) {
			// can't renew; try again later.
			renewal = GET_TICK() + std::max(mapper.renewal(), 10u) * 60 * 1000;
			return 0;
		}

		auto addRule = [this, &mapper](const unsigned short port, Mapper::Protocol protocol, const string& description) {
			// just launch renewal requests - don't bother with possible failures.
			if(port) {
				mapper.open(port, protocol, str(dcpp_fmt("%1% %2% port (%3% %4%)") %
					APPNAME % description % port % Mapper::protocols[protocol]));
			}
		};

		addRule(conn_port, Mapper::PROTOCOL_TCP, "Transfer");
		addRule(secure_port, Mapper::PROTOCOL_TCP, "Encrypted transfer");
		addRule(search_port, Mapper::PROTOCOL_UDP, "Search");
		addRule(web_port, Mapper::PROTOCOL_TCP, "Web UI");
		addRule(dht_port, Mapper::PROTOCOL_UDP, "DHT");

		auto minutes = mapper.renewal();
		if(minutes) {
			renewal = GET_TICK() + std::max(minutes, 10u) * 60 * 1000;
		} else {
			TimerManager::getInstance()->removeListener(this);
		}

		return 0;
	}

	// move the preferred mapper to the top of the stack.
	const auto& setting = SETTING(MAPPER);
	for(auto i = mappers.begin(); i != mappers.end(); ++i) {
		if(i->first == setting) {
			if(i != mappers.begin()) {
				auto mapper = *i;
				mappers.erase(i);
				mappers.insert(mappers.begin(), mapper);
			}
			break;
		}
	}

	for(auto i = mappers.begin(); i != mappers.end(); ++i) {
		unique_ptr<Mapper> pMapper(i->second());
		Mapper& mapper = *pMapper;

		ScopedFunctor([&mapper] { mapper.uninit(); });
		if(!mapper.init()) {
			log(STRING_F(MAPPING_INTERFACE_FAILED_INIT, mapper.getName()), LogManager::LOG_WARNING);
			continue;
		}

		auto addRule = [this, &mapper](const unsigned short port, Mapper::Protocol protocol, const string& description) -> bool {
			if(port && !mapper.open(port, protocol, str(dcpp_fmt("%1% %2% port (%3% %4%)") %
				APPNAME % description % port % Mapper::protocols[protocol])))
			{
				this->log(STRING_F(MAPPING_INTERFACE_FAILED_PORT, mapper.getName() % description % port % Mapper::protocols[protocol]), LogManager::LOG_WARNING);
				mapper.close();
				return false;
			}
			return true;
		};

		if(!(addRule(conn_port, Mapper::PROTOCOL_TCP, "Transfer") &&
			addRule(secure_port, Mapper::PROTOCOL_TCP, "Encrypted transfer") &&
			addRule(search_port, Mapper::PROTOCOL_UDP, "Search") &&
			addRule(web_port, Mapper::PROTOCOL_TCP, "Web UI") &&
			addRule(dht_port, Mapper::PROTOCOL_UDP, "DHT")))
			continue;

		log(STRING_F(MAPPING_SUCCESSFULLY_CREATED_MAPPINGS, conn_port % search_port % secure_port % deviceString(mapper) % mapper.getName()), LogManager::LOG_INFO);

		working = move(pMapper);

		if(!BOOLSETTING(NO_IP_OVERRIDE)) {
			string externalIP = mapper.getExternalIP();
			if(!externalIP.empty()) {
				SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, externalIP);
			} else {
				// no cleanup because the mappings work and hubs will likely provide the correct IP.
				log(STRING(MAPPING_FAILED_TO_GET_EXTERNAL_IP), LogManager::LOG_WARNING);
			}
		}

		ConnectivityManager::getInstance()->mappingFinished(mapper.getName());

		auto minutes = mapper.renewal();
		if(minutes) {
			renewal = GET_TICK() + std::max(minutes, 10u) * 60 * 1000;
			TimerManager::getInstance()->addListener(this);
		}
		break;
	}

	if(!getOpened()) {
		log(STRING(MAPPING_FAILED_TO_CREATE_MAPPINGS), LogManager::LOG_WARNING);
		ConnectivityManager::getInstance()->mappingFinished(Util::emptyString);
	}

	return 0;
}

void MappingManager::close(Mapper& mapper) {
	if(mapper.hasRules()) {
		bool ret = mapper.init();
		if(ret) {
			ret = mapper.close();
			mapper.uninit();
		}
		if(ret) {
			log(STRING_F(MAPPING_REMOVED_MAPPINGS, deviceString(mapper) % mapper.getName()), LogManager::LOG_INFO);
		} else log(STRING_F(MAPPING_FAILED_TO_REMOVE_MAPPINGS, deviceString(mapper) % mapper.getName()), LogManager::LOG_WARNING);
	}
}

void MappingManager::log(const string& message, LogManager::Severity severity) {
	ConnectivityManager::getInstance()->log(STRING_F(MAPPING_LOG_MESSAGE, message), severity);
}

string MappingManager::deviceString(Mapper& mapper) const {
	string name(mapper.getDeviceName());
	if(name.empty())
		name = "Generic";
	return '"' + name + '"';
}

void MappingManager::on(TimerManagerListener::Minute, uint64_t tick) noexcept {
	if(tick >= renewal && !busy.test_and_set())
		start();
}

} // namespace dcpp
