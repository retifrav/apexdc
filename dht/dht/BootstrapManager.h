/*
 * Copyright (C) 2009-2011 Big Muscle, http://strongdc.sf.net
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
 
#ifndef _BOOTSTRAPMANAGER_H
#define _BOOTSTRAPMANAGER_H

#include "Constants.h"
#include "KBucket.h"

#include <client/CID.h>
#include <client/HttpManagerListener.h>
#include <client/Singleton.h>

namespace dht
{

	class BootstrapManager :
		public Singleton<BootstrapManager>, private HttpManagerListener
	{
	public:
		BootstrapManager(void);
		~BootstrapManager(void);
		
		void bootstrap();
		
		void process();
		
		void addBootstrapNode(const string& ip, uint16_t udpPort, const CID& targetCID, const UDPKey& udpKey);
		
	private:
	
		CriticalSection cs;
		HttpConnection* conn;
		
		struct BootstrapNode
		{
			string		ip;
			uint16_t	udpPort;
			CID			cid;
			UDPKey		udpKey;
		};
		
		/** List of bootstrap nodes */
		deque<BootstrapNode> bootstrapNodes;

		void on(HttpManagerListener::Complete, HttpConnection* currentConn, OutputStream* stream) noexcept;
		void on(HttpManagerListener::Failed, HttpConnection* currentConn, const string& aLine) noexcept;

	};

}

#endif	// _BOOTSTRAPMANAGER_H