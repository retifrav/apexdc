/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "Resource.h"

#define COMPILE_MULTIMON_STUBS 1
#include <MultiMon.h>
#include <psapi.h>
#include <powrprof.h>

#include "WinUtil.h"
#include "PrivateFrame.h"
#include "SearchFrm.h"
#include "LineDlg.h"
#include "MainFrm.h"

#include "../client/Util.h"
#include "../client/StringTokenizer.h"
#include "../client/ShareManager.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/FavoriteManager.h"
#include "../client/ResourceManager.h"
#include "../client/QueueManager.h"
#include "../client/UploadManager.h"
#include "../client/HashManager.h"
#include "../client/LogManager.h"
#include "../client/Magnet.h"
#include "../client/version.h"

#include "HubFrame.h"
#include "MagnetDlg.h"
#include "winamp.h"
#include "BarShader.h"
#include "ResourceLoader.h"

#include <netfw.h>

WinUtil::ImageMap WinUtil::fileIndexes;
int WinUtil::fileImageCount;
HBRUSH WinUtil::bgBrush = NULL;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
HFONT WinUtil::font = NULL;
int WinUtil::fontHeight = 0;
HFONT WinUtil::boldFont = NULL;
HFONT WinUtil::systemFont = NULL;
HFONT WinUtil::smallBoldFont = NULL;
CMenu WinUtil::mainMenu;
OMenu WinUtil::grantMenu;
CImageList WinUtil::fileImages;
CImageList WinUtil::userImages;
CImageList WinUtil::flagImages;
int WinUtil::dirIconIndex = 0;
int WinUtil::dirMaskedIndex = 0;
TStringList WinUtil::lastDirs;
HWND WinUtil::mainWnd = NULL;
HWND WinUtil::mdiClient = NULL;
FlatTabCtrl* WinUtil::tabCtrl = NULL;
HHOOK WinUtil::hook = NULL;
tstring WinUtil::tth;
tstring WinUtil::exceptioninfo;
bool WinUtil::urlDcADCRegistered = false;
bool WinUtil::urlMagnetRegistered = false;
bool WinUtil::isAppActive = false;
DWORD WinUtil::comCtlVersion = 0;
OSVERSIONINFOEX WinUtil::osvi = {0};
CHARFORMAT2 WinUtil::m_TextStyleTimestamp;
CHARFORMAT2 WinUtil::m_ChatTextGeneral;
CHARFORMAT2 WinUtil::m_TextStyleMyNick;
CHARFORMAT2 WinUtil::m_ChatTextMyOwn;
CHARFORMAT2 WinUtil::m_ChatTextServer;
CHARFORMAT2 WinUtil::m_ChatTextSystem;
CHARFORMAT2 WinUtil::m_TextStyleBold;
CHARFORMAT2 WinUtil::m_TextStyleFavUsers;
CHARFORMAT2 WinUtil::m_TextStyleOPs;
CHARFORMAT2 WinUtil::m_TextStyleURL;
CHARFORMAT2 WinUtil::m_ChatTextPrivate;
CHARFORMAT2 WinUtil::m_ChatTextLog;

static const char* countryNames[] = { "ANDORRA", "UNITED ARAB EMIRATES", "AFGHANISTAN", "ANTIGUA AND BARBUDA", 
"ANGUILLA", "ALBANIA", "ARMENIA", "NETHERLANDS ANTILLES", "ANGOLA", "ANTARCTICA", "ARGENTINA", "AMERICAN SAMOA", 
"AUSTRIA", "AUSTRALIA", "ARUBA", "ALAND", "AZERBAIJAN", "BOSNIA AND HERZEGOVINA", "BARBADOS", "BANGLADESH", 
"BELGIUM", "BURKINA FASO", "BULGARIA", "BAHRAIN", "BURUNDI", "BENIN", "BERMUDA", "BRUNEI DARUSSALAM", "BOLIVIA", 
"BRAZIL", "BAHAMAS", "BHUTAN", "BOUVET ISLAND", "BOTSWANA", "BELARUS", "BELIZE", "CANADA", "COCOS ISLANDS", 
"THE DEMOCRATIC REPUBLIC OF THE CONGO", "CENTRAL AFRICAN REPUBLIC", "CONGO", "SWITZERLAND", "COTE D'IVOIRE", "COOK ISLANDS", 
"CHILE", "CAMEROON", "CHINA", "COLOMBIA", "COSTA RICA", "SERBIA AND MONTENEGRO", "CUBA", "CAPE VERDE", 
"CHRISTMAS ISLAND", "CYPRUS", "CZECH REPUBLIC", "GERMANY", "DJIBOUTI", "DENMARK", "DOMINICA", "DOMINICAN REPUBLIC", 
"ALGERIA", "ECUADOR", "ESTONIA", "EGYPT", "WESTERN SAHARA", "ERITREA", "SPAIN", "ETHIOPIA", "EUROPEAN UNION", "FINLAND", "FIJI", 
"FALKLAND ISLANDS", "MICRONESIA", "FAROE ISLANDS", "FRANCE", "GABON", "UNITED KINGDOM", "GRENADA", "GEORGIA", 
"FRENCH GUIANA", "GUERNSEY", "GHANA", "GIBRALTAR", "GREENLAND", "GAMBIA", "GUINEA", "GUADELOUPE", "EQUATORIAL GUINEA", 
"GREECE", "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS", "GUATEMALA", "GUAM", "GUINEA-BISSAU", "GUYANA", 
"HONG KONG", "HEARD ISLAND AND MCDONALD ISLANDS", "HONDURAS", "CROATIA", "HAITI", "HUNGARY", 
"INDONESIA", "IRELAND", "ISRAEL",  "ISLE OF MAN", "INDIA", "BRITISH INDIAN OCEAN TERRITORY", "IRAQ", "IRAN", "ICELAND", 
"ITALY","JERSEY", "JAMAICA", "JORDAN", "JAPAN", "KENYA", "KYRGYZSTAN", "CAMBODIA", "KIRIBATI", "COMOROS", 
"SAINT KITTS AND NEVIS", "DEMOCRATIC PEOPLE'S REPUBLIC OF KOREA", "SOUTH KOREA", "KUWAIT", "CAYMAN ISLANDS", 
"KAZAKHSTAN", "LAO PEOPLE'S DEMOCRATIC REPUBLIC", "LEBANON", "SAINT LUCIA", "LIECHTENSTEIN", "SRI LANKA", 
"LIBERIA", "LESOTHO", "LITHUANIA", "LUXEMBOURG", "LATVIA", "LIBYAN ARAB JAMAHIRIYA", "MOROCCO", "MONACO", 
"MOLDOVA", "MONTENEGRO", "MADAGASCAR", "MARSHALL ISLANDS", "MACEDONIA", "MALI", "MYANMAR", "MONGOLIA", "MACAO", 
"NORTHERN MARIANA ISLANDS", "MARTINIQUE", "MAURITANIA", "MONTSERRAT", "MALTA", "MAURITIUS", "MALDIVES", 
"MALAWI", "MEXICO", "MALAYSIA", "MOZAMBIQUE", "NAMIBIA", "NEW CALEDONIA", "NIGER", "NORFOLK ISLAND", 
"NIGERIA", "NICARAGUA", "NETHERLANDS", "NORWAY", "NEPAL", "NAURU", "NIUE", "NEW ZEALAND", "OMAN", "PANAMA", 
"PERU", "FRENCH POLYNESIA", "PAPUA NEW GUINEA", "PHILIPPINES", "PAKISTAN", "POLAND", "SAINT PIERRE AND MIQUELON", 
"PITCAIRN", "PUERTO RICO", "PALESTINIAN TERRITORY", "PORTUGAL", "PALAU", "PARAGUAY", "QATAR", "REUNION", 
"ROMANIA", "SERBIA", "RUSSIAN FEDERATION", "RWANDA", "SAUDI ARABIA", "SOLOMON ISLANDS", "SEYCHELLES", "SUDAN", 
"SWEDEN", "SINGAPORE", "SAINT HELENA", "SLOVENIA", "SVALBARD AND JAN MAYEN", "SLOVAKIA", "SIERRA LEONE", 
"SAN MARINO", "SENEGAL", "SOMALIA", "SURINAME", "SAO TOME AND PRINCIPE", "EL SALVADOR", "SYRIAN ARAB REPUBLIC", 
"SWAZILAND", "TURKS AND CAICOS ISLANDS", "CHAD", "FRENCH SOUTHERN TERRITORIES", "TOGO", "THAILAND", "TAJIKISTAN", 
"TOKELAU", "TIMOR-LESTE", "TURKMENISTAN", "TUNISIA", "TONGA", "TURKEY", "TRINIDAD AND TOBAGO", "TUVALU", "TAIWAN", 
"TANZANIA", "UKRAINE", "UGANDA", "UNITED STATES MINOR OUTLYING ISLANDS", "UNITED STATES", "URUGUAY", "UZBEKISTAN", 
"VATICAN", "SAINT VINCENT AND THE GRENADINES", "VENEZUELA", "BRITISH VIRGIN ISLANDS", "U.S. VIRGIN ISLANDS", 
"VIET NAM", "VANUATU", "WALLIS AND FUTUNA", "SAMOA", "YEMEN", "MAYOTTE", "YUGOSLAVIA", "SOUTH AFRICA", "ZAMBIA", 
"ZIMBABWE" };

static const char* countryCodes[] = { 
 "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AN", "AO", "AQ", "AR", "AS", "AT", "AU", "AW", "AX", "AZ", "BA", "BB", 
 "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BM", "BN", "BO", "BR", "BS", "BT", "BV", "BW", "BY", "BZ", "CA", "CC", 
 "CD", "CF", "CG", "CH", "CI", "CK", "CL", "CM", "CN", "CO", "CR", "CS", "CU", "CV", "CX", "CY", "CZ", "DE", "DJ", 
 "DK", "DM", "DO", "DZ", "EC", "EE", "EG", "EH", "ER", "ES", "ET", "EU", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", 
 "GB", "GD", "GE", "GF", "GG", "GH", "GI", "GL", "GM", "GN", "GP", "GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", 
 "HM", "HN", "HR", "HT", "HU", "ID", "IE", "IL", "IM", "IN", "IO", "IQ", "IR", "IS", "IT", "JE", "JM", "JO", "JP", 
 "KE", "KG", "KH", "KI", "KM", "KN", "KP", "KR", "KW", "KY", "KZ", "LA", "LB", "LC", "LI", "LK", "LR", "LS", "LT", 
 "LU", "LV", "LY", "MA", "MC", "MD", "ME", "MG", "MH", "MK", "ML", "MM", "MN", "MO", "MP", "MQ", "MR", "MS", "MT", 
 "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC", "NE", "NF", "NG", "NI", "NL", "NO", "NP", "NR", "NU", "NZ", "OM", 
 "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", "PN", "PR", "PS", "PT", "PW", "PY", "QA", "RE", "RO", "RS", "RU", 
 "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", "SI", "SJ", "SK", "SL", "SM", "SN", "SO", "SR", "ST", "SV", "SY", 
 "SZ", "TC", "TD", "TF", "TG", "TH", "TJ", "TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", 
 "UM", "US", "UY", "UZ", "VA", "VC", "VE", "VG", "VI", "VN", "VU", "WF", "WS", "YE", "YT", "YU", "ZA", "ZM", "ZW" };
	
HLSCOLOR RGB2HLS (COLORREF rgb) {
	unsigned char minval = min(GetRValue(rgb), min(GetGValue(rgb), GetBValue(rgb)));
	unsigned char maxval = max(GetRValue(rgb), max(GetGValue(rgb), GetBValue(rgb)));
	float mdiff  = float(maxval) - float(minval);
	float msum   = float(maxval) + float(minval);

	float luminance = msum / 510.0f;
	float saturation = 0.0f;
	float hue = 0.0f; 

	if ( maxval != minval ) { 
		float rnorm = (maxval - GetRValue(rgb) ) / mdiff;      
		float gnorm = (maxval - GetGValue(rgb) ) / mdiff;
		float bnorm = (maxval - GetBValue(rgb) ) / mdiff;   

		saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

		if (GetRValue(rgb) == maxval) hue = 60.0f * (6.0f + bnorm - gnorm);
		if (GetGValue(rgb) == maxval) hue = 60.0f * (2.0f + rnorm - bnorm);
		if (GetBValue(rgb) == maxval) hue = 60.0f * (4.0f + gnorm - rnorm);
		if (hue > 360.0f) hue = hue - 360.0f;
	}
	return HLS ((hue*255)/360, luminance*255, saturation*255);
}

static BYTE _ToRGB (float rm1, float rm2, float rh) {
	if      (rh > 360.0f) rh -= 360.0f;
	else if (rh <   0.0f) rh += 360.0f;

	if      (rh <  60.0f) rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;   
	else if (rh < 180.0f) rm1 = rm2;
	else if (rh < 240.0f) rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;      

	return (BYTE)(rm1 * 255);
}

COLORREF HLS2RGB (HLSCOLOR hls) {
	float hue        = ((int)HLS_H(hls)*360)/255.0f;
	float luminance  = HLS_L(hls)/255.0f;
	float saturation = HLS_S(hls)/255.0f;

	if ( saturation == 0.0f ) {
		return RGB (HLS_L(hls), HLS_L(hls), HLS_L(hls));
	}
	float rm1, rm2;

	if ( luminance <= 0.5f ) rm2 = luminance + luminance * saturation;  
	else                     rm2 = luminance + saturation - luminance * saturation;
	rm1 = 2.0f * luminance - rm2;   
	BYTE red   = _ToRGB (rm1, rm2, hue + 120.0f);   
	BYTE green = _ToRGB (rm1, rm2, hue);
	BYTE blue  = _ToRGB (rm1, rm2, hue - 120.0f);

	return RGB (red, green, blue);
}

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S) {
	HLSCOLOR hls = RGB2HLS (rgb);
	BYTE h = HLS_H(hls);
	BYTE l = HLS_L(hls);
	BYTE s = HLS_S(hls);

	if ( percent_L > 0 ) {
		l = BYTE(l + ((255 - l) * percent_L) / 100);
	} else if ( percent_L < 0 )	{
		l = BYTE((l * (100+percent_L)) / 100);
	}
	if ( percent_S > 0 ) {
		s = BYTE(s + ((255 - s) * percent_S) / 100);
	} else if ( percent_S < 0 ) {
		s = BYTE((s * (100+percent_S)) / 100);
	}
	return HLS2RGB (HLS(h, l, s));
}

void UserInfoBase::matchQueue(const string& hubHint) {
	if(getUser()) {
		try {
			QueueManager::getInstance()->addList(HintedUser(getUser(), hubHint), QueueItem::FLAG_MATCH_QUEUE);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);
		}
	}
}

void UserInfoBase::doReport(const string& hubHint) {
	if(getUser()) {
		ClientManager::getInstance()->reportUser(HintedUser(getUser(), hubHint));
	}
}

void UserInfoBase::getList(const string& hubHint) {
	if(getUser()) {
		try {
			QueueManager::getInstance()->addList(HintedUser(getUser(), hubHint), QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);		
		}
	}
}
void UserInfoBase::browseList(const string& hubHint) {
	if(!getUser() || getUser()->getCID().isZero())
		return;
	try {
		QueueManager::getInstance()->addList(HintedUser(getUser(), hubHint), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);		
	}
}
void UserInfoBase::checkList(const string& hubHint) {
	if(getUser()) {
		try {
			QueueManager::getInstance()->addList(HintedUser(getUser(), hubHint), QueueItem::FLAG_USER_CHECK);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);		
		}
	}
}
void UserInfoBase::addFav() {
	if(getUser()) {
		FavoriteManager::getInstance()->addFavoriteUser(getUser());
	}
}
void UserInfoBase::pm(const string& hubHint) {
	if(getUser()) {
		PrivateFrame::openWindow(HintedUser(getUser(), hubHint), Util::emptyStringT, NULL);
	}
}
void UserInfoBase::connectFav() {
	if(getUser()) {
		string url = FavoriteManager::getInstance()->getUserURL(getUser());
		if(!url.empty()) {
			HubFrame::openWindow(Text::toT(url));
		}
	}
}
void UserInfoBase::grant(const string& hubHint) {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), hubHint), 600);
	}
}
void UserInfoBase::removeAll() {
	if(getUser()) {
		QueueManager::getInstance()->removeSource(getUser(), QueueItem::Source::FLAG_REMOVED);
	}
}
void UserInfoBase::grantHour(const string& hubHint) {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), hubHint), 3600);
	}
}
void UserInfoBase::grantDay(const string& hubHint) {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), hubHint), 24*3600);
	}
}
void UserInfoBase::grantWeek(const string& hubHint) {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), hubHint), 7*24*3600);
	}
}
void UserInfoBase::ungrant() {
	if(getUser()) {
		UploadManager::getInstance()->unreserveSlot(getUser());
	}
}

bool WinUtil::getVersionInfo(OSVERSIONINFOEX& ver) {
	// version can't change during process lifetime
	if(osvi.dwOSVersionInfoSize != 0) {
		ver = osvi;
		return true;
	}

	memzero(&ver, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
			return false;
		}
	}

	if(&ver != &osvi) osvi = ver;
	return true;
}

static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	if(code == HC_ACTION) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			if(lParam & 0x80000000) {
				WinUtil::tabCtrl->endSwitch();
			} else {
				WinUtil::tabCtrl->startSwitch();
			}
		}
	}
	return CallNextHookEx(WinUtil::hook, code, wParam, lParam);
}

void WinUtil::reLoadImages(){
	userImages.Destroy();

	if(SETTING(USERLIST_IMAGE) == "") {
		ResourceLoader::LoadImageList(IDR_USERS, userImages, 16, 16);
	} else {
		ResourceLoader::LoadImageList(Text::toT(SETTING(USERLIST_IMAGE)).c_str(), userImages, 16, 16);
	}
}

void WinUtil::init(HWND hWnd) {
	mainWnd = hWnd;

	mainMenu.CreateMenu();

	CMenuHandle file;
	file.CreatePopupMenu();

	file.AppendMenu(MF_STRING, IDC_OPEN_FILE_LIST, CTSTRING(MENU_OPEN_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_OPEN_MY_LIST, CTSTRING(MENU_OPEN_OWN_LIST));
	file.AppendMenu(MF_STRING, IDC_MATCH_ALL, CTSTRING(MENU_OPEN_MATCH_ALL));
	file.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_FILE_QUICK_CONNECT, CTSTRING(MENU_QUICK_CONNECT));
	file.AppendMenu(MF_STRING, IDC_FOLLOW, CTSTRING(MENU_FOLLOW_REDIRECT));
	file.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_FILE_SETTINGS, CTSTRING(MENU_SETTINGS));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)file, CTSTRING(MENU_FILE));

	CMenuHandle view;
	view.CreatePopupMenu();

	view.AppendMenu(MF_STRING, ID_FILE_CONNECT, CTSTRING(MENU_PUBLIC_HUBS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_RECENTS, CTSTRING(MENU_FILE_RECENT_HUBS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_FAVORITES, CTSTRING(MENU_FAVORITE_HUBS));
	view.AppendMenu(MF_STRING, IDC_FAVUSERS, CTSTRING(MENU_FAVORITE_USERS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, ID_FILE_SEARCH, CTSTRING(MENU_SEARCH));
	view.AppendMenu(MF_STRING, IDC_FILE_ADL_SEARCH, CTSTRING(MENU_ADL_SEARCH));
	view.AppendMenu(MF_STRING, IDC_SEARCH_SPY, CTSTRING(MENU_SEARCH_SPY));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_CDMDEBUG_WINDOW, CTSTRING(MENU_CDMDEBUG_MESSAGES));
	view.AppendMenu(MF_STRING, IDC_NOTEPAD, CTSTRING(MENU_NOTEPAD));
	view.AppendMenu(MF_STRING, IDC_HASH_PROGRESS, CTSTRING(MENU_HASH_PROGRESS));
	view.AppendMenu(MF_STRING, IDC_SYSTEM_LOG, CTSTRING(SYSTEM_LOG));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_TOPMOST, CTSTRING(MENU_TOPMOST));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, CTSTRING(MENU_TOOLBAR));
	view.AppendMenu(MF_STRING, ID_TOGGLE_QSEARCH, CTSTRING(TOGGLE_QSEARCH));	
	view.AppendMenu(MF_STRING, ID_VIEW_STATUS_BAR, CTSTRING(MENU_STATUS_BAR));
	view.AppendMenu(MF_STRING, ID_VIEW_TRANSFER_VIEW, CTSTRING(MENU_TRANSFER_VIEW));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)view, CTSTRING(MENU_VIEW));

	CMenuHandle transfers;
	transfers.CreatePopupMenu();

	transfers.AppendMenu(MF_STRING, IDC_QUEUE, CTSTRING(MENU_DOWNLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED, CTSTRING(FINISHED_DOWNLOADS));
	transfers.AppendMenu(MF_SEPARATOR);
	transfers.AppendMenu(MF_STRING, IDC_UPLOAD_QUEUE, CTSTRING(UPLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED_UL, CTSTRING(FINISHED_UPLOADS));
	transfers.AppendMenu(MF_SEPARATOR);
	transfers.AppendMenu(MF_STRING, IDC_NET_STATS, CTSTRING(MENU_NETWORK_STATISTICS));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)transfers, CTSTRING(MENU_TRANSFERS));

	CMenuHandle window;
	window.CreatePopupMenu();

	window.AppendMenu(MF_STRING, ID_WINDOW_CASCADE, CTSTRING(MENU_CASCADE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_HORZ, CTSTRING(MENU_HORIZONTAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_VERT, CTSTRING(MENU_VERTICAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_ARRANGE, CTSTRING(MENU_ARRANGE));
	window.AppendMenu(MF_STRING, ID_WINDOW_MINIMIZE_ALL, CTSTRING(MENU_MINIMIZE_ALL));
	window.AppendMenu(MF_STRING, ID_WINDOW_RESTORE_ALL, CTSTRING(MENU_RESTORE_ALL));
	window.AppendMenu(MF_SEPARATOR);
	window.AppendMenu(MF_STRING, IDC_CLOSE_DISCONNECTED, CTSTRING(MENU_CLOSE_DISCONNECTED));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_HUBS, CTSTRING(MENU_CLOSE_ALL_HUBS));
	window.AppendMenu(MF_STRING, IDC_CLOSE_HUBS_NO_USR, CTSTRING(MENU_CLOSE_HUBS_EMPTY));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_PM, CTSTRING(MENU_CLOSE_ALL_PM));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_OFFLINE_PM, CTSTRING(MENU_CLOSE_ALL_OFFLINE_PM));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_DIR_LIST, CTSTRING(MENU_CLOSE_ALL_DIR_LIST));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_SEARCH_FRAME, CTSTRING(MENU_CLOSE_ALL_SEARCHFRAME));
	window.AppendMenu(MF_STRING, IDC_RECONNECT_DISCONNECTED, CTSTRING(MENU_RECONNECT_DISCONNECTED));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)window, CTSTRING(MENU_WINDOW));

	CMenuHandle tools;
	tools.CreatePopupMenu();

	tools.AppendMenu(MF_STRING, ID_APP_INSTALL, CTSTRING(MENU_TOOLS_INSTALL));
	tools.AppendMenu(MF_SEPARATOR);
	tools.AppendMenu(MF_STRING, ID_APP_GEOFILE, CTSTRING(MENU_TOOLS_GEOFILE));
	tools.AppendMenu(MF_STRING, ID_GET_HASH, CTSTRING(MENU_HASH));
	tools.AppendMenu(MF_STRING, IDC_UPDATE, CTSTRING(UPDATE_CHECK));
#if BETA
	tools.AppendMenu(MF_SEPARATOR);
	tools.AppendMenu(MF_STRING, ID_APP_LOGIN, CTSTRING(MENU_TOOLS_LOGIN));
#endif

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)tools, CTSTRING(MENU_TOOLS));

	CMenuHandle help;
	help.CreatePopupMenu();

	help.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	help.AppendMenu(MF_SEPARATOR);
	help.AppendMenu(MF_STRING, IDC_HELP_HOMEPAGE, CTSTRING(MENU_HOMEPAGE));
	help.AppendMenu(MF_STRING, IDC_HELP_DISCUSS, CTSTRING(MENU_DISCUSS));
	help.AppendMenu(MF_STRING, IDC_HELP_DONATE, CTSTRING(MENU_DONATE));
	help.AppendMenu(MF_STRING, IDC_GUIDES, CTSTRING(MENU_SITES_GUIDES));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)help, CTSTRING(MENU_HELP));

	ResourceLoader::LoadImageList(IDR_FOLDERS, fileImages, 16, 16);
	dirIconIndex = fileImageCount++;
	dirMaskedIndex = fileImageCount++;
	fileImageCount++;

	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		memzero(&fi, sizeof(SHFILEINFO));
		if(::SHGetFileInfo(_T("./"), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
			fileImages.ReplaceIcon(dirIconIndex, fi.hIcon);

			{
				HICON overlayIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_EXEC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

				CImageList tmpIcons;
				tmpIcons.Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 1);
				tmpIcons.AddIcon(fi.hIcon);
				tmpIcons.AddIcon(overlayIcon);

				CImageList mergeList(ImageList_Merge(tmpIcons, 0, tmpIcons, 1, 0, 0));
				HICON icDirIcon = mergeList.GetIcon(0);
				fileImages.ReplaceIcon(dirMaskedIndex, icDirIcon);

				mergeList.Destroy();
				tmpIcons.Destroy();

				::DestroyIcon(icDirIcon);
				::DestroyIcon(overlayIcon);
			}

			
			::DestroyIcon(fi.hIcon);
		}
	}

	ResourceLoader::LoadImageList(IDR_FLAGS, flagImages, 25, 15);

	if(SETTING(USERLIST_IMAGE).empty()) {
		ResourceLoader::LoadImageList(IDR_USERS, userImages, 16, 16);
	} else {
		ResourceLoader::LoadImageList(Text::toT(SETTING(USERLIST_IMAGE)).c_str(), userImages, 16, 16);
	}
	
	LOGFONT lf, lf2;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_FONT, Text::fromT(encodeFont(lf)));
	decodeFont(Text::toT(SETTING(TEXT_FONT)), lf);
	::GetObject((HFONT)GetStockObject(ANSI_FIXED_FONT), sizeof(lf2), &lf2);
	
	lf2.lfHeight = lf.lfHeight;
	lf2.lfWeight = lf.lfWeight;
	lf2.lfItalic = lf.lfItalic;

	bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);
	font = ::CreateFontIndirect(&lf);
	fontHeight = WinUtil::getTextHeight(mainWnd, font);
	lf.lfWeight = FW_BOLD;
	boldFont = ::CreateFontIndirect(&lf);
	lf.lfHeight *= 5;
	lf.lfHeight /= 6;
	smallBoldFont = ::CreateFontIndirect(&lf);
	systemFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

	if(BOOLSETTING(URL_HANDLER)) {
		registerDchubHandler();
		registerADChubHandler();
		urlDcADCRegistered = true;
	}
	if(BOOLSETTING(MAGNET_REGISTER)) {
		registerMagnetHandler();
		urlMagnetRegistered = true; 
	}

	DWORD dwMajor = 0, dwMinor = 0;
	if(SUCCEEDED(ATL::AtlGetCommCtrlVersion(&dwMajor, &dwMinor))) {
		comCtlVersion = MAKELONG(dwMinor, dwMajor);
	}
	
	hook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardProc, NULL, GetCurrentThreadId());
	
	grantMenu.CreatePopupMenu();
	grantMenu.InsertSeparatorFirst(CTSTRING(GRANT_SLOTS_MENU));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));

	initColors();
}

void WinUtil::initColors() {
	bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);

	CHARFORMAT2 cf;
	memzero(&cf, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(cf);
	cf.dwReserved = 0;
	cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD | CFM_ITALIC;
	cf.dwEffects = 0;
	cf.crBackColor = SETTING(BACKGROUND_COLOR);
	cf.crTextColor = SETTING(TEXT_COLOR);

	m_TextStyleTimestamp = cf;
	m_TextStyleTimestamp.crBackColor = SETTING(TEXT_TIMESTAMP_BACK_COLOR);
	m_TextStyleTimestamp.crTextColor = SETTING(TEXT_TIMESTAMP_FORE_COLOR);
	if(SETTING(TEXT_TIMESTAMP_BOLD))
		m_TextStyleTimestamp.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_TIMESTAMP_ITALIC))
		m_TextStyleTimestamp.dwEffects |= CFE_ITALIC;

	m_ChatTextGeneral = cf;
	m_ChatTextGeneral.crBackColor = SETTING(TEXT_GENERAL_BACK_COLOR);
	m_ChatTextGeneral.crTextColor = SETTING(TEXT_GENERAL_FORE_COLOR);
	if(SETTING(TEXT_GENERAL_BOLD))
		m_ChatTextGeneral.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_GENERAL_ITALIC))
		m_ChatTextGeneral.dwEffects |= CFE_ITALIC;

	m_TextStyleBold = m_ChatTextGeneral;
	m_TextStyleBold.dwEffects = CFE_BOLD;
	
	m_TextStyleMyNick = cf;
	m_TextStyleMyNick.crBackColor = SETTING(TEXT_MYNICK_BACK_COLOR);
	m_TextStyleMyNick.crTextColor = SETTING(TEXT_MYNICK_FORE_COLOR);
	if(SETTING(TEXT_MYNICK_BOLD))
		m_TextStyleMyNick.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_MYNICK_ITALIC))
		m_TextStyleMyNick.dwEffects |= CFE_ITALIC;

	m_ChatTextMyOwn = cf;
	m_ChatTextMyOwn.crBackColor = SETTING(TEXT_MYOWN_BACK_COLOR);
	m_ChatTextMyOwn.crTextColor = SETTING(TEXT_MYOWN_FORE_COLOR);
	if(SETTING(TEXT_MYOWN_BOLD))
		m_ChatTextMyOwn.dwEffects       |= CFE_BOLD;
	if(SETTING(TEXT_MYOWN_ITALIC))
		m_ChatTextMyOwn.dwEffects       |= CFE_ITALIC;

	m_ChatTextPrivate = cf;
	m_ChatTextPrivate.crBackColor = SETTING(TEXT_PRIVATE_BACK_COLOR);
	m_ChatTextPrivate.crTextColor = SETTING(TEXT_PRIVATE_FORE_COLOR);
	if(SETTING(TEXT_PRIVATE_BOLD))
		m_ChatTextPrivate.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_PRIVATE_ITALIC))
		m_ChatTextPrivate.dwEffects |= CFE_ITALIC;

	m_ChatTextSystem = cf;
	m_ChatTextSystem.crBackColor = SETTING(TEXT_SYSTEM_BACK_COLOR);
	m_ChatTextSystem.crTextColor = SETTING(TEXT_SYSTEM_FORE_COLOR);
	if(SETTING(TEXT_SYSTEM_BOLD))
		m_ChatTextSystem.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_SYSTEM_ITALIC))
		m_ChatTextSystem.dwEffects |= CFE_ITALIC;

	m_ChatTextServer = cf;
	m_ChatTextServer.crBackColor = SETTING(TEXT_SERVER_BACK_COLOR);
	m_ChatTextServer.crTextColor = SETTING(TEXT_SERVER_FORE_COLOR);
	if(SETTING(TEXT_SERVER_BOLD))
		m_ChatTextServer.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_SERVER_ITALIC))
		m_ChatTextServer.dwEffects |= CFE_ITALIC;

	m_ChatTextLog = m_ChatTextGeneral;
	m_ChatTextLog.crTextColor = OperaColors::blendColors(SETTING(TEXT_GENERAL_BACK_COLOR), SETTING(TEXT_GENERAL_FORE_COLOR), 0.4);

	m_TextStyleFavUsers = cf;
	m_TextStyleFavUsers.crBackColor = SETTING(TEXT_FAV_BACK_COLOR);
	m_TextStyleFavUsers.crTextColor = SETTING(TEXT_FAV_FORE_COLOR);
	if(SETTING(TEXT_FAV_BOLD))
		m_TextStyleFavUsers.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_FAV_ITALIC))
		m_TextStyleFavUsers.dwEffects |= CFE_ITALIC;

	m_TextStyleOPs = cf;
	m_TextStyleOPs.crBackColor = SETTING(TEXT_OP_BACK_COLOR);
	m_TextStyleOPs.crTextColor = SETTING(TEXT_OP_FORE_COLOR);
	if(SETTING(TEXT_OP_BOLD))
		m_TextStyleOPs.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_OP_ITALIC))
		m_TextStyleOPs.dwEffects |= CFE_ITALIC;

	m_TextStyleURL = cf;
	m_TextStyleURL.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR | CFM_LINK | CFM_UNDERLINE;
	m_TextStyleURL.crBackColor = SETTING(TEXT_URL_BACK_COLOR);
	m_TextStyleURL.crTextColor = SETTING(TEXT_URL_FORE_COLOR);
	m_TextStyleURL.dwEffects = CFE_LINK | CFE_UNDERLINE;
	if(SETTING(TEXT_URL_BOLD))
		m_TextStyleURL.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_URL_ITALIC))
		m_TextStyleURL.dwEffects |= CFE_ITALIC;
}

void WinUtil::uninit() {
	fileImages.Destroy();
	userImages.Destroy();
	flagImages.Destroy();
	::DeleteObject(font);
	::DeleteObject(boldFont);
	::DeleteObject(smallBoldFont);
	::DeleteObject(bgBrush);

	mainMenu.DestroyMenu();
	grantMenu.DestroyMenu();

	UnhookWindowsHookEx(hook);	

}

void WinUtil::decodeFont(const tstring& setting, LOGFONT &dest) {
	StringTokenizer<tstring> st(setting, _T(','));
	TStringList &sl = st.getTokens();
	
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(dest), &dest);
	tstring face;
	if(sl.size() == 4)
	{
		face = sl[0];
		dest.lfHeight = Util::toInt(Text::fromT(sl[1]));
		dest.lfWeight = Util::toInt(Text::fromT(sl[2]));
		dest.lfItalic = (BYTE)Util::toInt(Text::fromT(sl[3]));
	}
	
	if(!face.empty()) {
		memzero(dest.lfFaceName, LF_FACESIZE);
		_tcscpy(dest.lfFaceName, face.c_str());
	}
}

int CALLBACK WinUtil::browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData) {
	switch(uMsg) {
	case BFFM_INITIALIZED: 
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
		break;
	}
	return 0;
}

bool WinUtil::browseDirectory(tstring& target, HWND owner /* = NULL */) {
	TCHAR buf[MAX_PATH];
	BROWSEINFO bi;
	LPMALLOC ma;
	
	memzero(&bi, sizeof(bi));
	
	bi.hwndOwner = owner;
	bi.pszDisplayName = buf;
	bi.lpszTitle = CTSTRING(CHOOSE_FOLDER);
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lParam = (LPARAM)target.c_str();
	bi.lpfn = &browseCallbackProc;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if(pidl != NULL) {
		SHGetPathFromIDList(pidl, buf);
		target = buf;
		
		if(target.size() > 0 && target[target.size()-1] != _T('\\'))
			target+=_T('\\');
		
		if(SHGetMalloc(&ma) != E_FAIL) {
			ma->Free(pidl);
			ma->Release();
		}
		return true;
	}
	return false;
}

bool WinUtil::browseFile(tstring& target, HWND owner /* = NULL */, bool save /* = true */, const tstring& initialDir /* = Util::emptyString */, const TCHAR* types /* = NULL */, const TCHAR* defExt /* = NULL */) {
	TCHAR buf[MAX_PATH];
	OPENFILENAME ofn = { 0 };       // common dialog box structure
	target = Text::toT(Util::validateFileName(Text::fromT(target)));
	_tcscpy(buf, target.c_str());
	// Initialize OPENFILENAME
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = owner;
	ofn.lpstrFile = buf;
	ofn.lpstrFilter = types;
	ofn.lpstrDefExt = defExt;
	ofn.nFilterIndex = 1;

	if(!initialDir.empty()) {
		ofn.lpstrInitialDir = initialDir.c_str();
	}
	ofn.nMaxFile = sizeof(buf);
	ofn.Flags = (save ? 0: OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
	
	// Display the Open dialog box. 
	if ( (save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn) ) ==TRUE) {
		target = ofn.lpstrFile;
		return true;
	}
	return false;
}

tstring WinUtil::encodeFont(LOGFONT const& font)
{
	tstring res(font.lfFaceName);
	res += L',';
	res += Util::toStringT(font.lfHeight);
	res += L',';
	res += Util::toStringT(font.lfWeight);
	res += L',';
	res += Util::toStringT(font.lfItalic);
	return res;
}

void WinUtil::setClipboard(const tstring& str) {
	if(!::OpenClipboard(mainWnd)) {
		return;
	}

	EmptyClipboard();

#ifdef UNICODE	
	OSVERSIONINFOEX ver;
	if( WinUtil::getVersionInfo(ver) ) {
		if( ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
			string tmp = Text::wideToAcp(str);

			HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (tmp.size() + 1) * sizeof(char)); 
			if (hglbCopy == NULL) { 
				CloseClipboard(); 
				return; 
			} 

			// Lock the handle and copy the text to the buffer. 
			char* lptstrCopy = (char*)GlobalLock(hglbCopy); 
			strcpy(lptstrCopy, tmp.c_str());
			GlobalUnlock(hglbCopy);

			SetClipboardData(CF_TEXT, hglbCopy);

			CloseClipboard();

			return;
		}
	}
#endif

	// Allocate a global memory object for the text. 
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(TCHAR)); 
	if (hglbCopy == NULL) { 
		CloseClipboard(); 
		return; 
	} 

	// Lock the handle and copy the text to the buffer. 
	TCHAR* lptstrCopy = (TCHAR*)GlobalLock(hglbCopy); 
	_tcscpy(lptstrCopy, str.c_str());
	GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
#ifdef UNICODE
	SetClipboardData(CF_UNICODETEXT, hglbCopy); 
#else
	SetClipboardData(CF_TEXT hglbCopy);
#endif

	CloseClipboard();
}

void WinUtil::splitTokens(int* array, const string& tokens, int maxItems /* = -1 */) noexcept {
	StringTokenizer<string> t(tokens, _T(','));
	StringList& l = t.getTokens();
	if(maxItems == -1)
		maxItems = l.size();
	
	int k = 0;
	for(StringList::const_iterator i = l.begin(); i != l.end() && k < maxItems; ++i, ++k) {
		array[k] = Util::toInt(*i);
	}
}

bool WinUtil::getUCParams(HWND parent, const UserCommand& uc, StringMap& sm) noexcept {
	string::size_type i = 0;
	StringMap done;

	while( (i = uc.getCommand().find("%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			LineDlg dlg;
			dlg.title = Text::toT(Util::toString(" > ", uc.getDisplayName()));
			dlg.description = Text::toT(name);
			dlg.line = Text::toT(sm["line:" + name]);

			if(uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), dlg.description);
				Util::replace(_T("\\s"), _T(" "), dlg.description);
			}

			if(dlg.DoModal(parent) == IDOK) {
				sm["line:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	i = 0;
	while( (i = uc.getCommand().find("%[kickline:", i)) != string::npos) {
		i += 11;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			KickDlg dlg;
			dlg.title = Text::toT(Util::toString(" > ", uc.getDisplayName()));
			dlg.description = Text::toT(name);

			if(uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), dlg.description);
				Util::replace(_T("\\s"), _T(" "), dlg.description);
			}

			if(dlg.DoModal(parent) == IDOK) {
				sm["kickline:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	return true;
}

#define LINE2 _T("-- http://www.apexdc.net/  <ApexDC++ ") _T(VERSIONSTRING) _T(" / ") _T(DCVERSIONSTRING) _T(">")
TCHAR *msgs[] = { _T("\r\n-- I'm a happy ApexDC++ user. You could be happy too.\r\n") LINE2,
_T("\r\n-- rm-...what? Nope...never heard of it...\r\n") LINE2,
_T("\r\n-- Evolution of species: Ape --> Man\r\n-- Evolution of science: \"The Earth is Flat\" --> \"The Earth is Round\"\r\n-- Evolution of sharing: DC++ --> StrongDC++ --> ApexDC++\r\n") LINE2,
_T("\r\n-- I share, therefore I am.\r\n") LINE2,
_T("\r\n-- I came, I searched, I found...\r\n") LINE2,
_T("\r\n-- I came, I shared, I sent...\r\n") LINE2,
_T("\r\n-- I can add multiple users to the same file and download from them simultaneously :)\r\n") LINE2
};

#define MSGS 7

tstring CommandsStart = _T("\t\t\t\t\t HELP \t\t\t\t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
tstring CommandsPart1 = _T("/refresh \t\t\t\t(refresh share) \t\t\t\t\t\t\n/savequeue \t\t\t\t(save Download Queue) \t\t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
tstring CommandsPart2 = _T("/search <string> \t\t\t(do a search...) \t\t\t\t\t\t\t\n/g <searchstring> \t\t\t(search Google) \t\t\t\t\t\t\n/imdb <imdbquery> \t\t\t(search film from IMDB database) \t\t\t\t\t\n/whois [IP] \t\t\t\t(do whois query for IP) \t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
tstring CommandsPart3 = _T("/slots # \t\t\t\t(upload slots) \t\t\t\t\t\t\t\n/extraslots # \t\t\t\t(extra slots for small files) \t\t\t\t\t\n/smallfilesize # \t\t\t\t(max small file size) \t\t\t\t\n/ts \t\t\t\t\t(timestamps on/off) \t\t\t\n/connection \t\t\t\t(show IP and port where connected) \t\t\t\t\n/showjoins \t\t\t\t(enable/disable showing joins & parts in mainchat) \t\n/shutdown \t\t\t\t(activate shutdown sequence) \t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
tstring CommandsPart4 = _T("/apexdc++ \t\t\t\t(print ApexDC++ version to chat) \t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
tstring CommandsPart5 = _T("/away <msg> \t\t\t\t(enable/disable away mode) \t\t\t\t\t\n/winamp \t\t\t\t(Winamp spam to chat) \t\t\t\t\t\n/w \t\t\t\t\t(Winamp spam to chat) \t\t\t\t\t\n/clear,/c \t\t\t\t(clear chat) \t\t\t\t\t\n/ignorelist \t\t\t\t(show ignorelist in chat) \t\t\t\t\t\n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
tstring CommandsEnd = _T("\t\tFor more help please head to ApexDC++ Forums (Help -> Forums) \n------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
tstring WinUtil::commands = CommandsStart + CommandsPart1 + CommandsPart2 + CommandsPart3 + CommandsPart4 + CommandsPart5 + CommandsEnd;

bool WinUtil::checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson) {
	string::size_type i = cmd.find(' ');
	if(i != string::npos) {
		param = cmd.substr(i+1);
		cmd = cmd.substr(1, i - 1);
	} else {
		cmd = cmd.substr(1);
	}

	if(stricmp(cmd.c_str(), _T("log")) == 0) {
		if(stricmp(param.c_str(), _T("system")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM))));
		} else if(stricmp(param.c_str(), _T("downloads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::DOWNLOAD))));
		} else if(stricmp(param.c_str(), _T("uploads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::UPLOAD))));
		} else {
			return false;
		}
	} else if(stricmp(cmd.c_str(), _T("me")) == 0) {
		message = param;
		thirdPerson = true;
	} else if(stricmp(cmd.c_str(), _T("refresh"))==0) {
		try {
			ShareManager::getInstance()->setDirty();
			ShareManager::getInstance()->refresh(true);
		} catch(const ShareException& e) {
			status = Text::toT(e.getError());
		}
	} else if(stricmp(cmd.c_str(), _T("slots"))==0) {
		int j = Util::toInt(Text::fromT(param));
		if(j > 0) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, j);
			status = TSTRING(SLOTS_SET);
			ClientManager::getInstance()->infoUpdated();
		} else {
			status = TSTRING(INVALID_NUMBER_OF_SLOTS);
		}
	} else if(stricmp(cmd.c_str(), _T("search")) == 0) {
		if(!param.empty()) {
			SearchFrame::openWindow(param);
		} else {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		}
	} else if(stricmp(cmd.c_str(), _T("apexdc++")) == 0 || stricmp(cmd.c_str(), _T("apexdc")) == 0) {
		message = msgs[GET_TICK() % MSGS];
	} else if((stricmp(cmd.c_str(), _T("filext")) == 0) || (stricmp(cmd.c_str(), _T("fx")) == 0)){
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.filext.com/detaillist.php?extdetail=") + Text::toT(Util::encodeURI(Text::fromT(param))) + _T("&Submit3=Go%21"));
		}
	} else if((stricmp(cmd.c_str(), _T("y")) == 0) || (stricmp(cmd.c_str(), _T("yahoo")) == 0)) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://search.yahoo.com/search?p=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
		//Google defination search support
	} else if(stricmp(cmd.c_str(), _T("define")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.google.com/search?hl=en&q=define%3A+") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if((stricmp(cmd.c_str(), _T("uptime")) == 0) || (stricmp(cmd.c_str(), _T("ut")) == 0)) {
		thirdPerson = true;
		message = Text::toT("uptime: " + WinUtil::formatTime(time(NULL) - Util::getStartTime()));
	} else if((stricmp(cmd.c_str(), _T("ratio")) == 0) || (stricmp(cmd.c_str(), _T("r")) == 0)) {
		char ratio[256];
		thirdPerson = true;
		snprintf(ratio, sizeof(ratio), "ratio: %s (Uploaded: %s | Downloaded: %s)",
			(SETTING(TOTAL_DOWNLOAD) > 0) ? Util::toString(((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD))).c_str() : "inf.",
			Util::formatBytes(SETTING(TOTAL_UPLOAD)).c_str(),  Util::formatBytes(SETTING(TOTAL_DOWNLOAD)).c_str());
		message = Text::toT(ratio);
	} else if(stricmp(cmd.c_str(), _T("limit")) == 0) {
		if(BOOLSETTING(THROTTLE_ENABLE) == true) {
			SettingsManager::getInstance()->set(SettingsManager::THROTTLE_ENABLE, false);
			MainFrame::setLimiterButton(false);
			status = TSTRING(LIMITER_OFF);
		} else {
			SettingsManager::getInstance()->set(SettingsManager::THROTTLE_ENABLE, true);
			MainFrame::setLimiterButton(true);
			status = TSTRING(LIMITER_ON);
		}
	} else if(stricmp(cmd.c_str(), _T("away")) == 0) {
		if(Util::getAway() && param.empty()) {
			Util::setAway(false);
			status = TSTRING(AWAY_MODE_OFF);
		} else {
			Util::setAway(true);
			Util::setAwayMessage(Text::fromT(param));
			
			StringMap sm;
			status = TSTRING(AWAY_MODE_ON) + _T(" ") + Text::toT(Util::getAwayMessage(sm));
		}

		WinUtil::setButtonPressed(IDC_AWAY, Util::getAway());
		ClientManager::getInstance()->infoUpdated();
	} else if(stricmp(cmd.c_str(), _T("g")) == 0 || stricmp(cmd.c_str(), _T("google")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(stricmp(cmd.c_str(), _T("imdb")) == 0 || stricmp(cmd.c_str(), _T("i")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.imdb.com/find?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(stricmp(cmd.c_str(), _T("wikipedia")) == 0 || stricmp(cmd.c_str(), _T("wiki")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://en.wikipedia.org/w/index.php?title=Special%3ASearch&search=") + Text::toT(Util::encodeURI(Text::fromT(param))) + _T("&fulltext=Search"));
		}
	} else if(stricmp(cmd.c_str(), _T("discogs")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.discogs.com/search?type=all&q=") + Text::toT(Util::encodeURI(Text::fromT(param))) + _T("&btn=Search"));
		}
	} else if(stricmp(cmd.c_str(), _T("u")) == 0) {
		if (!param.empty()) {
			WinUtil::openLink(Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(stricmp(cmd.c_str(), _T("rebuild")) == 0) {
		HashManager::getInstance()->rebuild();
	} else if(stricmp(cmd.c_str(), _T("shutdown")) == 0) {
		MainFrame::setShutDown(!(MainFrame::getShutDown()));
		if (MainFrame::getShutDown()) {
			status = TSTRING(SHUTDOWN_ON);
		} else {
			status = TSTRING(SHUTDOWN_OFF);
			}
	} else if((stricmp(cmd.c_str(), _T("winamp")) == 0) || (stricmp(cmd.c_str(), _T("w")) == 0) || (stricmp(cmd.c_str(), _T("f")) == 0) || (stricmp(cmd.c_str(), _T("foobar")) == 0)) {
		HWND hwndWinamp = FindWindow(_T("Winamp v1.x"), NULL);
		if (hwndWinamp) {
			StringMap params;
			int waVersion = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETVERSION),
				majorVersion, minorVersion;
			majorVersion = waVersion >> 12;
			if (((waVersion & 0x00F0) >> 4) == 0) {
				minorVersion = ((waVersion & 0x0f00) >> 8) * 10 + (waVersion & 0x000f);
			} else {
				minorVersion = ((waVersion & 0x00f0) >> 4) * 10 + (waVersion & 0x000f);
			}
			params["version"] = Util::toString(majorVersion + minorVersion / 100.0);
			int state = SendMessage(hwndWinamp,WM_USER, 0, IPC_ISPLAYING);
			switch (state) {
				case 0: params["state"] = "stopped";
					break;
				case 1: params["state"] = "playing";
					break;
				case 3: params["state"] = "paused";
			};
			TCHAR titleBuffer[2048];
			int buffLength = sizeof(titleBuffer);
			GetWindowText(hwndWinamp, titleBuffer, buffLength);
			tstring title = titleBuffer;
			params["rawtitle"] = Text::fromT(title);
			// there's some winamp bug with scrolling. wa5.09x and 5.1 or something.. see:
			// http://forums.winamp.com/showthread.php?s=&postid=1768775#post1768775
			int starpos = title.find(_T("***"));
			if (starpos >= 1) {
				tstring firstpart = title.substr(0, starpos - 1);
				if (firstpart == title.substr(title.size() - firstpart.size(), title.size())) {
					// fix title
					title = title.substr(starpos, title.size());
				}
			}
			// fix the title if scrolling is on, so need to put the stairs to the end of it
			tstring titletmp = title.substr(title.find(_T("***")) + 2, title.size());
			title = titletmp + title.substr(0, title.size() - titletmp.size());
			title = title.substr(title.find(_T('.')) + 2, title.size());
			if (title.rfind(_T('-')) != string::npos) {
				params["title"] = Text::fromT(title.substr(0, title.rfind(_T('-')) - 1));
			}
			int curPos = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETOUTPUTTIME);
			int length = SendMessage(hwndWinamp,WM_USER, 1, IPC_GETOUTPUTTIME);
			if (curPos == -1) {
				curPos = 0;
			} else {
				curPos /= 1000;
			}
			int intPercent;
			if (length > 0 ) {
				intPercent = curPos * 100 / length;
			} else {
				length = 0;
				intPercent = 0;
			}
			params["percent"] = Util::toString(intPercent) + "%";
			params["elapsed"] = Text::fromT(Util::formatSeconds(curPos, true));
			params["length"] = Text::fromT(Util::formatSeconds(length, true));
			int numFront = min(max(intPercent / 10, 0), 10),
				numBack = min(max(10 - 1 - numFront, 0), 10);
			string inFront = string(numFront, '-'),
				inBack = string(numBack, '-');
			params["bar"] = "[" + inFront + (state ? "|" : "-") + inBack + "]";
			int waSampleRate = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETINFO),
				waBitRate = SendMessage(hwndWinamp,WM_USER, 1, IPC_GETINFO),
				waChannels = SendMessage(hwndWinamp,WM_USER, 2, IPC_GETINFO);
			params["bitrate"] = Util::toString(waBitRate) + "kbps";
			params["sample"] = Util::toString(waSampleRate) + "kHz";
			// later it should get some improvement:
			string waChannelName;
			switch (waChannels) {
				case 2:
					waChannelName = "stereo";
					break;
				case 6:
					waChannelName = "5.1 surround";
					break;
				default:
					waChannelName = "mono";
			}
			params["channels"] = waChannelName;
			message = Text::toT(Util::formatParams(SETTING(WINAMP_FORMAT), params, false));
			thirdPerson = strnicmp(message.c_str(), _T("/me "), 4) == 0;
			if(thirdPerson) {
				message = message.substr(4);
			}
		} else {
			if((stricmp(cmd.c_str(), _T("f")) == 0) || (stricmp(cmd.c_str(), _T("foobar")) == 0)) {
				status = _T("Supported version of Foobar2000 is not running or component foo_winamp_spam not installed");
			} else {
				status = _T("Supported version of Winamp is not running");
			}
		}
	} else if(stricmp(cmd.c_str(), _T("tvtome")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else
			WinUtil::openLink(_T("http://www.tvtome.com/tvtome/servlet/Search?searchType=all&searchString=") + Text::toT(Util::encodeURI(Text::fromT(param))));
	} else if(stricmp(cmd.c_str(), _T("csfd")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.csfd.cz/search.php?search=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else {
		return false;
	}

	return true;
}

void WinUtil::bitziLink(const TTHValue& aHash) {
	// to use this free service by bitzi, we must not hammer or request information from bitzi
	// except when the user requests it (a mass lookup isn't acceptable), and (if we ever fetch
	// this data within DC++, we must identify the client/mod in the user agent, so abuse can be 
	// tracked down and the code can be fixed
	openLink(_T("http://bitzi.com/lookup/tree:tiger:") + Text::toT(aHash.toBase32()));
}

string WinUtil::getMagnet(const TTHValue& aHash, const string& aFile, int64_t aSize) {
	return "magnet:?xt=urn:tree:tiger:" + aHash.toBase32() + "&xl=" + Util::toString(aSize) + "&dn=" + Util::encodeURI(aFile);
}

void WinUtil::copyMagnet(const TTHValue& aHash, const string& aFile, int64_t aSize) {
	if(!aFile.empty()) {
		setClipboard(Text::toT(getMagnet(aHash, aFile, aSize)));
 	}
}

 void WinUtil::searchHash(const TTHValue& aHash) {
	SearchFrame::openWindow(Text::toT(aHash.toBase32()), 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
 }

 void WinUtil::registerDchubHandler() {
	HKEY hk;
	TCHAR Buf[512];
	tstring app = _T("\"") + Text::toT(getAppName()) + _T("\" \"%1\"");
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		::RegCloseKey(hk);
	}

	if(stricmp(app.c_str(), Buf) != 0) {
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_DCHUB), LogManager::LOG_ERROR);
			return;
		}
	
		TCHAR* tmp = _T("URL:Direct Connect Protocol");
		::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, sizeof(TCHAR) * (_tcslen(tmp) + 1));
		::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		::RegCloseKey(hk);

		::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub\\Shell\\Open\\Command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);

		::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		app = Text::toT(getAppName());
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);
	}
}

 void WinUtil::unRegisterDchubHandler() {
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub"));
 }

 void WinUtil::registerADChubHandler() {
	 HKEY hk;
	 TCHAR Buf[512];
	 tstring app = _T("\"") + Text::toT(getAppName()) + _T("\" \"%1\"");
	 Buf[0] = 0;

	 if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		 DWORD bufLen = sizeof(Buf);
		 DWORD type;
		 ::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		 ::RegCloseKey(hk);
	 }

	 if(stricmp(app.c_str(), Buf) != 0) {
		 if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			 LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_ADC), LogManager::LOG_ERROR);
			 return;
		 }

		 TCHAR* tmp = _T("URL:Direct Connect Protocol");
		 ::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, sizeof(TCHAR) * (_tcslen(tmp) + 1));
		 ::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		 ::RegCloseKey(hk);

		 ::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc\\Shell\\Open\\Command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);

		 ::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		 app = Text::toT(getAppName());
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);
	 }
 }

 void WinUtil::unRegisterADChubHandler() {
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc"));
 }

void WinUtil::registerMagnetHandler() {
	HKEY hk;
	TCHAR buf[512];
	tstring openCmd;
	tstring appName = Text::toT(getAppName());
	buf[0] = 0;

	// what command is set up to handle magnets right now?
	if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet\\shell\\open\\command"), 0, KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(TCHAR) * sizeof(buf);
		::RegQueryValueEx(hk, NULL, NULL, NULL, (LPBYTE)buf, &bufLen);
		::RegCloseKey(hk);
	}
	openCmd = buf;
	buf[0] = 0;

	// (re)register the handler if magnet.exe isn't the default, or if DC++ is handling it
	if(BOOLSETTING(MAGNET_REGISTER) && (strnicmp(openCmd.c_str(), appName.c_str(), appName.size()) != 0)) {
		SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet"));
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_MAGNET), LogManager::LOG_ERROR);
			return;
		}
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)CTSTRING(MAGNET_SHELL_DESC), sizeof(TCHAR)*(TSTRING(MAGNET_SHELL_DESC).length()+1));
		::RegSetValueEx(hk, _T("URL Protocol"), NULL, REG_SZ, NULL, NULL);
		::RegCloseKey(hk);
		::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)appName.c_str(), sizeof(TCHAR)*(appName.length()+1));
		::RegCloseKey(hk);
		appName += _T(" \"%1\"");
		::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet\\shell\\open\\command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)appName.c_str(), sizeof(TCHAR)*(appName.length()+1));
		::RegCloseKey(hk);
	}
}

void WinUtil::unRegisterMagnetHandler() {
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet"));
}

void WinUtil::openLink(const tstring& url) {
	if(_strnicmp(Text::fromT(url).c_str(), "magnet:?", 8) == 0) {
		parseMagnetUri(url);
		return;
	}
	if(_strnicmp(Text::fromT(url).c_str(), "dchub://", 8) == 0) {
		parseDchubUrl(url, false);
		return;
	}
	if(_strnicmp(Text::fromT(url).c_str(), "nmdcs://", 8) == 0) {
		parseDchubUrl(url, true);
		return;
	}	
	if(_strnicmp(Text::fromT(url).c_str(), "adc://", 6) == 0) {
		parseADChubUrl(url, false);
		return;
	}
	if(_strnicmp(Text::fromT(url).c_str(), "adcs://", 7) == 0) {
		parseADChubUrl(url, true);
		return;
	}	

	::ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void WinUtil::parseDchubUrl(const tstring& aUrl, bool secure) {
	uint16_t port = 411;
	string proto, host, file, query, fragment;
	Util::decodeUrl(Text::fromT(aUrl), proto, host, port, file, query, fragment);
	string url = host + ":" + Util::toString(port);
	if(!host.empty()) {
		HubFrame::openWindow((secure ? _T("nmdcs://") : _T("")) + Text::toT(host) + _T(":") + Util::toStringW(port));
	}
	if(!file.empty()) {
		if(file[0] == '/') // Remove any '/' in from of the file
			file = file.substr(1);
		try {
			if(!file.empty()) {
				UserPtr user = ClientManager::getInstance()->findLegacyUser(file);
				if(user)
					QueueManager::getInstance()->addList(HintedUser(user, url), QueueItem::FLAG_CLIENT_VIEW);
			}
			// @todo else report error
		} catch(const Exception&) {
			// ...
		}
	}
}

void WinUtil::parseADChubUrl(const tstring& aUrl, bool secure) {
	string proto, host, file, query, fragment;
	uint16_t port = 0; //make sure we get a port since adc doesn't have a standard one
	Util::decodeUrl(Text::fromT(aUrl), proto, host, port, file, query, fragment);
	if(!host.empty() && port > 0) {
		tstring queryPart;

		if (!query.empty())
			queryPart = _T("?") + Text::toT(query) + Text::toT(fragment);

		HubFrame::openWindow((secure ? _T("adcs://") : _T("adc://")) + Text::toT(host) + _T(":") + Util::toStringW(port) + Text::toT(file) + queryPart);
	}
}

void WinUtil::parseMagnetUri(const tstring& aUrl, bool /*aOverride*/) {
	string hash, name, key;
	uint64_t size = 0;
	if (Magnet::parseUri(Text::fromT(aUrl), hash, name, size, key) && Encoder::isBase32(hash.c_str())) {
		name = Util::validateFileName(string(name.c_str()));
		if(!BOOLSETTING(MAGNET_ASK) && size > 0 && !name.empty()) {
			switch(SETTING(MAGNET_ACTION)) {
				case SettingsManager::MAGNET_AUTO_DOWNLOAD:
					try {
						QueueManager::getInstance()->add(FavoriteManager::getInstance()->getDownloadDirectory(Util::getFileExt(name)) + name, size, TTHValue(hash), HintedUser(UserPtr(), Util::emptyString));
					} catch(const Exception& e) {
						LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);
					}
					break;
				case SettingsManager::MAGNET_AUTO_SEARCH:
					SearchFrame::openWindow(Text::toT(hash), 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
					break;
			};
		} else {
			// use aOverride to force the display of the dialog. used for auto-updating
			MagnetDlg dlg(Text::toT(hash), Text::toT(name), size);
			dlg.DoModal(mainWnd);
		}
	} else {
		MessageBox(mainWnd, CTSTRING(MAGNET_DLG_TEXT_BAD), CTSTRING(MAGNET_DLG_TITLE), MB_OK | MB_ICONEXCLAMATION);
	}
}

int WinUtil::textUnderCursor(POINT p, CEdit& ctrl, tstring& x) {
	
	int i = ctrl.CharFromPos(p);
	int line = ctrl.LineFromChar(i);
	int c = LOWORD(i) - ctrl.LineIndex(line);
	int len = ctrl.LineLength(i) + 1;
	if(len < 3) {
		return 0;
	}

	x.resize(len);
	ctrl.GetLine(line, &x[0], len);

	string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
	if(start == string::npos)
		start = 0;
	else
		start++;

	return start;
}

bool WinUtil::parseDBLClick(const tstring& aString, string::size_type start, string::size_type end) {
	if( (strnicmp(aString.c_str() + start, _T("http://"), 7) == 0) || 
		(strnicmp(aString.c_str() + start, _T("www."), 4) == 0) ||
		(strnicmp(aString.c_str() + start, _T("ftp://"), 6) == 0) ||
		(strnicmp(aString.c_str() + start, _T("irc://"), 6) == 0) ||
		(strnicmp(aString.c_str() + start, _T("https://"), 8) == 0) ||	
		(strnicmp(aString.c_str() + start, _T("file://"), 7) == 0) ||
		(strnicmp(aString.c_str() + start, _T("mailto:"), 7) == 0) )
	{

		openLink(aString.substr(start, end-start));
		return true;
	} else if(strnicmp(aString.c_str() + start, _T("dchub://"), 8) == 0) {
		parseDchubUrl(aString.substr(start, end-start), false);
		return true;
	} else if(strnicmp(aString.c_str() + start, _T("nmdcs://"), 8) == 0) {
		parseDchubUrl(aString.substr(start, end-start), true);
		return true;	
	} else if(strnicmp(aString.c_str() + start, _T("magnet:?"), 8) == 0) {
		parseMagnetUri(aString.substr(start, end-start));
		return true;
	} else if(strnicmp(aString.c_str() + start, _T("adc://"), 6) == 0) {
		parseADChubUrl(aString.substr(start, end-start), false);
		return true;
	} else if(strnicmp(aString.c_str() + start, _T("adcs://"), 7) == 0) {
		parseADChubUrl(aString.substr(start, end-start), true);
		return true;
	}
	return false;
}

void WinUtil::saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
							  SettingsManager::StrSetting widths, int n, 
							  int* indexes, int* sizes) noexcept {
	string tmp;

	ctrl.GetColumnOrderArray(n, indexes);
	int i;
	for(i = 0; i < n; ++i) {
		tmp += Util::toString(indexes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(order, tmp);
	tmp.clear();
	int nHeaderItemsCount = ctrl.GetHeader().GetItemCount();
	for(i = 0; i < n; ++i) {
		sizes[i] = ctrl.GetColumnWidth(i);
		if (i >= nHeaderItemsCount) // Not exist column
			sizes[i] = 0;
		tmp += Util::toString(sizes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(widths, tmp);
}

int WinUtil::getIconIndex(const tstring& aFileName) {
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		tstring x = Text::toLower(Util::getFileExt(aFileName));
		if(!x.empty()) {
			ImageIter j = fileIndexes.find(x);
			if(j != fileIndexes.end())
				return j->second;
		}
		tstring fn = Text::toLower(Util::getFileName(aFileName));
		::SHGetFileInfo(fn.c_str(), FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		::DestroyIcon(fi.hIcon);

		fileIndexes[x] = fileImageCount++;
		return fileImageCount - 1;
	} else {
		return 2;
	}
}

double WinUtil::toBytes(TCHAR* aSize) {
	double bytes = _tstof(aSize);

	if (_tcsstr(aSize, CTSTRING(PB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(TB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(GB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(MB))) {
		return bytes * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(KB))) {
		return bytes * 1024.0;
	} else {
		return bytes;
	}
}

int WinUtil::getOsMajor(bool wowver /*= false*/) {
	if(wowver)
		return (DWORD)(LOBYTE(LOWORD(GetVersion())));

	if(osvi.dwOSVersionInfoSize == 0)
		WinUtil::getVersionInfo(osvi);

	return osvi.dwMajorVersion;
}

int WinUtil::getOsMinor(bool wowver /*= false*/) {
	if(wowver)
		return (DWORD)(HIBYTE(LOWORD(GetVersion())));

	if(osvi.dwOSVersionInfoSize == 0)
		WinUtil::getVersionInfo(osvi);

	return osvi.dwMinorVersion;
}

tstring WinUtil::getNicks(const CID& cid, const string& hintUrl) {
	return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(cid, hintUrl)));
}

tstring WinUtil::getNicks(const UserPtr& u, const string& hintUrl) {
	return getNicks(u->getCID(), hintUrl);
}

tstring WinUtil::getNicks(const CID& cid, const string& hintUrl, bool priv) {
	return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(cid, hintUrl, priv)));
}

static pair<tstring, bool> formatHubNames(const StringList& hubs) {
	if(hubs.empty()) {
		return make_pair(CTSTRING(OFFLINE), false);
	} else {
		return make_pair(Text::toT(Util::toString(hubs)), true);
	}
}

pair<tstring, bool> WinUtil::getHubNames(const CID& cid, const string& hintUrl) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid, hintUrl));
}

pair<tstring, bool> WinUtil::getHubNames(const UserPtr& u, const string& hintUrl) {
	return getHubNames(u->getCID(), hintUrl);
}

pair<tstring, bool> WinUtil::getHubNames(const CID& cid, const string& hintUrl, bool priv) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid, hintUrl, priv));
}

void WinUtil::getContextMenuPos(CListViewCtrl& aList, POINT& aPt) {
	int pos = aList.GetNextItem(-1, LVNI_SELECTED | LVNI_FOCUSED);
	if(pos >= 0) {
		CRect lrc;
		aList.GetItemRect(pos, &lrc, LVIR_LABEL);
		aPt.x = lrc.left;
		aPt.y = lrc.top + (lrc.Height() / 2);
	} else {
		aPt.x = aPt.y = 0;
	}
	aList.ClientToScreen(&aPt);
}

void WinUtil::getContextMenuPos(CTreeViewCtrl& aTree, POINT& aPt) {
	CRect trc;
	HTREEITEM ht = aTree.GetSelectedItem();
	if(ht) {
		aTree.GetItemRect(ht, &trc, TRUE);
		aPt.x = trc.left;
		aPt.y = trc.top + (trc.Height() / 2);
	} else {
		aPt.x = aPt.y = 0;
	}
	aTree.ClientToScreen(&aPt);
}
void WinUtil::getContextMenuPos(CEdit& aEdit, POINT& aPt) {
	CRect erc;
	aEdit.GetRect(&erc);
	aPt.x = erc.Width() / 2;
	aPt.y = erc.Height() / 2;
	aEdit.ClientToScreen(&aPt);
}

void WinUtil::openFolder(const tstring& file) {
	if (File::getSize(Text::fromT(file)) != -1)
		::ShellExecute(NULL, NULL, Text::toT("explorer.exe").c_str(), Text::toT("/e, /select, \"" + (Text::fromT(file)) + "\"").c_str(), NULL, SW_SHOWNORMAL);
	else
		::ShellExecute(NULL, NULL, Text::toT("explorer.exe").c_str(), Text::toT("/e, \"" + Util::getFilePath(Text::fromT(file)) + "\"").c_str(), NULL, SW_SHOWNORMAL);
}

void WinUtil::ClearPreviewMenu(OMenu &previewMenu){
	while(previewMenu.GetMenuItemCount() > 0) {
		previewMenu.RemoveMenu(0, MF_BYPOSITION);
	}
}

int WinUtil::SetupPreviewMenu(CMenu &previewMenu, string extension){
	int PreviewAppsSize = 0;
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();
	if(lst.size()>0){		
		PreviewAppsSize = 0;
		for(PreviewApplication::Iter i = lst.begin(); i != lst.end(); i++){
			StringList tok = StringTokenizer<string>((*i)->getExtension(), ';').getTokens();
			bool add = false;
			if(tok.size()==0)add = true;

			
			for(StringIter si = tok.begin(); si != tok.end(); ++si) {
				if(_stricmp(extension.c_str(), si->c_str())==0){
					add = true;
					break;
				}
			}
							
			if (add) previewMenu.AppendMenu(MF_STRING, IDC_PREVIEW_APP + PreviewAppsSize, Text::toT(((*i)->getName())).c_str());
			PreviewAppsSize++;
		}
	}
	return PreviewAppsSize;
}

void WinUtil::RunPreviewCommand(unsigned int index, const string& target) {
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();

	if(index <= lst.size()) {
		string application = lst[index]->getApplication();
		string arguments = lst[index]->getArguments();
		StringMap ucParams;				
	
		ucParams["file"] = "\"" + target + "\"";
		ucParams["dir"] = "\"" + Util::getFilePath(target) + "\"";

		::ShellExecute(NULL, NULL, Text::toT(application).c_str(), Text::toT(Util::formatParams(arguments, ucParams, false)).c_str(), Text::toT(ucParams["dir"]).c_str(), SW_SHOWNORMAL);
	}
}

string WinUtil::formatTime(time_t rest) {
	char buf[128];
	string formatedTime;
	uint64_t n, i;
	i = 0;
	n = rest / (24*3600*7);
	rest %= (24*3600*7);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%lld weeks ", n);
		else
			snprintf(buf, sizeof(buf), "%lld week ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (24*3600);
	rest %= (24*3600);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%lld days ", n); 
		else
			snprintf(buf, sizeof(buf), "%lld day ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (3600);
	rest %= (3600);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%lld hours ", n);
		else
			snprintf(buf, sizeof(buf), "%lld hour ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (60);
	rest %= (60);
	if(n) {
		snprintf(buf, sizeof(buf), "%lld min ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest;
	if(++i <= 3) {
		snprintf(buf, sizeof(buf),"%lld sec ", n); 
		formatedTime += (string)buf;
	}
	return formatedTime;
}

uint8_t WinUtil::getFlagIndexByCode(const char* countryCode) {
	// country codes are sorted, use binary search for better performance
	int begin = 0;
	int end = (sizeof(countryCodes) / sizeof(countryCodes[0])) - 1;

	while(begin <= end) {
		int mid = end + (begin - end) / 2;
		int cmp = memcmp(countryCode, countryCodes[mid], 2);

		if(cmp > 0)
			begin = mid + 1;
		else if(cmp < 0)
			end = mid - 1;
		else
			return static_cast<uint8_t>(mid + 1);
	}

	return 0;
}

uint8_t WinUtil::getFlagIndexByName(const char* countryName) {
	// country codes are not sorted, use linear searching (it is not used so often)
	for(uint8_t i = 0; i < (sizeof(countryNames) / sizeof(countryNames[0])); ++i)
		if(_stricmp(countryName, countryNames[i]) == 0)
			return i + 1;

	return 0;
}

wchar_t arrayutf[42] = { L'Á', L'È', L'Ï', L'É', L'Ì', L'Í', L'¼', L'Ò', L'Ó', L'Ø', L'Š', L'', L'Ú', L'Ù', L'Ý', L'Ž', L'á', L'è', L'ï', L'é', L'ì', L'í', L'¾', L'ò', L'ó', L'ø', L'š', L'', L'ú', L'ù', L'ý', L'ž', L'Ä', L'Ë', L'Ö', L'Ü', L'ä', L'ë', L'ö', L'ü', L'£', L'³' };
wchar_t arraywin[42] = { L'A', L'C', L'D', L'E', L'E', L'I', L'L', L'N', L'O', L'R', L'S', L'T', L'U', L'U', L'Y', L'Z', L'a', L'c', L'd', L'e', L'e', L'i', L'l', L'n', L'o', L'r', L's', L't', L'u', L'u', L'y', L'z', L'A', L'E', L'O', L'U', L'a', L'e', L'o', L'u', L'L', L'l' };

const tstring& WinUtil::disableCzChars(tstring& message) {
	for(size_t j = 0; j < message.length(); j++) {
		for(size_t l = 0; l < (sizeof(arrayutf) / sizeof(arrayutf[0])); l++) {
			if (message[j] == arrayutf[l]) {
				message[j] = arraywin[l];
				break;
			}
		}
	}

	return message;
}

string WinUtil::generateStats() {
	OSVERSIONINFOEX osvi;
	if(WinUtil::getVersionInfo(osvi) &&
		((osvi.dwMajorVersion >= 5 && osvi.dwMinorVersion >= 1 && osvi.wServicePackMajor >= 1)
		|| osvi.dwMajorVersion >= 6))
	{
		typedef BOOL (CALLBACK* LPFUNC)(HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);
		typedef ULONGLONG (CALLBACK* LPFUNC2)(void);

		static HINSTANCE psapiLib = NULL;
		static HINSTANCE kernel32lib = NULL;

		if(!psapiLib)
			psapiLib = LoadLibrary(_T("psapi"));

		char buf[1024];
		PROCESS_MEMORY_COUNTERS pmc;
		pmc.cb = sizeof(pmc);

		LPFUNC _GetProcessMemoryInfo = (LPFUNC)GetProcAddress(psapiLib, "GetProcessMemoryInfo");
		_GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

		FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
		GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
		int64_t cpuTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
		cpuTime += userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);
		cpuTime /= (10I64 * 1000I64 * 1000I64);

		if(!kernel32lib)
			kernel32lib = LoadLibrary(_T("kernel32"));

		LPFUNC2 _GetTickCount64 = (LPFUNC2)GetProcAddress(kernel32lib, "GetTickCount64");
		time_t sysUptime = (_GetTickCount64 ? _GetTickCount64() : GetTickCount()) / 1000;

		snprintf(buf, sizeof(buf), "\r\n-=[ ApexDC++ %s Compiled on: %s ]=-\r\n-=[ Uptime: %s][ Cpu time: %s ]=-\r\n-=[ Memory usage (peak): %s (%s) ]=-\r\n-=[ Virtual memory usage (peak): %s (%s) ]=-\r\n-=[ Downloaded: %s ][ Uploaded: %s ]=-\r\n-=[ Total download: %s ][ Total upload: %s ]=-\r\n-=[ System Uptime: %s]=-", 
			VERSIONSTRING, Text::fromT(getCompileDate()).c_str(), formatTime(time(NULL) - Util::getStartTime()).c_str(), Text::fromT(Util::formatSeconds(cpuTime)).c_str(), 
			Util::formatBytes(pmc.WorkingSetSize).c_str(), Util::formatBytes(pmc.PeakWorkingSetSize).c_str(), 
			Util::formatBytes(pmc.PagefileUsage).c_str(), Util::formatBytes(pmc.PeakPagefileUsage).c_str(), 
			Util::formatBytes(Socket::getTotalDown()).c_str(), Util::formatBytes(Socket::getTotalUp()).c_str(), 
			Util::formatBytes(SETTING(TOTAL_DOWNLOAD)).c_str(), Util::formatBytes(SETTING(TOTAL_UPLOAD)).c_str(),
			formatTime(sysUptime).c_str());
		return buf;
	} else {
		char buf[512];
		snprintf(buf, sizeof(buf), "\r\n-=[ ApexDC++ %s Compiled on: %s ]=-\r\n-=[ Uptime: %s]=-\r\n-=[ Downloaded: %s ][ Uploaded: %s ]=-\r\n-=[ Total download: %s ][ Total upload: %s ]=-", 
			VERSIONSTRING, Text::fromT(getCompileDate()).c_str(), formatTime(time(NULL) - Util::getStartTime()).c_str(),
			Util::formatBytes(Socket::getTotalDown()).c_str(), Util::formatBytes(Socket::getTotalUp()).c_str(), 
			Util::formatBytes(SETTING(TOTAL_DOWNLOAD)).c_str(), Util::formatBytes(SETTING(TOTAL_UPLOAD)).c_str());
		return buf;
	}
} 

bool WinUtil::shutDown(int action) {
	// Prepare for shutdown
	UINT iForceIfHung = 0;
	OSVERSIONINFOEX osvi;
	if (WinUtil::getVersionInfo(osvi) && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		iForceIfHung = EWX_FORCEIFHUNG;
		HANDLE hToken;
		OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken);

		LUID luid;
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid);
		
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		tp.Privileges[0].Luid = luid;
		AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
		CloseHandle(hToken);
	}

	// Shutdown
	switch(action) {
		case 0: { action = EWX_POWEROFF; break; }
		case 1: { action = EWX_LOGOFF; break; }
		case 2: { action = EWX_REBOOT; break; }
		case 3: { SetSuspendState(false, false, false); return true; }
		case 4: { SetSuspendState(true, false, false); return true; }
		case 5: { 
			if(osvi.dwMajorVersion >= 5) {
				typedef BOOL (CALLBACK* LPLockWorkStation)(void);
				static HINSTANCE user32lib = NULL;

				if(!user32lib)
					user32lib = LoadLibrary(_T("user32"));

				LPLockWorkStation _d_LockWorkStation = (LPLockWorkStation)GetProcAddress(user32lib, "LockWorkStation");
				_d_LockWorkStation();
			}
			return true;
		}
	}

	if (ExitWindowsEx(action | iForceIfHung, 0) == 0) {
		return false;
	} else {
		return true;
	}
}

int WinUtil::getFirstSelectedIndex(CListViewCtrl& list) {
	for(int i = 0; i < list.GetItemCount(); ++i) {
		if (list.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED) {
			return i;
		}
	}
	return -1;
}

int WinUtil::setButtonPressed(int nID, bool bPressed /* = true */) {
	if (nID == -1)
		return -1;
	if (!MainFrame::getMainFrame()->getToolBar().IsWindow())
		return -1;

	MainFrame::getMainFrame()->getToolBar().CheckButton(nID, bPressed);
	return 0;
}

void WinUtil::showPopup(tstring szMsg, tstring szTitle, HICON hIcon, bool force) {
	MainFrame::getMainFrame()->ShowPopup(szMsg, szTitle, hIcon, force); 
}

void WinUtil::showPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags, bool force) {
	MainFrame::getMainFrame()->ShowPopup(szMsg, szTitle, dwInfoFlags, force); 
}

void WinUtil::setPMTray() {
	::PostMessage(MainFrame::getMainFrame()->m_hWnd, WM_SPEAKER, MainFrame::SET_PM_TRAY_ICON, NULL);
}

void WinUtil::MDIActivate(HWND hwndChild) {
	MainFrame::getMainFrame()->MDIActivate(hwndChild);
}

HWND WinUtil::MDIGetActive() {
	return MainFrame::getMainFrame()->MDIGetActive();
}

bool WinUtil::setListCtrlWatermark(HWND hListCtrl, UINT nID, COLORREF clr, int width /*= 128*/, int height /*= 128*/) {
	// Only version 6 and up can do watermarks
	if(!BOOLSETTING(USE_EXPLORER_THEME) || WinUtil::comCtlVersion < MAKELONG(0,6))
		return false;

	// Despite documentation LVBKIF_FLAG_ALPHABLEND works only with version 6.1 and up
	bool supportsAlpha = (WinUtil::comCtlVersion >= MAKELONG(1,6));

	// If there already is a watermark with alpha channel, stop here...
	if(supportsAlpha) {
		LVBKIMAGE lvbk;
		lvbk.ulFlags = LVBKIF_TYPE_WATERMARK;
		ListView_GetBkImage(hListCtrl, &lvbk);
		if(lvbk.hbm != NULL) return false;
	}

	ExCImage image;
	if(!image.LoadFromResource(nID, _T("PNG")))
		return false;

	HBITMAP bmp = NULL;
	HDC screen_dev = ::GetDC(hListCtrl);

	if(screen_dev) {
		// Create a compatible DC
		HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
		if(dst_hdc) {
			// Create a new bitmap of icon size
			bmp = ::CreateCompatibleBitmap(screen_dev, width, height);
			if(bmp) {
				// Select it into the compatible DC
				HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);

				// Fill the background of the compatible DC with the given color
				if(!supportsAlpha) {
					::SetBkColor(dst_hdc, clr);
					RECT rc = { 0, 0, width, height };
					::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
				}

				// Draw the icon into the compatible DC
				image.Draw(dst_hdc, 0, 0, width, height);
				::SelectObject(dst_hdc, old_dst_bmp);
			}
			::DeleteDC(dst_hdc);
		}
	}

	// Free stuff
	::ReleaseDC(hListCtrl, screen_dev); 
	image.Destroy();

	if(!bmp)
		return false;

	LVBKIMAGE lv;
	lv.ulFlags = LVBKIF_TYPE_WATERMARK | (supportsAlpha ? LVBKIF_FLAG_ALPHABLEND : 0);
	lv.hbm = bmp;
	lv.xOffsetPercent = 100;
	lv.yOffsetPercent = 100;
	ListView_SetBkImage(hListCtrl, &lv);
	return true;
}

// ApexDC++ MiniInstaller
void WinUtil::createShortcut(const string& aPath, const string& aLinkName, const string& aWorkingDir, const string& aDesc, int nFolder) {
	HRESULT hres;
	IShellLink* psl;
	TCHAR linkDir[MAX_PATH];

	::SHGetFolderPath(NULL, nFolder | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, linkDir);
	const tstring linkPath = linkDir + Text::toT("\\" + aLinkName + ".lnk");

	// Get a pointer to the IShellLink interface. 
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
									IID_IShellLink, (PVOID *) &psl);
	if(SUCCEEDED(hres)) {
		IPersistFile* ppf;

		psl->SetPath(Text::toT(aPath).c_str());
		psl->SetWorkingDirectory(Text::toT(aWorkingDir).c_str());
		psl->SetDescription(Text::toT(aDesc).c_str());

		hres = psl->QueryInterface(IID_IPersistFile, (PVOID*) &ppf);

		if(SUCCEEDED(hres)) {
			hres = ppf->Save(linkPath.c_str(), TRUE);
			ppf->Release();
		}
		psl->Release();
	}
}

void WinUtil::moveDirectory(const string& aSource, const string& aTarget, bool bSilent /*= true*/) {
	if(Util::fileExists(aSource)) {
		if(bSilent) {
			File::ensureDirectory(aTarget);
			WIN32_FIND_DATA fData;
			HANDLE hFind;
	
			hFind = FindFirstFile(Text::toT(aSource + "*").c_str(), &fData);

			if(hFind != INVALID_HANDLE_VALUE) {
				do {
					const string& name = Text::fromT(fData.cFileName);
					if(name == "." || name == "..")
						continue;
					if(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						moveDirectory(aSource + name + PATH_SEPARATOR, aTarget + name + PATH_SEPARATOR);
					} else {
						try {
							File::renameFile(aSource + name, aTarget + name);
						} catch(const FileException&) {
							// ...
						}
					}
				} while(FindNextFile(hFind, &fData));
			}

			FindClose(hFind);
			RemoveDirectory(Text::toT(aSource).c_str());
		} else {
			SHFILEOPSTRUCT SHFileOp;
			memzero(&SHFileOp, sizeof(SHFILEOPSTRUCT));

			TCHAR fromPath[MAX_PATH];
			TCHAR toPath[MAX_PATH];
			memset(fromPath, '\0', MAX_PATH);
			memset(toPath, '\0', MAX_PATH);

			lstrcpy(fromPath, Text::toT(aSource + "*").c_str());
			lstrcpy(toPath, Text::toT(aTarget).c_str());

			SHFileOp.hwnd = NULL;
			SHFileOp.wFunc = FO_MOVE;
			SHFileOp.pFrom = fromPath;
			SHFileOp.pTo = toPath;
			SHFileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

			if(SHFileOperation(&SHFileOp) == 0) {
				RemoveDirectory(Text::toT(aSource).c_str());
			}
		}
	}
}

void WinUtil::removeDirectory(const string& aSource) {
	if(Util::fileExists(aSource)) {
		File::ensureDirectory(aSource);
		WIN32_FIND_DATA fData;
		HANDLE hFind;
	
		hFind = FindFirstFile(Text::toT(aSource + "*").c_str(), &fData);

		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				const string& name = Text::fromT(fData.cFileName);
				if(name == "." || name == "..")
					continue;
				if(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					removeDirectory(aSource + name + PATH_SEPARATOR);
				} else {
					try {
						if(fData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
							SetFileAttributes(Text::toT(aSource + name).c_str(), FILE_ATTRIBUTE_NORMAL);
						}
						File::deleteFile(aSource + name);
					} catch(const FileException&) {
						// ...
					}
				}
			} while(FindNextFile(hFind, &fData));
		}

		FindClose(hFind);
		RemoveDirectory(Text::toT(aSource).c_str());
	}
}

void WinUtil::alterWinFirewall(bool bState) {
	OSVERSIONINFOEX ver;
	WinUtil::getVersionInfo(ver);
	if(!((ver.dwMajorVersion >= 5 && ver.dwMinorVersion >= 1 && ver.wServicePackMajor >= 2) || (ver.dwMajorVersion >= 6)) || !isUserAdmin()) {
		return;
	}

	HRESULT hres = S_OK;
	INetFwProfile* fwProfile = NULL;
	INetFwMgr* fwMgr = NULL;
	INetFwPolicy* fwPolicy = NULL;

	// Create an instance of the firewall settings manager.
	hres = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&fwMgr);
	if(SUCCEEDED(hres)) {
		// Retrieve the local firewall policy.
		hres = fwMgr->get_LocalPolicy(&fwPolicy);
		if(SUCCEEDED(hres)) {
			// Retrieve the firewall profile currently in effect
			hres = fwPolicy->get_CurrentProfile(&fwProfile);
		}
	}

	// Release the local firewall policy.
	if(fwPolicy != NULL) {
		fwPolicy->Release();
	}

	// Release the firewall settings manager.
	if(fwMgr != NULL) {
		fwMgr->Release();
	}

	if(SUCCEEDED(hres)) {
		INetFwAuthorizedApplication* fwApp = NULL;
		INetFwAuthorizedApplications* fwApps = NULL;
		VARIANT_BOOL fwAppEnabled = NULL;

		// Retrieve the authorized application collection.
		hres = fwProfile->get_AuthorizedApplications(&fwApps);
		if(SUCCEEDED(hres)) {
			// Attempt to retrieve the authorized application.
			hres = fwApps->Item(CComBSTR(Text::toT(WinUtil::getAppName()).c_str()), &fwApp);
			if(SUCCEEDED(hres)) {
				// Find out if the authorized application is enabled.
				hres = fwApp->get_Enabled(&fwAppEnabled);
			}
		}

		if(bState && fwApps != NULL) {
			if(SUCCEEDED(hres)) {
				if(fwAppEnabled == VARIANT_FALSE) {
					hres = fwApp->put_Enabled(VARIANT_TRUE);
				}
			} else {
				hres = CoCreateInstance(__uuidof(NetFwAuthorizedApplication), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwAuthorizedApplication), (void**)&fwApp);
				if(SUCCEEDED(hres)) {
					hres = fwApp->put_ProcessImageFileName(CComBSTR(Text::toT(WinUtil::getAppName()).c_str()));
					if(SUCCEEDED(hres)) {
						hres = fwApp->put_Name(CComBSTR(_T(APPNAME) _T(" - Pinnacle of File Sharing")));
						if(SUCCEEDED(hres)) {
							hres = fwApps->Add(fwApp);
						}
					}
				}
			}
		} else if(fwApps != NULL) {
			if(SUCCEEDED(hres)) {
				if(fwAppEnabled != VARIANT_FALSE) {
					hres = fwApps->Remove(CComBSTR(Text::toT(WinUtil::getAppName()).c_str()));
				}
			}
		}

		// Release the authorized application instance.
		if(fwApp != NULL) {
			fwApp->Release();
		}

		// Release the authorized application collection.
		if(fwApps != NULL) {
			fwApps->Release();
		}
	}

	// Release the firewall profile.
	if (fwProfile != NULL) {
		fwProfile->Release();
	}

	if(FAILED(hres)) {
		dcdebug("WinUtil::alterWinFirewall failed: 0x%08lx\n", hres);
	}
}

void WinUtil::createAddRemoveEntry() {
	// Register uninstaller
	HKEY hk;
	if(RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ApexDC++"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS) {
		// Values to be set
		const tstring& displayIcon = Text::toT(getAppName());
		const tstring& displayName = _T(APPNAME) _T(" - Pinnacle of File Sharing");
		const tstring& displayVersion = _T(VERSIONSTRING);
		const tstring& uninstallString = _T("\"") + Text::toT(getAppName()) + _T("\" /uninstall");
		const tstring& url = _T(HOMEPAGE);
		DWORD disableFunctions = 1;

		RegSetValueEx(hk, _T("DisplayIcon"), NULL, REG_SZ, (LPBYTE)displayIcon.c_str(), (displayIcon.length() + 1) * sizeof(TCHAR));
		RegSetValueEx(hk, _T("DisplayName"), NULL, REG_SZ, (LPBYTE)displayName.c_str(), (displayName.length() + 1) * sizeof(TCHAR));
		RegSetValueEx(hk, _T("DisplayVersion"), NULL, REG_SZ, (LPBYTE)displayVersion.c_str(), (displayVersion.length() + 1) * sizeof(TCHAR));
		RegSetValueEx(hk, _T("InstallLocation"), NULL, REG_SZ, (LPBYTE)Util::getFilePath(displayIcon).c_str(), (Util::getFilePath(displayIcon).length() + 1) * sizeof(TCHAR));
		RegSetValueEx(hk, _T("NoModify"), NULL, REG_DWORD, (LPBYTE)&disableFunctions, sizeof(DWORD));
		RegSetValueEx(hk, _T("NoRepair"), NULL, REG_DWORD, (LPBYTE)&disableFunctions, sizeof(DWORD));
		RegSetValueEx(hk, _T("UninstallString"), NULL, REG_SZ, (LPBYTE)uninstallString.c_str(), (uninstallString.length() + 1) * sizeof(TCHAR));
		RegSetValueEx(hk, _T("URLInfoAbout"), NULL, REG_SZ, (LPBYTE)url.c_str(), (url.length() + 1) * sizeof(TCHAR));
		RegCloseKey(hk);
	}
}

void WinUtil::uninstallApp() {
	// Don't translate anything here

	TCHAR linkDir[MAX_PATH];
	StringList shortcuts;

	// Get all possible shorcut paths, for deletion
	shortcuts.push_back(Text::fromT((::SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, linkDir), linkDir)) + "\\ApexDC++.lnk");
	shortcuts.push_back(Text::fromT((::SHGetFolderPath(NULL, CSIDL_STARTMENU, NULL, SHGFP_TYPE_CURRENT, linkDir), linkDir)) + "\\ApexDC++.lnk");
	shortcuts.push_back(Text::fromT((::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, linkDir), linkDir)) + "\\Microsoft\\Internet Explorer\\Quick Launch\\ApexDC++.lnk");

	// Set batch file location
	const string& batLoc = Util::getTempPath() + "ApexUninstall.bat";

	// Delete shortcuts
	for(StringIter i = shortcuts.begin(); i != shortcuts.end(); ++i) {
		if(Util::fileExists(*i)) {
			File::deleteFile(*i);
		}
	}

	// We remove entry for ApexDC++
	WinUtil::alterWinFirewall(false);

	// Clean registry
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub"));
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc"));
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ApexDC++"));

	// Delete settings if possible and said to do so...
	if(Util::fileExists(Util::getFilePath(getAppName()) + "dcppboot.xml") == false) {
		if(::MessageBox(NULL, _T("Remove application settings and logs?"), _T(APPNAME) _T(" Uninstall"), MB_YESNO | MB_ICONQUESTION| MB_TOPMOST) == IDYES) {
			removeDirectory(Util::getFilePath(getAppName()) + "Settings\\");
			removeDirectory(Util::getFilePath(getAppName()) + "Logs\\");
		}
	}

	File f(batLoc, File::WRITE, File::OPEN | File::CREATE | File::TRUNCATE);
	f.setEndPos(0);

	f.write("@echo off\r\n");
	f.write(":repeat\r\n");
	f.write("del \"" + getAppName() + "\" > nul\r\n");
	f.write("If Exist \"" + getAppName() + "\" goto repeat\r\n");
	f.write("del \"" + Util::getFilePath(getAppName()) + PDB_NAME "\" > nul\r\n");
	f.write("del \"" + batLoc + "\" > nul\r\n");
	f.write("exit\r\n");
	f.close();

	::MessageBox(NULL, _T("ApexDC++ has been succesfully removed from your computer."), _T(APPNAME) _T(" Uninstall"), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);

	// Prepare for fast self deletion
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memzero(&pi, sizeof(pi));
	memzero(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	// Spawn the batch file with low-priority and suspended.
	LPTSTR cmdLine = _tcsdup(Text::toT("\"" + batLoc + "\"").c_str());
	if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE,
		CREATE_SUSPENDED | IDLE_PRIORITY_CLASS, NULL, _T("\\"), &si, &pi)) {

		// Lower the batch file's priority even more.
		SetThreadPriority(pi.hThread, THREAD_PRIORITY_IDLE);

		// Raise our priority so that we terminate as quickly as possible.
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

		// Allow the batch file to run and clean-up our handles.
		CloseHandle(pi.hProcess);
		ResumeThread(pi.hThread);
		// We want to terminate right away now so that we can be deleted
		CloseHandle(pi.hThread);
	}
	free(cmdLine);
}

bool WinUtil::isUserAdmin() {
	BOOL ret;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	ret = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
	if(ret) {
		if(!CheckTokenMembership(NULL, AdministratorsGroup, &ret)) {
			ret = FALSE;
		}
		FreeSid(AdministratorsGroup);
	}
	return (ret > 0);
}

void WinUtil::loadReBarSettings(HWND bar) {
	CReBarCtrl rebar = bar;
	
	REBARBANDINFO rbi = { 0 };
	rbi.cbSize = sizeof(rbi);
	rbi.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
	
	StringTokenizer<string> st(SETTING(TOOLBAR_SETTINGS), ';');
	StringList &sl = st.getTokens();
	
	for(StringList::const_iterator i = sl.begin(); i != sl.end(); i++)
	{
		StringTokenizer<string> stBar(*i, ',');
		StringList &slBar = stBar.getTokens();

		int nBand = rebar.IdToIndex(Util::toUInt32(slBar[1]));
		if(nBand == -1) return;

		rebar.MoveBand(nBand, i - sl.begin());
		rebar.GetBandInfo(i - sl.begin(), &rbi);
		
		rbi.cx = Util::toUInt32(slBar[0]);
		
		if(slBar[2] == "1") {
			rbi.fStyle |= RBBS_BREAK;
		} else {
			rbi.fStyle &= ~RBBS_BREAK;
		}
		
		rebar.SetBandInfo(i - sl.begin(), &rbi);		
	}
}

void WinUtil::saveReBarSettings(HWND bar) {
	string toolbarSettings;
	CReBarCtrl rebar = bar;
	
	REBARBANDINFO rbi = { 0 };
	rbi.cbSize = sizeof(rbi);
	rbi.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
	
	for(unsigned int i = 0; i < rebar.GetBandCount(); i++)
	{
		rebar.GetBandInfo(i, &rbi);
		toolbarSettings += Util::toString(rbi.cx) + "," + Util::toString(rbi.wID) + "," + Util::toString((int)(rbi.fStyle & RBBS_BREAK)) + ";";
	}
	
	SettingsManager::getInstance()->set(SettingsManager::TOOLBAR_SETTINGS, toolbarSettings);
}

string WinUtil::getReport(const Identity& identity, HWND hwnd)
{
	map<string, string> reportMap = identity.getReport();

	string report = "*** Info on " + identity.getNick() + " ***" + "\n";

	for(map<string, string>::const_iterator i = reportMap.begin(); i != reportMap.end(); ++i) {
		int width = getTextWidth(Text::toT(i->first + ":"), hwnd);
		string tabs = (width < 70) ? "\t\t\t" : (width < 135 ? "\t\t" : "\t");

		report += "\n" + i->first + ":" + tabs + i->second;// + " >>> " + Util::toString(width);
	}

	return report + "\n";
}

tstring WinUtil::getCompileDate(tstring sFormat) {
#if VERSION_DATE == 0
	COleDateTime tCompileDate; 
	tCompileDate.ParseDateTime( _T(__DATE__) _T(" ") _T(__TIME__), LOCALE_NOUSEROVERRIDE, 1033 );		
	return tCompileDate.Format(sFormat.c_str()).GetString();
#else
	return Text::toT(Util::formatTime(Text::fromT(sFormat), VERSION_DATE));
#endif
}

/**
 * @file
 * $Id$
 */
