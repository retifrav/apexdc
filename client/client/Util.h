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

#ifndef DCPLUSPLUS_DCPP_UTIL_H
#define DCPLUSPLUS_DCPP_UTIL_H

#include "compiler.h"

#include <cstdlib>
#include <ctime>

#include <map>

#include <boost/range/algorithm/find_if.hpp>

#ifdef _WIN32

# define PATH_SEPARATOR '\\'
# define PATH_SEPARATOR_STR "\\"

#else

# define PATH_SEPARATOR '/'
# define PATH_SEPARATOR_STR "/"

#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>

# ifndef RGB
#  define RGB(r, g, b) ((b << 16) | (g << 8) | r)
# endif

#endif

#include "Text.h"
#include "GetSet.h"

namespace dcpp {

using boost::find_if;
using std::map;

#if !defined NOCASE_WCSCMP_FUNC && !defined wcscasecmp
inline int wcscasecmp( const wchar_t *s1, const wchar_t *s2 )
{
	std::wint_t a, b;
	for ( ; a = std::towlower( *s1 ), b = std::towlower( *s2 ), a && a == b; ++s1, ++s2 );
	return a - b;
}
#endif

#if !defined NOCASE_WCSNCMP_FUNC && !defined wcsncasecmp
inline int wcsncasecmp( const wchar_t *s1, const wchar_t *s2, std::size_t n )
{
	std::wint_t a = 0, b = 0;
	for ( ; n && ( a = std::towlower( *s1 ), b = std::towlower( *s2 ), a && a == b ); ++s1, ++s2, --n );
	return a - b;
}
#endif

inline int stricmp(const std::string& a, const std::string& b) { return strcasecmp(a.c_str(), b.c_str()); }
inline int strnicmp(const std::string& a, const std::string& b, size_t n) { return strncasecmp(a.c_str(), b.c_str(), n); }
inline int wcsicmp(const std::wstring& a, const std::wstring& b) { return wcscasecmp(a.c_str(), b.c_str()); }
inline int wcsnicmp(const std::wstring& a, const std::wstring& b, size_t n) { return wcsncasecmp(a.c_str(), b.c_str(), n); }

#if !defined(_MSC_VER) && !defined(__MINGW32__)
inline int stricmp(const char* a, const char* b) { return strcasecmp(a, b); }
inline int strnicmp(const char* a, const char* b, size_t n) { return strncasecmp(a, b, n); }
inline int wcsicmp(const wchar_t* a, const wchar_t* b) { return wcscasecmp(a, b); }
inline int wcsnicmp(const wchar_t* a, const wchar_t* b, size_t n) { return wcsncasecmp(a, b, n); }
#endif

#ifdef UNICODE
inline int stricmp(const std::wstring& a, const std::wstring& b) { return wcscasecmp(a.c_str(), b.c_str()); }
inline int strnicmp(const std::wstring& a, const std::wstring& b, size_t n) { return wcsncasecmp(a.c_str(), b.c_str(), n); }
inline int stricmp(const wchar_t* a, const wchar_t* b) { return wcscasecmp(a, b); }
inline int strnicmp(const wchar_t* a, const wchar_t* b, size_t n) { return wcsncasecmp(a, b, n); }
#endif

#define LIT(x) x, (sizeof(x)-1)

/** Evaluates op(pair<T1, T2>.first, compareTo) */
template<class T1, class T2, class op = std::equal_to<T1> >
class CompareFirst {
public:
	CompareFirst(const T1& compareTo) : a(compareTo) { }
	bool operator()(const pair<T1, T2>& p) { return op()(p.first, a); }
private:
	CompareFirst& operator=(const CompareFirst&);
	const T1& a;
};

/** Evaluates op(pair<T1, T2>.second, compareTo) */
template<class T1, class T2, class op = std::equal_to<T2> >
class CompareSecond {
public:
	CompareSecond(const T2& compareTo) : a(compareTo) { }
	bool operator()(const pair<T1, T2>& p) { return op()(p.second, a); }
private:
	CompareSecond& operator=(const CompareSecond&);
	const T2& a;
};

/**
 * Compares two values
 * @return -1 if v1 < v2, 0 if v1 == v2 and 1 if v1 > v2
 */
template<typename T>
int compare(const T& v1, const T& v2) {
	return (v1 < v2) ? -1 : ((v1 == v2) ? 0 : 1);
}

class Util  
{
public:
	static tstring emptyStringT;
	static string emptyString;
	static wstring emptyStringW;

	enum Paths {
		/** Global configuration */
		PATH_GLOBAL_CONFIG,
		/** Per-user configuration (queue, favorites, ...) */
		PATH_USER_CONFIG,
		/** Per-user local data (cache, temp files, ...) */
		PATH_USER_LOCAL,
		/** Various resources (help files etc) */
		PATH_RESOURCES,
		/** Translations */
		PATH_LOCALE,
		/** Default download location */
		PATH_DOWNLOADS,
		/** Default file list location */
		PATH_FILE_LISTS,
		/** Default hub list cache */
		PATH_HUB_LISTS,
		/** Where the notepad file is stored */
		PATH_NOTEPAD,
		/** Folder with emoticons packs */
		PATH_EMOPACKS,
		PATH_LAST
	};

	typedef std::map<Util::Paths, std::string> PathsMap;
	static void initialize(PathsMap pathOverrides = PathsMap());

	/** Path of temporary storage */
	static string getTempPath();

	/** Path of crash log files */
	static string getFatalLogPath();

	/** Path of configuration files */
	static const string& getPath(Paths path) { return paths[path]; }

	/** Migrate from pre-localmode config location */
	static void migrate(const string& file) noexcept;

	/** Path of file lists */
	static string getListPath() { return getPath(PATH_FILE_LISTS); }
	/** Path of hub lists */
	static string getHubListsPath() { return getPath(PATH_HUB_LISTS); }
	/** Notepad filename */
	static string getNotepadFile() { return getPath(PATH_NOTEPAD); }

	static string translateError(int aError);

	static time_t getStartTime() { return startTime; }

	static string getFilePath(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != string::npos) ? path.substr(0, i + 1) : path;
	}
	static string getFileName(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != string::npos) ? path.substr(i + 1) : path;
	}
	static string getFileExt(const string& path) {
		string::size_type i = path.rfind('.');
		return (i != string::npos) ? path.substr(i) : Util::emptyString;
	}
	static string getLastDir(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		if(i == string::npos)
			return Util::emptyString;
		string::size_type j = path.rfind(PATH_SEPARATOR, i-1);
		return (j != string::npos) ? path.substr(j+1, i-j-1) : path;
	}

	static wstring getFilePath(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != wstring::npos) ? path.substr(0, i + 1) : path;
	}
	static wstring getFileName(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != wstring::npos) ? path.substr(i + 1) : path;
	}
	static wstring getFileExt(const wstring& path) {
		wstring::size_type i = path.rfind('.');
		return (i != wstring::npos) ? path.substr(i) : Util::emptyStringW;
	}
	static wstring getLastDir(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		if(i == wstring::npos)
			return Util::emptyStringW;
		wstring::size_type j = path.rfind(PATH_SEPARATOR, i-1);
		return (j != wstring::npos) ? path.substr(j+1, i-j-1) : path;
	}

	template<typename string_t>
	static void replace(const string_t& search, const string_t& replacement, string_t& str) {
		typename string_t::size_type i = 0;
		while((i = str.find(search, i)) != string_t::npos) {
			str.replace(i, search.size(), replacement);
			i += replacement.size();
		}
	}
	template<typename string_t>
	static inline void replace(const typename string_t::value_type* search, const typename string_t::value_type* replacement, string_t& str) {
		replace(string_t(search), string_t(replacement), str);
	}

	// Some helpers to deal with differences between old and new signatures for now
	static void decodeUrl(const string& aUrl, string& protocol, string& host, uint16_t& port, string& path, string& query, string& fragment) {
		bool isSecure;
		decodeUrl(aUrl, protocol, host, port, path, query, fragment, isSecure);
	}

	static void decodeUrl(const string& aUrl, string& protocol, string& host, uint16_t& port, string& path, string& query, string& fragment, bool& isSecure) {
		string sPort = Util::toString(port);
		decodeUrl(aUrl, protocol, host, sPort, path, query, fragment, isSecure);
		port = static_cast<uint16_t>(Util::toUInt32(sPort));
	}

	static void decodeUrl(const string& aUrl, string& protocol, string& host, string& port, string& path, string& query, string& fragment) {
		bool isSecure;
		decodeUrl(aUrl, protocol, host, port, path, query, fragment, isSecure);
	}


	static void sanitizeUrl(string& url);
	static void decodeUrl(const string& aUrl, string& protocol, string& host, string& port, string& path, string& query, string& fragment, bool& isSecure);
	static map<string, string> decodeQuery(const string& query);

	static string validateFileName(string aFile);
	static string cleanPathChars(string aNick);
	static string addBrackets(const string& s);
	
	static inline string formatBytes(const string& aString) { return formatBytes(toInt64(aString)); }

	static string getShortTimeString(time_t t = time(NULL) );

	static string getTimeString(time_t _tt = time(NULL));
	static string toAdcFile(const string& file);
	static string fromAdcFile(const string& file);
	
	static string formatBytes(int64_t aBytes);
	static wstring formatBytesW(int64_t aBytes);

	static string formatExactSize(int64_t aBytes);

	static wstring formatSeconds(int64_t aSec, bool supressHours = false) {
		wchar_t buf[64];
		if (!supressHours)
			swprintf(buf, sizeof(buf) / sizeof *buf, L"%01lu:%02d:%02d", (unsigned long)(aSec / (60*60)), (int)((aSec / 60) % 60), (int)(aSec % 60));
		else
			swprintf(buf, sizeof(buf) / sizeof *buf, L"%02d:%02d", (int)(aSec / 60), (int)(aSec % 60));
		return buf;
	}

	static string formatParams(const string& msg, const StringMap& params, bool filter, const time_t t = time(NULL));
	static string formatTime(const string &msg, const time_t t);
	static string formatRegExp(const string& msg, const StringMap& params);

	static inline int64_t roundDown(int64_t size, int64_t blockSize) {
		return ((size + blockSize / 2) / blockSize) * blockSize;
	}

	static inline int64_t roundUp(int64_t size, int64_t blockSize) {
		return ((size + blockSize - 1) / blockSize) * blockSize;
	}

	static inline int roundDown(int size, int blockSize) {
		return ((size + blockSize / 2) / blockSize) * blockSize;
	}

	static inline int roundUp(int size, int blockSize) {
		return ((size + blockSize - 1) / blockSize) * blockSize;
	}

	static inline bool isNumeric(char c) {
		return (c >= '0' && c <= '9') ? true : false;
	}

	static inline bool isNumericW(wchar_t c) {
		return (c >= L'0' && c <= L'9') ? true : false;
	}

	static int64_t toInt64(const string& aString) {
#ifdef _WIN32
		return _atoi64(aString.c_str());
#else
		return strtoll(aString.c_str(), (char **)NULL, 10);
#endif
	}
	static uint64_t toUInt64(const string& aString) {
#ifdef _WIN32
		return _strtoui64(aString.c_str(), (char **)NULL, 10);
#else
		return strtoull(aString.c_str(), (char **)NULL, 10);
#endif
	}

	static int toInt(const string& aString) {
		return atoi(aString.c_str());
	}
	static uint32_t toUInt32(const string& str) {
		return toUInt32(str.c_str());
	}
	static uint32_t toUInt32(const char* c) {
#ifdef _MSC_VER
		/*
		* MSVC's atoi returns INT_MIN/INT_MAX if out-of-range; hence, a number
		* between INT_MAX and UINT_MAX can't be converted back to uint32_t.
		*/
		uint32_t ret = atoi(c);
		if(errno == ERANGE)
			return (uint32_t)_atoi64(c);
		return ret;
#else
		return (uint32_t)atoi(c);
#endif
	}

	static double toDouble(const string& aString) {
		// Work-around for atof and locales...
		lconv* lv = localeconv();
		string::size_type i = aString.find_last_of(".,");
		if(i != string::npos && aString[i] != lv->decimal_point[0]) {
			string tmp(aString);
			tmp[i] = lv->decimal_point[0];
			return atof(tmp.c_str());
		}
		return atof(aString.c_str());
	}

	static float toFloat(const string& aString) {
		return (float)toDouble(aString.c_str());
	}

	static string toString(short val) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%d", (int)val);
		return buf;
	}
	static string toString(unsigned short val) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
		return buf;
	}
	static string toString(int val) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", val);
		return buf;
	}
	static string toString(unsigned int val) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%u", val);
		return buf;
	}
	static string toString(long val) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%ld", val);
		return buf;
	}
	static string toString(unsigned long val) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%lu", val);
		return buf;
	}
	static string toString(long long val) {
		char buf[32];
		snprintf(buf, sizeof(buf), I64_FMT, val);
		return buf;
	}
	static string toString(unsigned long long val) {
		char buf[32];
		snprintf(buf, sizeof(buf), U64_FMT, val);
		return buf;
	}
	static string toString(double val) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%0.2f", val);
		return buf;
	}

	static string toString(const string& sep, const StringList& lst);
	static string toString(const StringList& lst);

	static wstring toStringW(int32_t val) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf) / sizeof *buf, L"%ld", val);
		return buf;
	}
	static wstring toStringW(uint32_t val) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf) / sizeof *buf, L"%d", val);
		return buf;
	}
	static wstring toStringW(int64_t val) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf) / sizeof *buf, WI64_FMT, val);
		return buf;
	}
	static wstring toStringW(uint64_t val) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf) / sizeof *buf, WU64_FMT, val);
		return buf;
	}
	static wstring toStringW(double val) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf) / sizeof *buf, L"%0.2f", val);
		return buf;
	}

	static string toHexEscape(char val) {
		char buf[sizeof(int)*2+1+1];
		snprintf(buf, sizeof(buf), "%%%X", val&0x0FF);
		return buf;
	}
	static char fromHexEscape(const string aString) {
		unsigned int res = 0;
		sscanf(aString.c_str(), "%X", &res);
		return static_cast<char>(res);
	}

	template<typename T>
	static T& intersect(T& t1, const T& t2) {
		for(auto i = t1.begin(); i != t1.end();) {
			if(find_if(t2, [&](const typename T::value_type &v) { return v == *i; }) == t2.end())
				i = t1.erase(i);
			else
				++i;
		}
		return t1;
	}

	static string encodeURI(const string& /*aString*/, bool reverse = false);
	static string getLocalIp();
	static bool isPrivateIp(string const& ip);
	/**
	 * Case insensitive substring search.
	 * @return First position found or string::npos
	 */
	static string::size_type findSubString(const string& aString, const string& aSubString, string::size_type start = 0) noexcept;
	static wstring::size_type findSubString(const wstring& aString, const wstring& aSubString, wstring::size_type start = 0) noexcept;

	static const string& getIpCountry(const string& IP, bool full = false);

	static bool getAway() { return away; }
	static void setAway(bool aAway);
	static string getAwayMessage(StringMap& params);

	static void setAwayMessage(const string& aMsg) { awayMsg = aMsg; }

	static uint64_t getDirSize(const string &sFullPath);
	static bool fileExists(const string &aFile);

	static uint32_t rand();
	static uint32_t rand(uint32_t high) { return rand() % high; }
	static uint32_t rand(uint32_t low, uint32_t high) { return rand(high-low) + low; }
	static double randd() { return ((double)rand()) / ((double)0xffffffff); }

	static int DefaultSort(const wchar_t* a, const wchar_t* b, bool noCase = true);
	static int DefaultSort(const char* a, const char* b, bool noCase = true);

private:
	/** In local mode, all config and temp files are kept in the same dir as the executable */
	static bool localMode;

	static string paths[PATH_LAST];

	static bool away;
	static string awayMsg;
	static time_t awayTime;
	static time_t startTime;
	
	typedef map<uint32_t, uint16_t> CountryList;
	typedef CountryList::iterator CountryIter;
	static CountryList countries;
	static StringPairList countryNames;

	static void loadBootConfig();
};
	
/** Case insensitive hash function for strings */
struct noCaseStringHash {
	size_t operator()(const string* s) const {
		return operator()(*s);
	}

	size_t operator()(const string& s) const {
		size_t x = 0;
		const char* end = s.data() + s.size();
		for(const char* str = s.data(); str < end; ) {
			wchar_t c = 0;
			int n = Text::utf8ToWc(str, c);
			if(n < 0) {
				x = x*32 - x + '_';
				str += abs(n);
			} else {
				x = x*32 - x + (size_t)Text::toLower(c);
				str += n;
			}
		}
		return x;
	}

	size_t operator()(const wstring* s) const {
		return operator()(*s);
	}
	size_t operator()(const wstring& s) const {
		size_t x = 0;
		const wchar_t* y = s.data();
		wstring::size_type j = s.size();
		for(wstring::size_type i = 0; i < j; ++i) {
			x = x*31 + (size_t)Text::toLower(y[i]);
		}
		return x;
	}

	bool operator()(const string* a, const string* b) const {
		return stricmp(*a, *b) < 0;
	}
	bool operator()(const string& a, const string& b) const {
		return stricmp(a, b) < 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return wcsicmp(*a, *b) < 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return wcsicmp(a, b) < 0;
	}
};

/** Case insensitive string comparison */
struct noCaseStringEq {
	bool operator()(const string* a, const string* b) const {
		return a == b || stricmp(*a, *b) == 0;
	}
	bool operator()(const string& a, const string& b) const {
		return stricmp(a, b) == 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return a == b || wcsicmp(*a, *b) == 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return wcsicmp(a, b) == 0;
	}
};

/** Case insensitive string ordering */
struct noCaseStringLess {
	bool operator()(const string* a, const string* b) const {
		return stricmp(*a, *b) < 0;
	}
	bool operator()(const string& a, const string& b) const {
		return stricmp(a, b) < 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return wcsicmp(*a, *b) < 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return wcsicmp(a, b) < 0;
	}
};

#ifdef UNICODE
#define formatBytesT(x) formatBytesW(x)
#define toStringT(x) toStringW(x)
#else
#define formatBytesT(x) formatBytes(x)
#define toStringT(x) toString(x)
#endif

} // namespace dcpp

#endif // !defined(UTIL_H)

/**
 * @file
 * $Id$
 */
