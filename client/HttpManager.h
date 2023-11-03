/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_HTTP_MANAGER_H
#define DCPLUSPLUS_DCPP_HTTP_MANAGER_H

#include <string>
#include <functional>

#include "forward.h"
#include "Flags.h"
#include "CriticalSection.h"
#include "HttpConnectionListener.h"
#include "HttpManagerListener.h"
#include "Singleton.h"
#include "Speaker.h"
#include "TimerManager.h"
#include "typedefs.h"
#include "Util.h"

namespace dcpp {

using std::function;
using std::string;
using std::map;

class HttpManager :
	public Singleton<HttpManager>,
	public Speaker<HttpManagerListener>,
	private HttpConnectionListener,
	private TimerManagerListener
{
public:
	typedef function<void (const HttpConnection*, OutputStream*, Flags::MaskType)> CallBack;

	enum ConnFlags {
		CONN_MANAGED = 0x01,
		CONN_STRING = 0x02,
		CONN_FILE = 0x04,
		CONN_COMPLETE = 0x08,
		CONN_FAILED = 0x10,
	};

	/** Send a GET request to the designated url. If unspecified, the stream defaults to a
	StringOutputStream. */
	HttpConnection* download(string url, OutputStream* stream, CallBack callback = CallBack(), const string& userAgent = Util::emptyString);
	HttpConnection* download(string url, CallBack callback = CallBack(), const string& userAgent = Util::emptyString) { return download(url, nullptr, callback, userAgent); }
	/** Send a POST request to the designated url. If unspecified, the stream defaults to a
	StringOutputStream. */
	HttpConnection* download(string url, const StringMap& postData, OutputStream* stream, CallBack callback = CallBack(), const string& userAgent = Util::emptyString);
	HttpConnection* download(string url, const StringMap& postData, CallBack callback = CallBack(), const string& userAgent = Util::emptyString) { return download(url, postData, nullptr, callback, userAgent); }
	/** Send a GET request to the designated url. Uses a managed File stream */
	HttpConnection* download(string url, const string& file, CallBack callback = CallBack(), const string& userAgent = Util::emptyString);

	void disconnect(const string& url);

	void shutdown();

private:
	struct Conn : Flags {
		Conn(HttpConnection* conn, OutputStream* os, CallBack cbfunc, uint64_t rtime = 0) : c(conn), stream(os), remove(rtime), callback(cbfunc) { }

		HttpConnection* c; 
		OutputStream* stream; 
		uint64_t remove;
		CallBack callback;
	};

	friend class Singleton<HttpManager>;

	HttpManager();
	~HttpManager();

	File* createFile(const string& file);
	bool useCoral(const string& url);

	HttpConnection* makeConn(string&& url, OutputStream* stream, CallBack callback, const string& userAgent, Flags::MaskType connFlags = 0);
	Conn* findConn(HttpConnection* c);
	void resetStream(HttpConnection* c);
	void removeLater(HttpConnection* c);

	// HttpConnectionListener
	void on(HttpConnectionListener::Data, HttpConnection*, const uint8_t*, size_t) noexcept;
	void on(HttpConnectionListener::Failed, HttpConnection*, const string&) noexcept;
	void on(HttpConnectionListener::Complete, HttpConnection*) noexcept;
	void on(HttpConnectionListener::Redirected, HttpConnection*, const string&) noexcept;

	// TimerManagerListener
	void on(TimerManagerListener::Minute, uint64_t tick) noexcept;

	mutable CriticalSection cs;
	vector<Conn> conns;
	map<HttpConnection*, CallBack> callbacks;
};

} // namespace dcpp

#endif
