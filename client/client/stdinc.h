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

#ifndef DCPLUSPLUS_DCPP_STDINC_H
#define DCPLUSPLUS_DCPP_STDINC_H

#include "compiler.h"

#ifndef DCAPI_HOST
# define DCAPI_HOST 1
#endif

#ifdef NDEBUG
# define BOOST_DISABLE_ASSERTS 1
#endif

#ifndef BOOST_ALL_NO_LIB
# define BOOST_ALL_NO_LIB 1
#endif

#ifndef BZ_NO_STDIO
# define BZ_NO_STDIO 1
#endif

#ifdef HAS_PCH

#ifdef _WIN32
#include "w.h"
#define BOOST_USE_WINDOWS_H 1
#else
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#define BOOST_PTHREAD_HAS_MUTEXATTR_SETTYPE 1
#endif

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/regex.hpp>
#include <boost/scoped_array.hpp>
#include <boost/smart_ptr/detail/atomic_count.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/variant.hpp>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <bzip2/bzlib.h>

#include <openssl/ssl.h>

#endif

namespace dcpp { }

#ifdef _DEBUG
   #ifndef DBG_NEW
	  #define BOOST_NO_RVALUE_REFERENCES
      #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
   #endif
#endif  // _DEBUG

#endif // !defined(STDINC_H)

/**
 * @file
 * $Id$
 */
