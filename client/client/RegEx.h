/* 
 * Copyright (C) 2006-2011 Crise, crise<at>mail.berlios.de
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

#ifndef REG_EX_H
#define REG_EX_H

#include <boost/regex.hpp>

#include "StringTokenizer.h"

namespace dcpp {

namespace RegEx {

template<typename T>
bool match(const T& text, const T& pattern, bool ignoreCase = false) {
	if(pattern.empty())
		return false;

	try {
		boost::regex reg(pattern, ignoreCase ?  boost::regex_constants::icase : 0);
		return !reg.empty() && boost::regex_search(text, reg);
	} catch(...) { /* ... */ }

	return false;
}

template<typename T>
bool match(const T& text, const T& patternlist, const typename T::value_type delimiter, bool ignoreCase = false) {
	if(patternlist.empty())
		return false;

	StringTokenizer<T> st(patternlist, delimiter);
	for(auto i = st.getTokens().cbegin(); i != st.getTokens().cend(); ++i) {
		if(match(text, *i, ignoreCase))
			return true;
	}
	return false;
}

} // namespace RegEx

namespace Wildcard {

template<typename T>
T toRegEx(const T& expr, bool useSet) {
	if(expr.empty())
		return expr;

	T regex = expr;
	typename T::size_type i = 0;

	while((i = regex.find_first_of(useSet ? "().{}+|^$" : "[]().{}+|^$", i)) != T::npos) {
		regex.insert(i, "\\");
		i+=2;
	}

	i = 0;
	while((i = regex.find("?", i)) != T::npos) {
		if(i > 0 && regex[i-1] == '\\') {
			++i; continue;
		}

		regex.replace(i, 1, ".");
	}

	i = 0;
	while((i = regex.find("*", i)) != T::npos) {
		if(i > 0 && regex[i-1] == '\\') {
			++i; continue;
		}

		regex.replace(i, 1, ".*");
		i+=2;
	}

	return "^" + regex + "$";
}

template<typename T>
bool match(const T& text, const T& pattern, bool useSet = true, bool ignoreCase = false) {
	return RegEx::match(text, toRegEx(pattern, useSet), ignoreCase);
}

template<typename T>
bool match(const T& text, const T& patternlist, const typename T::value_type delimiter, bool useSet = true, bool ignoreCase = false) {
	if(patternlist.empty())
		return false;

	StringTokenizer<T> st(patternlist, delimiter);
	for(auto i = st.getTokens().cbegin(); i != st.getTokens().cend(); ++i) {
		if(match(text, *i, useSet, ignoreCase))
			return true;
	}
	return false;
}

} // namespace Wildcard

} // namespace dcpp

#endif // REG_EX_H
