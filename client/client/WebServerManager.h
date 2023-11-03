/* 
 * Copyright (C) 2003 Twink, spm7@waikato.ac.nz
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

#ifndef DCPLUSPLUS_DCPP_WEB_SERVER_MANAGER_H
#define DCPLUSPLUS_DCPP_WEB_SERVER_MANAGER_H

#include <boost/scoped_array.hpp>

#include "forward.h"
#include "typedefs.h"

#include "BufferedSocket.h"

#include "SearchManager.h"
#include "Singleton.h"
#include "Thread.h"
#include "Speaker.h"
#include "ZipFile.h"

namespace dcpp {

class WebServerListener {
public:
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Setup;
	typedef X<1> ShutdownPC;

	virtual void on(Setup) noexcept { };
	virtual void on(ShutdownPC, int action) noexcept = 0;

};

class WebServer : public Thread
{
public:
	WebServer(bool aSecure, uint16_t aPort, const string& aIp = "0.0.0.0");
	~WebServer() { die = true; join(); }

	static string getServerDate(time_t t = GET_TIME());
	static bool parseServerDate(const string& date, time_t& time);

	uint16_t getPort() const { return port; }

private:
	int run() noexcept;

	Socket sock;
	uint16_t port;
	string ip;
	bool secure;
	bool die;
};

class WebConnection : private BufferedSocketListener
{
public:
	enum ReqType {
		REQ_UNKNOWN = 0,
		REQ_GET,
		REQ_POST,
		REQ_HEAD,
		REQ_COMPLETE
	};

	enum CompMethod {
		COMPRESS_NONE = 0,
		COMPRESS_GZIP,
		COMPRESS_DEFLATE
	};

	WebConnection(bool aSecure);
	~WebConnection();

	void accept(const Socket& sock);
	void disconnect(bool graceless = false) { if(sock) sock->disconnect(graceless); }

	ReqType getType() const { return request.type; }
	CompMethod getCompression() const { return request.compress; }
	const string& getRemoteIp() const { return sock ? sock->getIp() : Util::emptyString; }

	const string& getPage() const { return request.page; }
	string getReqString() const { return request.page + getArg("args"); }

	const string& getArg(const string& name) const {
		StringMap::const_iterator i = request.args.find(name);
		if(i != request.args.end())
			return i->second;
		return Util::emptyString;
	}

	struct WebResponse {
		string header;
		string page;
		std::unique_ptr<InputStream> file;
	};

private:
	typedef boost::scoped_array<uint8_t> ByteArray;

	struct ContentType {
		const char* ext;
		const char* type;
		bool compress;
	};

	void getArgs(const string& arguments, StringMap& args, bool reset = false);
	void requestParse(string& page, StringMap& args);
	string processArgs(const string& page, StringMap args, ReqType type = REQ_GET);
	ContentType getContentTypeFromExt(const string& ext);
	size_t compressResponse(const string& data, CompMethod compression, ByteArray& compressed);

	// BufferedSocketListener
	void on(BufferedSocketListener::Line, const string& aLine) noexcept;
	void on(BufferedSocketListener::ModeChange) noexcept;
	void on(BufferedSocketListener::Data, uint8_t* aBuf, size_t aLen) noexcept { request.dataSeq.append((const char*)aBuf, aLen); }
	void on(BufferedSocketListener::Failed, const string& /*aLine*/) noexcept;

	// This is the current request
	struct WebRequest {
		ReqType type;
		CompMethod compress;
		time_t time;
		time_t modified;
		string page;
		StringMap args;

		// POST request incoming data
		uint64_t dataSize;
		string dataSeq;
	} request;

	// This is the response
	WebResponse response;

	BufferedSocket* sock;
	bool secure;
	bool closing;
};

class WebServerManager :  public Singleton<WebServerManager>, public Speaker<WebServerListener>, private SearchManagerListener
{
public:
	struct StaticFileInfo {
		StaticFileInfo(time_t ft, int64_t fs) : time(ft), size(fs) {}
		time_t time;
		int64_t size;
	};

	void start();
	void stop();
	void shutdown();
	void restart() { stop(); start(); }

	void accept(const Socket& sock, bool secure) noexcept;
	void disconnected(WebConnection* conn) noexcept;

	void doAction(const string& group, int action);

	bool getPage(WebConnection::WebResponse& response, const string& file, const string& IP);
	void getErrorPage(WebConnection::WebResponse& response, const string& errStr);
	void getLoginPage(WebConnection::WebResponse& response, const string& dest = "/index.html");
	StaticFileInfo getStaticFile(WebConnection::WebResponse& response, const string& file, bool compress);

	void search(string searchStr, SearchManager::TypeModes ftype);
	void reset(bool cancel = false);

	void login(const string& ip) {
		Lock l(cs);
		sessions[ip] = GET_TICK();
	}

	void logout(const string& ip) {
		Lock l(cs);
		SessionMap::iterator i;
		if((i = sessions.find(ip)) != sessions.end())
			sessions.erase(i);
	}

	bool isLoggedIn(const string& ip) {
		Lock l(cs);
		SessionMap::iterator i;
		if((i = sessions.find(ip)) != sessions.end()) {
            uint64_t elapsed = (GET_TICK() - i->second) / 1000;
			if(elapsed > 600) {
				sessions.erase(i);
				return false;
			}

			i->second = GET_TICK();
			return true;
		}

		return false;
	}

	uint16_t getPort() const { return server ? static_cast<uint16_t>(server->getPort()) : 0; }

private:
	friend class Singleton<WebServerManager>;

	WebServerManager() : started(false), sentSearch(false), searchInterval(10000), server(NULL) { }
	~WebServerManager() { shutdown(); }

	enum PageID {
		PAGE_INDEX,
		PAGE_DOWNLOAD_QUEUE,
		PAGE_DOWNLOAD_FINISHED,
		PAGE_UPLOAD_QUEUE,
		PAGE_UPLOAD_FINISHED,
		PAGE_SEARCH,
		PAGE_LOG,
		PAGE_SYSLOG,
		PAGE_ACTION,
		PAGE_LOGOUT,
		PAGE_404,
		PAGE_403
	};

	struct WebPageInfo {
		WebPageInfo(PageID n,string t) : id(n), title(t) {}
		PageID id;
		string title;
	};

	typedef unordered_map<string, WebPageInfo*, noCaseStringHash, noCaseStringEq> PageMap;
	typedef map<string, uint64_t> SessionMap;
	typedef vector<WebConnection*> ConnectionMap;

	string getDLQueue();
	string getULQueue();
	string getFinished(bool uploads);
	string getSearch();	
	string getLogs();
	string getSysLogs();

	// SearchManagerListener
	void on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept;

	bool sentSearch;
	StringList sClients;
	uint64_t searchInterval;
	int row;
	string token;
	string results;

	bool started;
	CriticalSection cs;
	WebServer* server;

	PageMap pages;
	ZipFile::FileMap staticFiles;
	SessionMap sessions;
	ConnectionMap activeConnections;
};

} // namespace dcpp

#endif	// DCPLUSPLUS_DCPP_WEB_SERVER_MANAGER_H
