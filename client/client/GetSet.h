/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_GET_SET_H
#define DCPLUSPLUS_DCPP_GET_SET_H

/* adds a private member variable to a class and provides public get & set member functions to
access it. */

#include <type_traits>

#define GETSET(t, name, name2) \
private: t name; \
public: std::conditional<std::is_class<t>::value, const t&, t>::type get##name2() const { return name; } \
	void set##name2(std::conditional<std::is_class<t>::value, const t&, t>::type a##name2) { this->name = a##name2; }

#endif /* GETSET_H_ */
