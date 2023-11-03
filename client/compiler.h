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

#ifndef DCPLUSPLUS_DCPP_COMPILER_H
#define DCPLUSPLUS_DCPP_COMPILER_H

#if defined(__GNUC__)
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5)
#error GCC 4.5 is required
#endif

#if __GNUC__ == 4 && __GNUC_MINOR__ <= 6
#define STD_MAP_STD_UNIQUE_PTR_BUG
#endif

#ifdef __MINGW32__
// MinGW might not have the gnu extension names for these but it does have VS names (TODO: recheck)
#define wcscasecmp _wcsicmp
#define wcsncasecmp _wcsnicmp
#endif

#elif defined(_MSC_VER)
#if _MSC_VER < 1600
#error MSVC 10 (2010) is required
#endif

#define _SECURE_SCL  0
#define _ITERATOR_DEBUG_LEVEL 0
#define _HAS_ITERATOR_DEBUGGING 0

//disable the deprecated warnings for the CRT functions.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _ATL_SECURE_NO_DEPRECATE 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#if _MSC_VER < 1900
#define strtoll _strtoi64
#define strtoull _strtoui64
#define snprintf _snprintf
#endif

#define timegm _mkgmtime
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define wcscasecmp _wcsicmp
#define wcsncasecmp _wcsnicmp

# pragma warning(disable: 4711) // function 'xxx' selected for automatic inline expansion
# pragma warning(disable: 4786) // identifier was truncated to '255' characters in the debug information
# pragma warning(disable: 4290) // C++ Exception Specification ignored
# pragma warning(disable: 4127) // constant expression
# pragma warning(disable: 4710) // function not inlined
# pragma warning(disable: 4503) // decorated name length exceeded, name was truncated
# pragma warning(disable: 4428) // universal-character-name encountered in source
# pragma warning(disable: 4201) // nonstadard extension used : nameless struct/union
#ifdef NDEBUG
# pragma warning(disable: 4100) // unreferenced formal parameter
# pragma warning(disable: 4456) // declaration hides previous local declaration
# pragma warning(disable: 4457) // declaration hides function parameter
# pragma warning(disable: 4458) // declaration hides class member
# pragma warning(disable: 4459) // declaration hides class member
#endif

#ifdef _WIN64
# pragma warning(disable: 4244) // conversion from 'xxx' to 'yyy', possible loss of data
# pragma warning(disable: 4267) // conversion from 'xxx' to 'yyy', possible loss of data
#endif

#else
#error No supported compiler found

#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#define _LL(x) x##ll
#define _ULL(x) x##ull
#define I64_FMT "%I64d"
#define U64_FMT "%I64d"
#define WI64_FMT L"%I64d"
#define WU64_FMT L"%I64d"
#elif defined(SIZEOF_LONG) && SIZEOF_LONG == 8
#define _LL(x) x##l
#define _ULL(x) x##ul
#define I64_FMT "%ld"
#define U64_FMT "%ld"
#define WI64_FMT L"%ld"
#define WU64_FMT L"%ld"
#else
#define _LL(x) x##ll
#define _ULL(x) x##ull
#define I64_FMT "%lld"
#define U64_FMT "%lld"
#define WI64_FMT L"%lld"
#define WU64_FMT L"%lld"
#endif

#ifndef _REENTRANT
# define _REENTRANT 1
#endif

#ifndef memzero
#define memzero(dest, n) std::memset(dest, 0, n)
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#endif // DCPLUSPLUS_DCPP_COMPILER_H
