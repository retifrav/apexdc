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

#ifndef DCPLUSPLUS_DCPP_FORWARD_H_
#define DCPLUSPLUS_DCPP_FORWARD_H_

/** @file
 * This file contains forward declarations for the various DC++ classes
 */

#include <boost/intrusive_ptr.hpp>

namespace dcpp {

class AdcCommand;

class ADLSearch;

class BufferedSocket;

struct ChatMessage;

class CID;

class Client;

class ClientManager;

class ConnectionQueueItem;

class DirectoryItem;
typedef DirectoryItem* DirectoryItemPtr;

class Download;
typedef Download* DownloadPtr;

class FavoriteHubEntry;
typedef FavoriteHubEntry* FavoriteHubEntryPtr;

class FavoriteUser;

class File;

class FinishedItem;
typedef FinishedItem* FinishedItemPtr;

class FinishedManager;

template<class Hasher>
struct HashValue;

struct HintedUser;

class HttpConnection;

class HubEntry;

class Identity;

class InputStream;

class OnlineUser;
typedef boost::intrusive_ptr<OnlineUser> OnlineUserPtr;

class OutputStream;

class QueueItem;
typedef QueueItem* QueueItemPtr;

class RecentHubEntry;

class SearchResult;
typedef boost::intrusive_ptr<SearchResult> SearchResultPtr;

class Socket;
class SocketException;

class StringSearch;

class TigerHash;

class Transfer;

typedef HashValue<TigerHash> TTHValue;

class UnZFilter;

class Upload;
typedef Upload* UploadPtr;

class UploadQueueItem;

class User;
typedef boost::intrusive_ptr<User> UserPtr;

class UserCommand;

class UserConnection;
typedef UserConnection* UserConnectionPtr;

} // namespace dcpp

#endif /*DCPLUSPLUS_CLIENT_FORWARD_H_*/
