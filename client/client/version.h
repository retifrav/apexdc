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

// For non versioned builds you may need to comment out the below line, see make-gitdefs scripts for versioned builds
#include "gitdefs.inc"

#define BETA 0
#define HAS_RDEX 0

#define APP_AUTH_URL "https://development.apexdc.net/auth/login_client/"
#define APP_AUTH_KEY TIGER(SETTING(CLIENTAUTH_USER) + "." + TIGER(VERSIONSTRING))

#define APPNAME "ApexDC++"
#define DCVERSIONSTRING "0.842"
#define SDCVERSIONSTRING "2.43"
#define BUILDID 2166

#ifdef GIT_TAG_LONG
# define VERSIONSTRING GIT_TAG_SHORT
# define VERSIONSTRING_FULL GIT_TAG_LONG
#else
# define GIT_REF_COMMIT "N/A"
# define VERSIONSTRING "1.x"
# define VERSIONSTRING_FULL VERSIONSTRING
# define VERSION_DATE 0
# define COMPATIBLE_VERSIONID BUILDID
#endif

#if !BETA
// SourceForge now does provide HTTPS, but not for VHOSTS, TODO: get this off of SourceForge
# define VERSION_URL "http://update.apexdc.net/version.xml"
#else
# define VERSION_URL "https://development.apexdc.net/beta.xml"
#endif

// - Added these here so Lee can update them without me, Crise
#define HOMEPAGE "https://www.apexdc.net/"
#define DISCUSS "https://forums.apexdc.net/"
#define DONATE "https://www.apexdc.net/donate/"
#define GUIDES "https://forums.apexdc.net/forum/13-guides/"

// #define GEOIPFILE "https://www.maxmind.com/download/geoip/database/GeoIPCountryCSV.zip"
#define GEOIPFILE "http://update.apexdc.net/updater/geoip.zip"

#define COMPLETEVERSIONSTRING(user)	APPNAME " " VERSIONSTRING + (!user.empty() ?  " [Login: " + user + "] " : " ") + "(" CONFIGURATION_TYPE ")"

#ifdef _WIN64
# define CONFIGURATION_TYPE "x86-64"
# define PDB_NAME "ApexDC-x64.pdb"
#else
# define CONFIGURATION_TYPE "x86-32"
# define PDB_NAME "ApexDC.pdb"
#endif

#ifdef NDEBUG
# define INST_NAME "{APEXDC-AEE8350A-B49A-4753-AB4B-E55479A48351}"
#else
# define INST_NAME "{APEXDC-AEE8350A-B49A-4753-AB4B-E55479A48350}"
#endif

/* Update the .rc file as well... */

/**
 * @file
 * $Id$
 */
