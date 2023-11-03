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
#include "ConnectivityManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "LogManager.h"
#include "MappingManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"

#include "WebServerManager.h"
#include "../dht/DHT.h"

namespace dcpp {

using dht::DHT;

ConnectivityManager::ConnectivityManager() :
autoDetected(false),
running(false)
{
}

void ConnectivityManager::startSocket() {
	autoDetected = false;

	disconnect();

	// active check mustn't be there to hub-dependent settings to work correctly
	//if(ClientManager::getInstance()->isActive()) {
		listen();

		// must be done after listen calls; otherwise ports won't be set
		if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP)
			MappingManager::getInstance()->open();
	//}
}

void ConnectivityManager::detectConnection() {
	if(running)
		return;

	running = true;

	status.clear();
	fire(ConnectivityManagerListener::Started());

	// restore connectivity settings to their default value.
	SettingsManager::getInstance()->unset(SettingsManager::TCP_PORT);
	SettingsManager::getInstance()->unset(SettingsManager::UDP_PORT);
	SettingsManager::getInstance()->unset(SettingsManager::TLS_PORT);
	SettingsManager::getInstance()->unset(SettingsManager::DHT_PORT);
	SettingsManager::getInstance()->unset(SettingsManager::EXTERNAL_IP);
	SettingsManager::getInstance()->unset(SettingsManager::NO_IP_OVERRIDE);
	SettingsManager::getInstance()->unset(SettingsManager::BIND_ADDRESS);

	if (MappingManager::getInstance()->getOpened()) {
		MappingManager::getInstance()->close();
	}

	disconnect();

	log("Determining the best connectivity settings...", LogManager::LOG_INFO);
	try {
		listen();
	} catch(const Exception& e) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_PASSIVE);
		SettingsManager::getInstance()->set(SettingsManager::ALLOW_NAT_TRAVERSAL, true);
		log("Unable to open " + e.getError() + " port(s); connectivity settings must be configured manually", LogManager::LOG_WARNING);
		fire(ConnectivityManagerListener::Finished());
		running = false;
		return;
	}

	autoDetected = true;

	if (!Util::isPrivateIp(Util::getLocalIp())) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_DIRECT);
		log("Public IP address detected, selecting active mode with direct connection", LogManager::LOG_INFO);
		fire(ConnectivityManagerListener::Finished());
		running = false;
		return;
	}

	SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_UPNP);
	log("Local network with possible NAT detected, trying to map the ports...", LogManager::LOG_INFO);

	if (!MappingManager::getInstance()->open()) {
		running = false;
	}
}

void ConnectivityManager::setup(bool settingsChanged) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		if (!autoDetected) detectConnection();
	} else {
		if(autoDetected || settingsChanged) {
			if(settingsChanged || (SETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_FIREWALL_UPNP)) {
				MappingManager::getInstance()->close();
			}
			startSocket();
		} else if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP && !MappingManager::getInstance()->getOpened()) {
			// previous mappings had failed; try again
			MappingManager::getInstance()->open();
		}
	}
}

void ConnectivityManager::mappingFinished(const string& mapper) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		if(mapper.empty()) {
			// don't disconnect when mapping fails otherwise DHT and active mode in favorite hubs won't work
			//disconnect();
			SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_PASSIVE);
			log("Active mode could not be achieved; a manual configuration is recommended for better connectivity", LogManager::LOG_WARNING);
		}
		fire(ConnectivityManagerListener::Finished());
	}

	// Always set the mapper that succeeded...
	if(!mapper.empty())
		SettingsManager::getInstance()->set(SettingsManager::MAPPER, mapper);

	running = false;
}

void ConnectivityManager::listen() {
	try {
		ConnectionManager::getInstance()->listen();
	} catch(const Exception&) {
		throw Exception("Transfer (TCP)");
	}

	try {
		SearchManager::getInstance()->listen();
	} catch(const Exception&) {
		throw Exception("Search (UDP)");
	}

	try {
		DHT::getInstance()->start();
	} catch(const Exception&) {
		throw Exception("DHT (UDP)");
	}
}

void ConnectivityManager::disconnect() {
	DHT::getInstance()->stop();
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();
}

void ConnectivityManager::log(const string& message, LogManager::Severity severity) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		status = move(message);
		LogManager::getInstance()->message("Connectivity: " + status, severity);
		fire(ConnectivityManagerListener::Message(), status);
	} else {
		LogManager::getInstance()->message(message, severity);
	}
}

} // namespace dcpp
