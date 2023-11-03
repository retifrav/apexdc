/* 
* Copyright (C) 2003 Twink, spm7@waikato.ac.nz
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of+
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdinc.h"
#include "WebServerManager.h"

#include "SettingsManager.h"
#include "QueueManager.h"
#include "FinishedManager.h"
#include "LogManager.h"
#include "ClientManager.h"
#include "UploadManager.h"
#include "StringTokenizer.h"
#include "ResourceManager.h"
#include "SearchResult.h"
#include "FavoriteManager.h"
#include "SSLSocket.h"
#include "ZUtils.h"
#include "version.h"

namespace dcpp {

static const char* monNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static const char* dayNames[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const uint32_t POLL_TIMEOUT = 250;

WebServer::WebServer(bool aSecure, uint16_t aPort, const string& aIp /* = "0.0.0.0" */) : port(0), secure(aSecure), die(false) {
	sock.create();
	sock.setSocketOpt(SO_REUSEADDR, 1);
	ip = aIp;
	port = sock.bind(aPort, ip);
	sock.listen();

	start();
}

string WebServer::getServerDate(time_t t /*= GET_TIME()*/) {
	struct tm* tmp = gmtime(&t);
	if(tmp) {
		struct tm gmt = *tmp;
		char buf[256];

		snprintf(buf, sizeof(buf), "%s, %0.2d %s %d %0.2d:%0.2d:%0.2d GMT", 
			dayNames[gmt.tm_wday], gmt.tm_mday, monNames[gmt.tm_mon],
			1900 + gmt.tm_year, gmt.tm_hour, gmt.tm_min, gmt.tm_sec);

		return buf;
	} else return Util::emptyString;
}

bool WebServer::parseServerDate(const string& date, time_t& time) {
	struct tm t;
	t.tm_isdst = -1;

	char month[4];
	int count = sscanf(date.c_str(), "%*3s, %2d %3s %4d %2d:%2d:%2d GMT",
		&t.tm_mday, month, &t.tm_year, &t.tm_hour, &t.tm_min, &t.tm_sec);

	if(count != 6) {
		// The most likely case failed...
		count = sscanf(date.c_str(), "%*s, %2d-%3s-%2d %2d:%2d:%2d GMT",
			&t.tm_mday, month, &t.tm_year, &t.tm_hour, &t.tm_min, &t.tm_sec);
		if(count != 6) {
			count = sscanf(date.c_str(), "%*3s %3s %2d %2d:%2d:%2d %4d", month, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec, &t.tm_year);
			if(count == 6) {
				t.tm_year -= 1900;
			} else return false;
		} else if(t.tm_year < 70) t.tm_year += 100;
	} else t.tm_year -= 1900;

	// Set the month correctly
	for(int i = 0; i < 12; ++i) {
		if(strcmp(monNames[i], month) == 0) {
			t.tm_mon = i;
			break;
		}
	}

	time = timegm(&t);
	return (time != (time_t)-1);
}

int WebServer::run() noexcept {
	while(!die) {
		try {
			while(!die) {
				if(sock.wait(POLL_TIMEOUT, Socket::WAIT_READ) == Socket::WAIT_READ) {
					WebServerManager::getInstance()->accept(sock, secure);
				}
			}
		} catch(const Exception& e) {
			dcdebug("WebServer::run Error: %s\n", e.getError().c_str());
		}			

		bool failed = false;
		while(!die) {
			try {
				sock.disconnect();
				sock.create();
				sock.bind(port, ip);
				sock.listen();
				if(failed) {
					LogManager::getInstance()->message("Connectivity restored", LogManager::LOG_INFO); // TODO: translate
					failed = false;
				}
				break;
			} catch(const SocketException& e) {
				dcdebug("WebServer::run Stopped listening: %s\n", e.getError().c_str());

				if(!failed) {
					LogManager::getInstance()->message("Connectivity error: " + e.getError(), LogManager::LOG_ERROR);
					failed = true;
				}

				// Spin for 60 seconds
				for(int i = 0; i < 60 && !die; ++i) {
					Thread::sleep(1000);
				}
			}
		}
	}
	return 0;
}

WebConnection::WebConnection(bool aSecure) : sock(NULL), secure(aSecure), closing(false) {
	request.type = REQ_UNKNOWN;
	request.compress = COMPRESS_NONE;
	request.time = GET_TIME();
	request.modified = (time_t)-1;
	request.dataSize = 0;
}

WebConnection::~WebConnection() {
	if (sock) BufferedSocket::putSocket(sock);
}

void WebConnection::accept(const Socket& aServer) {
	// Limiter is not applied to these...
	sock = BufferedSocket::getSocket(0x0a);
	sock->setSuperUser(true);

	sock->addListener(this);
	sock->accept(aServer, secure ? new SSLSocket(CryptoManager::SSL_SERVER) : new Socket());
}

void WebConnection::getArgs(const string& arguments, StringMap& args, bool reset /* = false*/) {
	if(reset) {
		args.clear();
		args["args"] = "?" + arguments;
	}

	string::size_type i = 0, j = 0;
	while((i = arguments.find("=", j)) != string::npos) {
		string key = arguments.substr(j, i-j);
		string value = arguments.substr(i + 1);;

		j = i + 1;
		if((i = arguments.find("&", j)) != string::npos) {
			value = arguments.substr(j, i-j);
			j = i + 1;
		} 

		args[key] = value;
	}
}

void WebConnection::requestParse(string& page, StringMap& args) {
	if(BOOLSETTING(LOG_WEBSERVER)) {
		StringMap params;
		params["file"] = page;
		params["ip"] = sock->getIp();
		LOG(LogManager::WEBSERVER, params);
	}

	string::size_type start = 0;
	if((start = page.find("?")) != string::npos) {
		getArgs(page.substr(start+1), args, true);
		page = page.substr(0, start);
	}

	// Validate filename
	if(request.page[page.size()-1] == '/') {
		page.append("index.html");
	} else if(Util::getFileExt(page).empty()) {
		page.append("/index.html");
	} else if(Util::getFileExt(page) == ".htm") {
		page.append("l");
	}
}

string WebConnection::processArgs(const string& page, StringMap args, ReqType type /*= REQ_GET*/) {
	if(type == REQ_POST) {
		if(!WebServerManager::getInstance()->isLoggedIn(sock->getIp())) {
			if((Util::encodeURI(args["user"], true).compare(SETTING(WEBSERVER_USER)) == 0) && (Util::encodeURI(args["pass"], true).compare(SETTING(WEBSERVER_PASS)) == 0)) {
				WebServerManager::getInstance()->login(sock->getIp());
			}
		} else if(page == "/add_download.html") {
			try {
				string name = Util::encodeURI(args["name"], true);
				QueueManager::getInstance()->add(FavoriteManager::getInstance()->getDownloadDirectory(Util::getFileExt(name)) + name, Util::toInt64(args["size"]), TTHValue(args["tth"]), HintedUser(UserPtr(), Util::emptyString));
			} catch(const Exception&) {
				// ...
			}
		}

		return Util::encodeURI(args["redirect"], true);
	}

	if(page == "/search.html") {
		if(!args["query"].empty()) {
			WebServerManager::getInstance()->search(Util::encodeURI(args["query"], true), static_cast<SearchManager::TypeModes>(Util::toInt(args["type"])));
		} else if(!args["stop"].empty()) {
			WebServerManager::getInstance()->reset(args["stop"] == "cancel");
		}
	} else if(page == "/action.html") {
		WebServerManager::getInstance()->doAction(args["id"], Util::toInt(args["act"]));
	}

	return Util::emptyString;
}

WebConnection::ContentType WebConnection::getContentTypeFromExt(const string& ext) {
	static ContentType types[11] = {
		{ ".txt", "text/plain", true },
		{ ".html", "text/html", true },
		{ ".css", "text/css", true},
		{ ".js", "application/x-javascript", true},
		{ ".svg", "image/svg+xml", true },
		{ ".svgz", "image/svg+xml", false },
		{ ".ico", "image/x-icon", false },
		{ ".gif", "image/gif", false },
		{ ".jpg", "image/jpeg", false },
		{ ".jpeg", "image/jpeg", false },
		{ ".png", "image/png", false }
	};

	for(size_t i = 0; i < 11; ++i) {
		if(strcmp(ext.c_str(), types[i].ext) == 0)
			return types[i];
	}

	// We don't really serve these
	ContentType unknown = { 0, 0, false };
	return unknown;
}

// Only compresses text data for now...
size_t WebConnection::compressResponse(const string &data, CompMethod compression, ByteArray &compressed) {
	if(request.compress == COMPRESS_NONE)
		return 0;

	size_t size = data.size();
	if(size < 128 || static_cast<size_t>(size * 1.1) < size)
		return 0;

	z_stream zs;
	memzero(&zs, sizeof(z_stream));
	bool gzip = compression == WebConnection::COMPRESS_GZIP;

	if(Z_OK != deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY))
		return 0;

	zs.next_in = (uint8_t*)&data[0];
	zs.avail_in = size;
	zs.total_in = 0;

	// make sure we have enough room
	size_t buf_size = static_cast<size_t>(size * 1.1) + 12;
	if(gzip) buf_size += 18;
	compressed.reset(new uint8_t[buf_size]);

	if(gzip) {
		time_t time = GET_TIME();

		// gzip header
		compressed[0] = 0x1f;
		compressed[1] = 0x8b;
		compressed[2] = Z_DEFLATED;
		compressed[3] = 0; // options
		compressed[4] = (time >>  0) & 0xff;
		compressed[5] = (time >>  8) & 0xff;
		compressed[6] = (time >> 16) & 0xff;
		compressed[7] = (time >> 24) & 0xff;
		compressed[8] = 0x00; // extra flags
		compressed[9] = 0x03; // UNIX

		size = 10;
		zs.next_out = (uint8_t*)&compressed[size];
		zs.avail_out = buf_size - 18;
	} else {
		size = 0;
		zs.next_out = (uint8_t*)&compressed[size];
		zs.avail_out = buf_size;
		zs.total_out = 0;
	}

	if(Z_STREAM_END != deflate(&zs, Z_FINISH)) {
		deflateEnd(&zs);
		compressed.reset();
		return 0;
	}

	size += zs.total_out;

	if(gzip) {
		// trailer
		CRC32Filter crc32;
		crc32(&data[0], data.size());

		uint8_t* c = &compressed[size];
		c[0] = (crc32.getValue() >>  0) & 0xff;
		c[1] = (crc32.getValue() >>  8) & 0xff;
		c[2] = (crc32.getValue() >> 16) & 0xff;
		c[3] = (crc32.getValue() >> 24) & 0xff;
		c[4] = (zs.total_in >>  0) & 0xff;
		c[5] = (zs.total_in >>  8) & 0xff;
		c[6] = (zs.total_in >> 16) & 0xff;
		c[7] = (zs.total_in >> 24) & 0xff;
		size += 8;
	}

	if(Z_OK != deflateEnd(&zs)) {
		compressed.reset();
		return 0;
	}

	return size;
}

void WebConnection::on(BufferedSocketListener::Line, const string& aLine) noexcept {
	if(request.type == REQ_COMPLETE) {
		if(closing) return;
		request.type = REQ_UNKNOWN;
		request.compress = COMPRESS_NONE;
		request.time = GET_TIME();
		request.modified = (time_t)-1;
		request.dataSize = 0;

		response.file.reset();
	}

	if(request.type == REQ_UNKNOWN) {
		string::size_type start = 0, end = 0;
		if(((start = aLine.find("GET ")) != string::npos) && (end = aLine.substr(start+4).find(" ")) != string::npos) {
			request.page = aLine.substr(start+4,end);
			request.type = REQ_GET;
			requestParse(request.page, request.args);
		} else if(((start = aLine.find("POST ")) != string::npos) && (end = aLine.substr(start+5).find(" ")) != string::npos) {
			request.page = aLine.substr(start+5,end);
			request.type = REQ_POST;
			requestParse(request.page, request.args);
		} else if(((start = aLine.find("HEAD ")) != string::npos) && (end = aLine.substr(start+5).find(" ")) != string::npos) {
			// This shouldn't happen, but we allow it if it does...
			request.page = aLine.substr(start+5,end);
			request.type = REQ_HEAD;
			requestParse(request.page, request.args);
		} else {
			// We don't implement any other request methods, so lets play nice and tell that...
			response.header = "HTTP/1.1 501 Not Implemented\r\n";

			string date = WebServer::getServerDate(request.time);
			if(!date.empty()) response.header += "Date: " + date  + "\r\n";
			response.header += "Server: " APPNAME "/" VERSIONSTRING_FULL "\r\n";
			response.header += "Content-Length: 0\r\n";
			response.header += "Connection: close\r\n\r\n";

			sock->write(response.header);

			closing = true;
			sock->disconnect();
		}
	} else if(Util::findSubString(aLine, "Content-Length") != string::npos) {
		request.dataSize = Util::toUInt64(aLine.substr(16, aLine.length() - 17));
	} else if(Util::findSubString(aLine, "Accept-Encoding") != string::npos) {
		string supports = aLine.substr(17, aLine.length() - 18);
		if(supports.find("gzip") != string::npos) {
			request.compress = COMPRESS_GZIP;
		} else if(supports.find("deflate") != string::npos) {
			request.compress = COMPRESS_DEFLATE;
		}
	} else if(Util::findSubString(aLine, "If-Modified-Since") != string::npos) {
		WebServer::parseServerDate(aLine.substr(19, aLine.length() - 20), request.modified);
	} else if(Util::findSubString(aLine, "Connection") != string::npos) {
		closing = aLine.substr(12, aLine.length() - 13).compare("close") == 0;
	}

	if(aLine[0] == 0x0d) {
		if(request.type == REQ_POST && request.dataSize != 0) {
			// Simplified POST request handling (assumes default urlencoded form data)
			sock->setDataMode(request.dataSize);
		} else if(request.type == REQ_GET || request.type == REQ_HEAD) {
			// Initiate the action associated with the page (in case of HEAD request we ignore)
			if(request.type != REQ_HEAD && !request.args.empty())
				processArgs(request.page, request.args);

			ContentType type = getContentTypeFromExt(Util::getFileExt(request.page));
			string date = WebServer::getServerDate(request.time);
			string lastModified = Util::emptyString;
			int64_t size = 0;

			if(type.type != 0) {
				if(strcmp(type.type, "text/html") == 0) {
					// Get the html page... 404 on an unknown page.
					if(!WebServerManager::getInstance()->isLoggedIn(sock->getIp())) {
						string self = request.page + request.args["args"];
						WebServerManager::getInstance()->getLoginPage(response, self);
					} else {
						WebServerManager::getInstance()->getPage(response, request.page, sock->getIp());
					}
					size = response.page.size();
				} else {
					WebServerManager::StaticFileInfo fi = WebServerManager::getInstance()->getStaticFile(response, request.page, type.compress);
					if(fi.size != -1) {
						lastModified = WebServer::getServerDate(fi.time);
						if(request.modified > request.time || request.modified < fi.time) {
							size = fi.size;
						} else {
							response.header = "HTTP/1.1 304 Not Modified\r\n";

							if(!date.empty()) response.header += "Date: " + date + "\r\n";
							response.header += "Server: " APPNAME "/" VERSIONSTRING_FULL "\r\n";
							if(!lastModified.empty()) response.header += "Last-Modified: " + lastModified + "\r\n";
							response.header += "\r\n";

							sock->write(response.header);

							if(closing) sock->disconnect();
							request.type = REQ_COMPLETE;
							return;
						}
					} else {
						type.ext = ".html"; type.type = "text/html"; type.compress = true;
						WebServerManager::getInstance()->getErrorPage(response, "404 Not Found");
						size = response.page.size();
					}
				}
			} else {
				// We don't allow serving this file...
				type.ext = ".html"; type.type = "text/html"; type.compress = true;
				WebServerManager::getInstance()->getErrorPage(response, "403 Forbidden");
				size = response.page.size();
			}

			if(!date.empty()) response.header += "Date: " + date + "\r\n";
			response.header += "Server: " APPNAME "/" VERSIONSTRING_FULL "\r\n";
			if(!lastModified.empty()) response.header += "Last-Modified: " + lastModified + "\r\n";
			response.header += "Content-Type: " + string(type.type) + "\r\n";

			ByteArray compressed(NULL);
			if(type.compress && (size = compressResponse(response.page, request.compress, compressed)) != 0) {
				string method = (request.compress == COMPRESS_GZIP ? "gzip" : "deflate");
				response.header += "Content-Encoding: " + method + "\r\n";
				response.header += "Content-Length: " + Util::toString(size) + "\r\n\r\n";

				sock->write(response.header);

				if(request.type != REQ_HEAD)
					sock->write((const char*)&compressed[0], static_cast<size_t>(size));
			} else {
				if(type.compress) size = response.page.size();
				response.header += "Content-Length: " + Util::toString(size) + "\r\n\r\n";
				if(response.file.get()) {
					sock->write(response.header);

					if(request.type != REQ_HEAD)
						sock->transmitFile(response.file.get());
				} else {
					sock->write(response.header);

					if(request.type != REQ_HEAD)
						sock->write(response.page);
				}
			}

			if(closing) sock->disconnect();
			request.type = REQ_COMPLETE;
		}
	}
}

void WebConnection::on(BufferedSocketListener::ModeChange) noexcept {
	dcassert(request.type == REQ_POST);

	if(!request.dataSeq.empty()) {
		getArgs(request.dataSeq, request.args);
		string resource = processArgs(request.page, request.args, request.type);
		if(!resource.empty()) {
			response.header = "HTTP/1.1 302 Found\r\n";

			response.page = "<html><head><title>HTTP/1.1 302 Found</title></head><body>";
			response.page += "<a href='" + resource + "'>New location</a>";
			response.page += " - Requested resource has moved! (" APPNAME " v" VERSIONSTRING ")";
			response.page += "</body></html>";

			size_t size = response.page.size();
			string date = WebServer::getServerDate(request.time);

			if(!date.empty()) response.header += "Date: " + date  + "\r\n";
			response.header += "Server: " APPNAME "/" VERSIONSTRING_FULL "\r\n";
			response.header += "Content-Type: text/html\r\n";
			response.header += "Location: " + resource + "\r\n";
			response.header += "Content-Length: " + Util::toString(size) + "\r\n\r\n";

			sock->write(response.header);
			sock->write(response.page);

			if(closing) sock->disconnect();
			request.type = REQ_COMPLETE;
		} else {
			// Let's take a shortcut
			request.type = REQ_GET;
			getArgs(request.args["args"], request.args, true);
			on(BufferedSocketListener::Line(), "\x0d");
		}
	}
}

void WebConnection::on(BufferedSocketListener::Failed, const string& /*aLine*/) noexcept {
	WebServerManager::getInstance()->disconnected(this);
	delete this;
}

void WebServerManager::start() {
	if(started) return;

	Lock l(cs);

	try {
		ZipFile zip(Util::getPath(Util::PATH_RESOURCES) + "WebUI.zip");
		zip.ReadFiles(staticFiles);
	} catch(const ZipFileException& e) {
		LogManager::getInstance()->message("Failed loading web server resources: " + e.getError() + "; stopping webserver...", LogManager::LOG_ERROR);
		return;
	}

	// This is unused right now...
	fire(WebServerListener::Setup());

	// Error pages (these don't really need an id or url, but nice to have)
	pages["/404.html"] = new WebPageInfo(PAGE_404, "404 Not Found");
	pages["/403.html"] = new WebPageInfo(PAGE_403, "403 Forbidden");

	// Normal pages
	pages["/index.html"] = new WebPageInfo(PAGE_INDEX, "Home");	
	pages["/dlqueue.html"] = new WebPageInfo(PAGE_DOWNLOAD_QUEUE, "Download Queue");
	pages["/dlfinished.html"] = new WebPageInfo(PAGE_DOWNLOAD_FINISHED, "Finished Downloads");
	pages["/ulqueue.html"] = new WebPageInfo(PAGE_UPLOAD_QUEUE, "Upload Queue");
	pages["/ulfinished.html"] = new WebPageInfo(PAGE_UPLOAD_FINISHED, "Finished Uploads");
	pages["/weblog.html"] = new WebPageInfo(PAGE_LOG, "Logs");
	pages["/syslog.html"] = new WebPageInfo(PAGE_SYSLOG, "System Logs");
	pages["/search.html"] = new WebPageInfo(PAGE_SEARCH, "Search");
	pages["/action.html"] = new WebPageInfo(PAGE_ACTION, "Remote Action");
	pages["/logout.html"] = new WebPageInfo(PAGE_LOGOUT, "Logout");

	// Here goes....
	server = new WebServer(CryptoManager::getInstance()->TLSOk(), static_cast<uint16_t>(SETTING(WEBSERVER_PORT)), SETTING(BIND_ADDRESS));
	started = true;
}

void WebServerManager::stop() {
	if(!started) return;

	Lock l(cs);

	started = false;
	delete server;

	sessions.clear();
	staticFiles.clear();

	for(PageMap::iterator p = pages.begin(); p != pages.end(); ++p) {
		if(p->second != NULL){
			delete p->second;
			p->second = NULL;
		}
	}
}

void WebServerManager::shutdown() {
	stop();
	{
		Lock l(cs);
		for(ConnectionMap::const_iterator j = activeConnections.begin(); j != activeConnections.end(); ++j) {
			(*j)->disconnect(true);
		}
	}
	while(true) {
		{
			Lock l(cs);
			if(activeConnections.empty()) break;
		}
		Thread::sleep(50);
	}
}

void WebServerManager::accept(const Socket& sock, bool secure) noexcept {
	WebConnection* conn = new WebConnection(secure);
	try {
		conn->accept(sock);
	} catch(const Exception&) {
		delete conn;
		return;
	}

	Lock l(cs);
	activeConnections.push_back(conn);
}

void WebServerManager::disconnected(WebConnection* conn) noexcept {
	Lock l(cs);
	activeConnections.erase(remove(activeConnections.begin(), activeConnections.end(), conn), activeConnections.end());
}

void WebServerManager::doAction(const string& group, int action) {
	if(group.compare("shutdown") == 0) {
		fire(WebServerListener::ShutdownPC(), action);
	}
}

void WebServerManager::getLoginPage(WebConnection::WebResponse& response, const string& dest /*= "/index.html"*/) {
	response.header = "HTTP/1.1 200 OK\r\n";

	response.page =  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">";
	response.page += "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en'>";
	response.page += "<head>";  
	response.page += "	<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
	response.page += "	<meta http-equiv='Cache-Control' content='no-cache, must-revalidate' />";
	response.page += "	<meta http-equiv='Expires' content='Sat, 26 Jul 1997 05:00:00 GMT' />";
	response.page += "	<title>ApexDC++ Webserver - Login Page</title>"; 
	response.page += "	<link rel='stylesheet' href='style.css' type='text/css' title='Default style' media='screen' />";
	response.page += "</head>";
	response.page += "<body>";
	response.page += "<div id='index_obsah'>";
	response.page += "<h1>ApexDC++ Webserver</h1>";
	response.page += "<div id='index_logo'></div>";
	response.page += "	<div id='login'>";
	response.page += "		<form method='post' action='login.html'>";
	response.page += "			<p><strong>Username: </strong><input type='text' name='user'  size='10'/></p>";
	response.page += "			<p><strong>Password: </strong><input type='password' name='pass' size='10' /></p>";
	response.page += "			<p><input type='hidden' name='redirect' value='" + dest + "' />";
	response.page += "			<input class='tlacitko' type='submit' value='Login' /></p>";
	response.page += "		</form>";
	response.page += "	</div>";
	response.page += "	<div id='paticka'>";
	response.page += "		2006-2010 | ApexDC++ Development Team | <a href='http://www.apexdc.net/'>ApexDC++</a>";
	response.page += "	</div>";
	response.page += "</div>";
	response.page += "</body>";
	response.page += "</html>";
}

void WebServerManager::getErrorPage(WebConnection::WebResponse& response, const string& errStr) {
	response.header = "HTTP/1.1 " + errStr + "\r\n";

	response.page =  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">";
	response.page += "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en'>";
	response.page += "<head>";  
	response.page += "	<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
	response.page += "	<meta http-equiv='Cache-Control' content='no-cache, must-revalidate' />";
	response.page += "	<meta http-equiv='Expires' content='Sat, 26 Jul 1997 05:00:00 GMT' />";
	response.page += "	<title>ApexDC++ Webserver - HTTP/1.1 " + errStr + "</title>"; 
	response.page += "	<link rel='stylesheet' href='style.css' type='text/css' title='Default style' media='screen' />";
	response.page += "</head>";
	response.page += "<body>";
	response.page += "<div id='index_obsah'>";
	response.page += "<h1>ApexDC++ Webserver</h1>";
	response.page += "<div id='index_logo'></div>";
	response.page += "	<div id='error'>";
	response.page += "		<strong style='text-align:center'>" + errStr + "!</strong>";
	response.page += "		<p>Please return to <a href='index.html'>index</a>.</p>";
	response.page += "		<p><small><strong>Note:</strong> This server is not intended for serving files!</small></p>";
	response.page += "	</div>";
	response.page += "	<div id='paticka'>";
	response.page += "		2006-2010 | ApexDC++ Development Team | <a href='http://www.apexdc.net/'>ApexDC++</a>";
	response.page += "	</div>";
	response.page += "</div>";
	response.page += "</body>";
	response.page += "</html>";
}

bool WebServerManager::getPage(WebConnection::WebResponse& response, const string& file, const string& IP) {
	Lock l(cs);

	PageMap::const_iterator itr = pages.find(file);
	WebPageInfo* page = (itr != pages.end()) ? itr->second : pages["/404.html"];

	// Handle pages that use separate templates here...
	if(page->id == PAGE_404 || page->id == PAGE_403) {
		getErrorPage(response, page->title);
		return false;
	} else if(page->id == PAGE_LOGOUT) {
		logout(IP);
		getLoginPage(response);
		return true;
	}

	response.header = "HTTP/1.1 200 OK\r\n";

	response.page =  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">";
	response.page += "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en'>";
	response.page += "<head>";
	response.page += "	<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
	response.page += "	<meta http-equiv='Cache-Control' content='no-cache, must-revalidate' />";
	response.page += "	<meta http-equiv='Expires' content='Sat, 26 Jul 1997 05:00:00 GMT' />";
	if((page->id == PAGE_SEARCH) && sentSearch)
		response.page += "	<meta http-equiv='Refresh' content='" + Util::toString((searchInterval / 1000) + 15) + ";url=search.html?stop=true' />";
	response.page += "	<title>ApexDC++ Webserver - " + page->title + "</title>";
	response.page += "	<link rel='stylesheet' href='style.css' type='text/css' title='Default style' media='screen' />";
	response.page += "</head>";
	response.page += "<body>";

	response.page += "<div id='obsah'>";
	response.page += "<h1>ApexDC++ Webserver</h1>";
	response.page += "	<div id='menu'>";
	response.page += "		<a href='weblog.html'>Webserver Log</a>";
	response.page += "		<a href='syslog.html'>System Log</a>";
	response.page += "		<a href='ulfinished.html'>Finished Uploads</a>";
	response.page += "		<a href='ulqueue.html'>Upload Queue  </a>";
	response.page += "		<a href='dlqueue.html'>Download Queue</a>";
	response.page += "		<a href='dlfinished.html'>Finished Downloads</a>";
	response.page += "		<a href='search.html'>Search</a>";
	response.page += "		<a href='logout.html'>Log out</a>";
	response.page += "	</div>";

	response.page += "	<div id='prava'>";

	switch(page->id) {
		case PAGE_INDEX: {
			response.page += "<h1>Home</h1>";
			response.page += "<br />&nbsp;Welcome to the ApexDC++ Webserver";
			break;
		}

		case PAGE_DOWNLOAD_QUEUE:
			response.page += getDLQueue();
			break;

		case PAGE_DOWNLOAD_FINISHED:
			response.page += getFinished(false);
			break;

		case PAGE_UPLOAD_QUEUE:
			response.page += getULQueue();
			break;

		case PAGE_UPLOAD_FINISHED:
			response.page += getFinished(true);
			break;

		case PAGE_SEARCH:
			response.page += getSearch();
			break;

		case PAGE_LOG:
			response.page += getLogs();
			break;

		case PAGE_SYSLOG:
			response.page += getSysLogs();
			break;

		case PAGE_ACTION: {
			response.page += "<h1>Status</h1>";
			response.page += "<br />&gt;&gt; Request sent to remote PC :)";
			break;
		}

		default: dcassert(0);
	}

	response.page += "	</div>";
	response.page += "	<div id='ikony'>";
	response.page += "			<a href='action.html?id=shutdown&amp;act=5' id='switch'></a>";
	response.page += "			<a href='action.html?id=shutdown&amp;act=1' id='logoff'></a>";
	response.page += "			<a href='action.html?id=shutdown&amp;act=3' id='suspend'></a>";
	response.page += "			<a href='action.html?id=shutdown&amp;act=2' id='reboot'></a>";
	response.page += "			<a href='action.html?id=shutdown&amp;act=0' id='shutdown'></a>";
	response.page += "	</div>";
	response.page += "	<div id='paticka'>";
	response.page += "		2006-2010 | ApexDC++ Development Team | <a href='http://www.apexdc.net/'>ApexDC++</a>";
	response.page += "	</div>";
	response.page += "</div>";

	response.page += "</body>";
	response.page += "</html>";
	return true;
}

WebServerManager::StaticFileInfo WebServerManager::getStaticFile(WebConnection::WebResponse& response, const string& file, bool compress) {
	Lock l(cs);
	response.header = "HTTP/1.1 200 OK\r\n";

	ZipFile::FileMap::iterator i;
	if((i = staticFiles.find(file.substr(1))) != staticFiles.end()) {
		size_t size = static_cast<size_t>(i->second.first.size);
		if(!size || compress) {
			response.page = string(i->second.second.get(), i->second.second.get() + size);
		} else {
			response.file = std::unique_ptr<InputStream>(new MemoryInputStream(i->second.second.get(), size));
		}
		return StaticFileInfo(i->second.first.time, size);
	}

	return StaticFileInfo(0, -1);
}

void WebServerManager::search(string searchStr, SearchManager::TypeModes ftype) {
	if (!sentSearch) {
		string::size_type i = 0;
		while ((i = searchStr.find("+", i)) != string::npos) {
			searchStr.replace(i, 1, " ");
			i++;
		}

		token = Util::toString(Util::rand());
		SearchManager::getInstance()->addListener(this);

		// Get ADC searchtype extensions if any is selected
		StringList extList;
		try {
			if (ftype == SearchManager::TYPE_ANY) {
				// Custom searchtype
				// disabled with current GUI extList = SettingsManager::getInstance()->getExtensions(Text::fromT(fileType->getText()));
			}
			else if (ftype > SearchManager::TYPE_ANY && ftype < SearchManager::TYPE_DIRECTORY) {
				// Predefined searchtype
				extList = SettingsManager::getInstance()->getExtensions(string(1, '0' + ftype));
			}
		}
		catch (const SearchTypeException&) {
			ftype = SearchManager::TYPE_ANY;
		}

		searchInterval = SearchManager::getInstance()->search(WebServerManager::getInstance()->sClients, searchStr, 0, ftype, SearchManager::SIZE_DONTCARE, token, extList, (void*)this);
		results = Util::emptyString;
		sentSearch = true;
	}
}

void WebServerManager::reset(bool cancel) {
	sentSearch = false;
	row = 0; /* Counter to permit FireFox correct item clicks */
	if (cancel) {
		results = Util::emptyString;
		ClientManager::getInstance()->cancelSearch((void*)this);
	}
	SearchManager::getInstance()->removeListener(this);
}

string WebServerManager::getLogs() {
	string ret = "<h1>Webserver Logs</h1>";
	string path = Util::validateFileName(LogManager::getInstance()->getPath(LogManager::WEBSERVER));
	
	try {
		File f(path, File::READ, File::OPEN);
		int64_t size = f.getSize();

		if(size > 32*1024)
			f.setPos(size - 32*1024);

		if(BOOLSETTING(LOG_WEBSERVER)) {
			StringList lines = StringTokenizer<string>(f.read(32*1024), "\r\n").getTokens();
			size_t linesCount = lines.size();
			size_t i = linesCount > 11 ? linesCount - 11 : 0;

			for(; i < (linesCount - 1); ++i)
				ret += "<br />&nbsp;" + lines[i];
		} else {
			ret += "<br />&nbsp;Webserver logging not enabled.";
		}

		f.close();
	} catch(FileException & e) {
		if(BOOLSETTING(LOG_WEBSERVER)) {
			ret += "<br />&nbsp;<strong>Error:</strong> " + e.getError();
		} else {
			ret += "<br />&nbsp;Webserver logging not enabled.";
		}
	}

	return ret;
}

string WebServerManager::getSysLogs() {
	string ret = "<h1>System Logs</h1>";
	string path = Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM));
		
	try {
		File f(path, File::READ, File::OPEN);
		
		int64_t size = f.getSize();

		if(size > 32*1024) {
			f.setPos(size - 32*1024);
		}

		if(BOOLSETTING(LOG_SYSTEM)) {
			StringList lines = StringTokenizer<string>(f.read(32*1024), "\r\n").getTokens();

			size_t linesCount = lines.size();

			size_t i = linesCount > 11 ? linesCount - 11 : 0;

			for(; i < (linesCount - 1); ++i){
				ret += "<br />&nbsp;" + lines[i];
			}
		} else {
			ret += "<br />&nbsp;System logging not enabled.";
		}

		f.close();
	} catch(FileException & e) {
		if(BOOLSETTING(LOG_SYSTEM)) {
			ret += "<br />&nbsp;<strong>Error:</strong> " + e.getError();
		} else {
			ret += "<br />&nbsp;System logging not enabled.";
		}
	}
	return ret;
}

string WebServerManager::getSearch() {
	string ret = "<form method='get' action='search.html'><fieldset style='border: none'>";

	ClientManager* clientMgr = ClientManager::getInstance();
	clientMgr->lock();
	const auto& clients = clientMgr->getClients();
	sClients.clear();

	for(auto it = clients.cbegin(); it != clients.cend(); ++it) {
		Client* client = it->second;
		if(!client->isConnected())
			continue;

		ret += "<br /> >>> " + client->getHubName();
		sClients.push_back(client->getHubUrl());
	}

	if(sentSearch) {
		ret += "<br /><br /><table width='100%'><tr><td align='center'><strong>Searching... Please wait.</strong></td></tr>";
		ret += "<tr><td align='center'><input class='tlacitko' type='submit' name='stop' value='cancel' />";
		ret += "</td></tr></table></fieldset></form>";
	} else {
		ret += "<br /><table width='100%'><tr><td align='center'><strong>Search for:&nbsp;</strong>";
		ret += "<input type='text' name='query' />";
		ret += "&nbsp;<input class='tlacitko' type='submit' value='Search' /></td></tr>";
		ret += "<tr><td align='center'>";
		ret += "<select name='type'>";
		ret += "<option value='0' selected='selected'>" + STRING(ANY) + "</option>";
		ret += "<option value='1'>" + STRING(AUDIO) + "</option>";
		ret += "<option value='2'>" + STRING(COMPRESSED) + "</option>";
		ret += "<option value='3'>" + STRING(DOCUMENT) + "</option>";
		ret += "<option value='4'>" + STRING(EXECUTABLE) + "</option>";
		ret += "<option value='5'>" + STRING(PICTURE) + "</option>";
		ret += "<option value='6'>" + STRING(VIDEO) + "</option>";
		ret += "<option value='7'>" + STRING(DIRECTORY) + "</option>";
		ret += "<option value='8'>TTH</option>";
		ret += "</select></td></tr></table></fieldset></form><br />";	
		if(!results.empty())
			ret += "<table width='100%' style='background-color: #EEEEEE'>" + results + "</table>";
	}

	return ret;
}

string WebServerManager::getFinished(bool uploads){
	string ret;

	const FinishedItem::List& fl = FinishedManager::getInstance()->lockList(uploads);
	ret = "	<h1>Finished ";
	ret += (uploads ? "Uploads" : "Downloads");
	ret += "</h1>";
	ret += "	<table  width='100%'>";
	ret += "		<tr class='tucne'>";
	ret += "			<td>Time</td>";
	ret += "			<td>Name</td>";
	ret += "			<td>Size</td>";
	ret += "		</tr>";
	for(FinishedItem::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		ret+="<tr>";
		ret+="	<td>" + Util::formatTime("%Y-%m-%d %H:%M:%S", (*i)->getTime()) + "</td>";
		ret+="	<td>" + Util::getFileName((*i)->getTarget()) + "</td>";
		ret+="	<td>" + Util::formatBytes((*i)->getSize()) + "</td>";			
		ret+="</tr>";
	}
	ret+="</table>";
	FinishedManager::getInstance()->unlockList();

	return ret;
}

string WebServerManager::getDLQueue(){
	string ret;

	QueueManager::getInstance()->lockedOperation([&ret](const QueueItem::StringMap& li) {
		ret = "	<h1>Download Queue</h1>";
		ret += "	<table  width='100%'>";
		ret += "		<tr class='tucne'>";
		ret += "			<td>Name</td>";
		ret += "			<td>Size</td>";
		ret += "			<td>Downloaded</td>";
		ret += "			<td>Speed</td>";
		ret += "			<td>Segments</td>";
		ret += "		</tr>";
		for(QueueItem::StringMap::const_iterator j = li.begin(); j != li.end(); ++j) {
			QueueItem* aQI = j->second;
			double percent = (aQI->getSize() > 0) ? aQI->getDownloadedBytes() * 100.0 / aQI->getSize() : 0;
			ret += "	<tr>";
			ret += "		<td>" + aQI->getTargetFileName() + "</td>";
			ret += "		<td>" + Util::formatBytes(aQI->getSize()) + "</td>";
			ret += "		<td>" + Util::formatBytes(aQI->getDownloadedBytes()) + " ("+ Util::toString(percent) + "%)</td>";
			ret += "		<td>" + Util::formatBytes(aQI->getAverageSpeed()) + "/s</td>";
			ret += "		<td>" + Util::toString((int)aQI->getDownloads().size())+"/"+Util::toString(aQI->getMaxSegments()) + "</td>";
			ret += "	</tr>";
		}
		ret+="</table>";
	});

	return ret;
}

string WebServerManager::getULQueue() {
	string ret = "";
	ret = "	<h1>Upload Queue</h1>";
	ret += "	<table  width='100%'>";
	ret += "		<tr class='tucne'>";
	ret += "			<td>User</td>";
	ret += "			<td>Filename</td>";
	ret += "		</tr>";
	UploadQueueItem::SlotQueue users = UploadManager::getInstance()->getUploadQueue();
	for(UploadQueueItem::SlotQueue::const_iterator ii = users.begin(); ii != users.end(); ++ii) {
		for(UploadQueueItem::List::const_iterator i = ii->second.begin(); i != ii->second.end(); ++i) {
			ret+="<tr><td>" + ClientManager::getInstance()->getNicks(ii->first.user)[0] + "</td>";
			ret+="<td>" + Util::getFileName((*i)->getFile()) + "</td></tr>";
		}
	}
	ret+="</table>";
	return ret;
}

void WebServerManager::on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept {
	// Check that this is really a relevant search result...
	if(!aResult->getToken().empty() && token != aResult->getToken())
		return;
		
	if(aResult->getType() == SearchResult::TYPE_FILE) {
		// There are ton of better ways todo this (markup) but it works
		results += "<form method='post' name='row" + Util::toString(row) + "' action='add_download.html'>";
		results += "<input type='hidden' name='size' value='" + Util::toString(aResult->getSize()) + "' />";
		results += "<input type='hidden' name='name' value='" + aResult->getFileName() + "' />";
		results += "<input type='hidden' name='tth' value='" + aResult->getTTH().toBase32() + "' />";
		results += "<input type='hidden' name='type' value='" + Util::toString(aResult->getType()) + "' />";
		results += "<input type='hidden' name='redirect' value='/search.html' />";
		results += "<tr onmouseover=\"this.style.backgroundColor='#316AC5'; this.style.color='#FFF'; this.style.cursor='pointer';\" onmouseout=\"this.style.backgroundColor='#EEEEEE'; this.style.color='#000';\" onclick=\"document.forms['row" + Util::toString(row) + "'].submit();\">";
		results += "<td>" + aResult->getFileName() + "</td><td>" + ClientManager::getInstance()->getNicks(aResult->getUser())[0] + "</td><td>" + Util::formatBytes(aResult->getSize()) + "</td><td>" + aResult->getTTH().toBase32() + "</td>";
		results += "</tr></form>";
		row++;
	}
}

} // namespace dcpp
