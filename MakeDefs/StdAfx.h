// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__935F4631_2FCE_4357_B2F2_20199EF53B78__INCLUDED_)
#define AFX_STDAFX_H__935F4631_2FCE_4357_B2F2_20199EF53B78__INCLUDED_

#ifdef _WIN32
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#endif // _WIN32

#ifdef NDEBUG
# define BOOST_DISABLE_ASSERTS 1
#endif

#ifndef BOOST_ALL_NO_LIB
# define BOOST_ALL_NO_LIB 1
#endif


#include <client/compiler.h>

#ifdef _WIN32
#include <client/w.h>
#define BOOST_USE_WINDOWS_H 1
#endif

#include <ctime>
#include <cstdio>
#include <cstdarg>

#include <memory>
#include <sys/types.h>

#include <string>
#include <vector>

namespace dcpp { }

using namespace dcpp;

using std::vector;
using std::string;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__935F4631_2FCE_4357_B2F2_20199EF53B78__INCLUDED_)
