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
 
#ifndef _UDPSOCKET_H
#define _UDPSOCKET_H

#include <deque>

#include <client/forward.h>
#include <client/typedefs.h>

#include <client/CID.h>
#include <client/FastAlloc.h>
#include <client/MerkleTree.h>
#include <client/Thread.h>

namespace dht
{

	struct Packet : 
		FastAlloc<Packet>
	{
		/** Public constructor */
		Packet(const string& ip_, uint16_t port_, const std::string& data_, const CID& _targetCID, const CID& _udpKey) : 
			ip(ip_), port(port_), data(data_), targetCID(_targetCID), udpKey(_udpKey)
		{
		}
		
		/** IP where send this packet to */
		string ip;
		
		/** To which port this packet should be sent */
		uint16_t port;
		
		/** Data to sent */
		std::string data;
		
		/** CID of target node */
		CID targetCID;

		/** Key to encrypt packet */
		CID udpKey;
			
	};
	
	class UDPSocket :
		private Thread
	{
	public:
		UDPSocket(void);
		~UDPSocket(void);
		
		/** Disconnects UDP socket */
		void disconnect() noexcept;
		
		/** Starts listening to UDP socket */
		void listen();
			
		/** Returns port used to listening to UDP socket */
		uint16_t getPort() const { return port;	}
		
		/** Sends command to ip and port */
		void send(AdcCommand& cmd, const string& ip, uint16_t port, const CID& targetCID, const CID& udpKey);
				
	private:
	
		std::unique_ptr<Socket> socket;
		
		/** Indicates to stop socket thread */
		bool stop;
		
		/** Port for communicating in this network */
		uint16_t port;		
		
		/** Queue for sending packets through UDP socket */
		std::deque<Packet*> sendQueue;
		
		/** Antiflooding protection */
		uint64_t delay;

		/** Locks access to sending queue */
		CriticalSection cs;
	
#ifndef NDEBUG
		// debug constants to optimize bandwidth
		size_t sentBytes;
		size_t receivedBytes;

		size_t sentPackets;
		size_t receivedPackets;
#endif
	
		/** Thread for receiving UDP packets */
		int run();
		
		void checkIncoming();
		void checkOutgoing(uint64_t& timer);

		void compressPacket(const string& data, uint8_t* destBuf, unsigned long& destSize);
		void encryptPacket(const CID& targetCID, const CID& udpKey, uint8_t* destBuf, unsigned long& destSize);

		bool decompressPacket(uint8_t* destBuf, unsigned long& destLen, const uint8_t* buf, size_t len);
		bool decryptPacket(uint8_t* buf, int& len, const string& remoteIp, bool& isUdpKeyValid);
	};

}

#endif // _UDPSOCKET_H
