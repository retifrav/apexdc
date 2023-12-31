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
 
#include "stdafx.h"
#include "BootstrapManager.h"

#include "Constants.h"
#include "DHT.h"
#include "SearchManager.h"

#include <boost/scoped_array.hpp>

#include <client/AdcCommand.h>
#include <client/ClientManager.h>
#include <client/HttpManager.h>
#include <client/LogManager.h>
#include <client/version.h>
#include <zlib/zlib.h>

namespace dht
{

	BootstrapManager::BootstrapManager(void) : conn(nullptr)
	{
		HttpManager::getInstance()->addListener(this);
	}

	BootstrapManager::~BootstrapManager(void)
	{
		HttpManager::getInstance()->removeListener(this);
	}

	void BootstrapManager::bootstrap()
	{
		if(bootstrapNodes.empty())
		{
			LogManager::getInstance()->message("DHT bootstrapping started", LogManager::LOG_INFO);
			
			// TODO: make URL settable
			string url = BOOTSTRAP_URL "?cid=" + ClientManager::getInstance()->getMe()->getCID().toBase32() + "&encryption=1";
			
			// store only active nodes to database
			if(ClientManager::getInstance()->isActive(Util::emptyString))
			{
				url += "&u4=" + Util::toString(DHT::getInstance()->getPort());
			}

			conn = HttpManager::getInstance()->download(url, HttpManager::CallBack(), "StrongDC++ v" SDCVERSIONSTRING);
		}
	}

	#define BUFSIZE 16384
	void BootstrapManager::on(HttpManagerListener::Complete, HttpConnection* currentConn, OutputStream* stream) noexcept
	{
		if (currentConn != conn) return;
		conn = nullptr;

		auto nodesXML = static_cast<StringOutputStream*>(stream)->getString();
		if(!nodesXML.empty())
		{
			try
			{
				uLongf destLen = BUFSIZE;
				boost::scoped_array<uint8_t> destBuf;
				
				// decompress incoming packet
				int result;
				
				do
				{
					destLen *= 2;
					destBuf.reset(new uint8_t[destLen]);
					
					result = uncompress(&destBuf[0], &destLen, (Bytef*)nodesXML.data(), nodesXML.length());
				} 
				while (result == Z_BUF_ERROR);
				
				if(result != Z_OK)
				{
					// decompression error!!!
					throw Exception("Decompress error.");
				}
							
				SimpleXML remoteXml;
				remoteXml.fromXML(string((char*)&destBuf[0], destLen));
				remoteXml.stepIn();
					
				while(remoteXml.findChild("Node"))
				{
					CID cid		= CID(remoteXml.getChildAttrib("CID"));
					string i4	= remoteXml.getChildAttrib("I4");
					string u4	= remoteXml.getChildAttrib("U4");
					
					addBootstrapNode(i4, static_cast<uint16_t>(Util::toInt(u4)), cid, UDPKey());
				}

				remoteXml.stepOut();
			}
			catch(Exception& e)
			{
				LogManager::getInstance()->message("DHT bootstrap error: " + e.getError(), LogManager::LOG_ERROR);
			}
		}
	}

	void BootstrapManager::on(HttpManagerListener::Failed, HttpConnection* /*conn*/, const string& aLine) noexcept
	{
		LogManager::getInstance()->message("DHT bootstrap error: " + aLine, LogManager::LOG_ERROR);
	}
	
	void BootstrapManager::addBootstrapNode(const string& ip, uint16_t udpPort, const CID& targetCID, const UDPKey& udpKey)
	{
		BootstrapNode node = { ip, udpPort, targetCID, udpKey };
		bootstrapNodes.push_back(node);
	}
	
	void BootstrapManager::process()
	{
		Lock l(cs);
		if(!bootstrapNodes.empty())
		{
			// send bootstrap request
			AdcCommand cmd(AdcCommand::CMD_GET, AdcCommand::TYPE_UDP);
			cmd.addParam("nodes");
			cmd.addParam("dht.xml");
			
			const BootstrapNode& node = bootstrapNodes.front();

			CID key;
			// if our external IP changed from the last time, we can't encrypt packet with this key
			// this won't probably work now
			if(DHT::getInstance()->getLastExternalIP() == node.udpKey.ip)
				key = node.udpKey.key;

			DHT::getInstance()->send(cmd, node.ip, node.udpPort, node.cid, key);
			
			bootstrapNodes.pop_front();
		}
	}
	
}
