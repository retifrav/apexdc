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

#ifndef DCPLUSPLUS_DCPP_SSLSOCKET_H
#define DCPLUSPLUS_DCPP_SSLSOCKET_H

#include "noexcept.h"
#include "typedefs.h"

#include "CryptoManager.h"
#include "Socket.h"
#include "Singleton.h"

#include "ssl.h"

namespace dcpp {

using std::unique_ptr;
using std::string;

class SSLSocketException : public SocketException 
{
public:
#ifndef NDEBUG
	SSLSocketException(const string& aError) noexcept : SocketException("SSLSocketException: " + aError) { }
#else //NDEBUG
	SSLSocketException(const string& aError) noexcept : SocketException(aError) { }
#endif // NDEBUG
	SSLSocketException(int aError) noexcept : SocketException(aError) { }

	virtual ~SSLSocketException() noexcept { }
};

class SSLSocket : public Socket 
{
public:
	SSLSocket(CryptoManager::SSLContext context, bool allowUntrusted, const string& expKP);
	/** Creates an SSL socket without any verification */
	SSLSocket(CryptoManager::SSLContext context);

	~SSLSocket() { disconnect(); verifyData.reset(); }

	void accept(const Socket& listeningSocket);
	void connect(const string& aIp, uint16_t aPort);
	int read(void* aBuffer, int aBufLen);
	int write(const void* aBuffer, int aLen);
	int wait(uint64_t millis, int waitFor);
	void shutdown() noexcept;
	void close() noexcept;

	bool isSecure() const noexcept { return true; }
	bool isTrusted() noexcept;
	string getCipherName() noexcept;
	ByteVector getKeyprint() const noexcept;
	bool verifyKeyprint(const string& expKeyp, bool allowUntrusted) noexcept;

	bool waitConnected(uint64_t millis);
	bool waitAccepted(uint64_t millis);

private:

	SSL_CTX* ctx;
	ssl::SSL ssl;
	unique_ptr<CryptoManager::SSLVerifyData> verifyData;	// application data used by verify_callback

	int checkSSL(int ret);
	bool waitWant(int ret, uint64_t millis);
};

} // namespace dcpp

#endif // SSLSOCKET_H
