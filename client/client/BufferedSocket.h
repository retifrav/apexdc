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

#ifndef DCPLUSPLUS_DCPP_BUFFERED_SOCKET_H
#define DCPLUSPLUS_DCPP_BUFFERED_SOCKET_H


#include <deque>
#include <memory>

#include "typedefs.h"

#include "BufferedSocketListener.h"
#include "Semaphore.h"
#include "Thread.h"
#include "Speaker.h"
#include "Socket.h"

#include "atomic.h"

namespace dcpp {

using std::deque;
using std::pair;
using std::unique_ptr;

class BufferedSocket : public Speaker<BufferedSocketListener>, private Thread {
public:
	enum Modes {
		MODE_LINE,
		MODE_ZPIPE,
		MODE_DATA
	};

	enum NatRoles {
		NAT_NONE,
		NAT_CLIENT,
		NAT_SERVER
	};

	/**
	 * BufferedSocket factory, each BufferedSocket may only be used to create one connection
	 * @param sep Line separator
	 * @return An unconnected socket
	 */
	static BufferedSocket* getSocket(char sep) {
		return new BufferedSocket(sep); 
	}

	static void putSocket(BufferedSocket* aSock) { 
		if(aSock) {
			aSock->removeListeners(); 
			aSock->shutdown();
		}
	}

	static void waitShutdown() {
		while(sockets > 0)
			Thread::sleep(100);
	}

	void accept(const Socket& srv, Socket* dest);
	void accept(const Socket& srv, bool secure, bool allowUntrusted, const string& expKP = Util::emptyString);
	void connect(const string& aAddress, uint16_t aPort, bool secure, bool allowUntrusted, bool proxy, const string& expKP = Util::emptyString);
	void connect(const string& aAddress, uint16_t aPort, uint16_t localPort, NatRoles natRole, bool secure, bool allowUntrusted, bool proxy, const string& expKP = Util::emptyString);

	/** Sets data mode for aBytes bytes. Must be called within onLine. */
	void setDataMode(int64_t aBytes = -1) { mode = MODE_DATA; dataBytes = aBytes; }
	/** 
	 * Rollback is an ugly hack to solve problems with compressed transfers where not all data received
	 * should be treated as data.
	 * Must be called from within onData. 
	 */
	void setLineMode(size_t aRollback) { setMode (MODE_LINE, aRollback);}
	void setMode(Modes mode, size_t aRollback = 0);
	Modes getMode() const { return mode; }
	const string& getIp() const { return sock->getIp(); }
	uint16_t getPort() { return sock->getPort(); }
	bool isConnected() const { return sock->isConnected(); }
	
	bool isSecure() const { return sock->isSecure(); }
	bool isTrusted() const { return sock->isTrusted(); }
	string getCipherName() const { return sock->getCipherName(); }
	ByteVector getKeyprint() const { return sock->getKeyprint(); }
	bool verifyKeyprint(const string& expKP, bool allowUntrusted) noexcept { return sock->verifyKeyprint(expKP, allowUntrusted); }

	void write(const string& aData) { write(aData.data(), aData.length()); }
	void write(const char* aBuf, size_t aLen) noexcept;
	/** Send the file f over this socket. */
	void transmitFile(InputStream* f) { Lock l(cs); addTask(SEND_FILE, new SendFileInfo(f)); }

	/** Send an updated signal to all listeners */
	void updated() { Lock l(cs); addTask(UPDATED, 0); }

	void disconnect(bool graceless = false) noexcept { Lock l(cs); if(graceless) disconnecting = true; addTask(DISCONNECT, 0); }

	string getLocalIp() const { return sock->getLocalIp(); }
	uint16_t getLocalPort() const { return sock->getLocalPort(); }
	bool hasSocket() const { return sock.get() != NULL; }

	GETSET(char, separator, Separator);
	GETSET(bool, superUser, SuperUser);
private:
	enum Tasks {
		CONNECT,
		DISCONNECT,
		SEND_DATA,
		SEND_FILE,
		SHUTDOWN,
		ACCEPTED,
		UPDATED
	};

	enum State {
		STARTING, // Waiting for CONNECT/ACCEPTED/SHUTDOWN
		RUNNING,
		FAILED
	};

	struct TaskData { 
		~TaskData() { }
	};
	struct ConnectInfo : public TaskData {
		ConnectInfo(string addr_, uint16_t port_, uint16_t localPort_, NatRoles natRole_, bool proxy_) : addr(addr_), port(port_), localPort(localPort_), natRole(natRole_), proxy(proxy_) { }
		string addr;
		uint16_t port;
		uint16_t localPort;
		NatRoles natRole;
		bool proxy;
	};
	struct SendFileInfo : public TaskData {
		SendFileInfo(InputStream* stream_) : stream(stream_) { }
		InputStream* stream;
	};

	BufferedSocket(char aSeparator);

	~BufferedSocket();

	CriticalSection cs;

	Semaphore taskSem;
	deque<pair<Tasks, unique_ptr<TaskData> > > tasks;
	ByteVector inbuf;
	ByteVector writeBuf;
	ByteVector sendBuf;
	
	string line;
	int64_t dataBytes;
	size_t rollback;

	Modes mode;
	State state;

	std::unique_ptr<UnZFilter> filterIn;
	std::unique_ptr<Socket> sock;

	bool disconnecting;
	
	int run();

	void threadConnect(const string& aAddr, uint16_t aPort, uint16_t localPort, NatRoles natRole, bool proxy);
	void threadAccept();
	void threadRead();
	void threadSendFile(InputStream* is);
	void threadSendData();

	void fail(const string& aError);	
	static atomic<long> sockets;

	bool checkEvents();
	void checkSocket();

	void setSocket(std::unique_ptr<Socket> s);
	void shutdown();
	void addTask(Tasks task, TaskData* data);
};

} // namespace dcpp

#endif // !defined(BUFFERED_SOCKET_H)

/**
 * @file
 * $Id$
 */
