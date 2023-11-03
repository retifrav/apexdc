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

#ifndef DCPLUSPLUS_DCPP_CONNECTIVITY_MANAGER_H
#define DCPLUSPLUS_DCPP_CONNECTIVITY_MANAGER_H

#include <string>

#include "Speaker.h"
#include "Singleton.h"

#include "LogManager.h"

namespace dcpp {

using std::string;

class ConnectivityManagerListener {
public:
	virtual ~ConnectivityManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Message;
	typedef X<1> Started;
	typedef X<2> Finished;
	typedef X<3> SettingChanged; // auto-detection has been enabled / disabled

	virtual void on(Message, const string&) noexcept { }
	virtual void on(Started) noexcept { }
	virtual void on(Finished) noexcept { }
	virtual void on(SettingChanged) noexcept { }
};

class ConnectivityManager : public Singleton<ConnectivityManager>, public Speaker<ConnectivityManagerListener>
{
public:
	void detectConnection();
	void setup(bool settingsChanged);
	bool isRunning() const { return running; }
	const string& getStatus() { return status; }

private:
	friend class Singleton<ConnectivityManager>;
	friend class MappingManager;
	
	ConnectivityManager();
	~ConnectivityManager() { }

	void mappingFinished(const string& mapper);
	void log(const string& msg, LogManager::Severity severity);

	void startSocket();
	void listen();
	void disconnect();

	bool autoDetected;
	bool running;

	string status;
};

} // namespace dcpp

#endif // !defined(CONNECTIVITY_MANAGER_H)
