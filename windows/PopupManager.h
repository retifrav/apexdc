/* 
* Copyright (C) 2003-2005 P�r Bj�rklund, per.bjorklund@gmail.com
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

#ifndef POPUPMANAGER_H
#define POPUPMANAGER_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "stdafx.h"

#include "../client/Singleton.h"
#include "../client/TimerManager.h"

#include "PopupDlg.h"
#include "WinUtil.h"
#include "ResourceLoader.h"

#define DOWNLOAD_COMPLETE 6

class PopupManager : public Singleton< PopupManager >, private TimerManagerListener
{
public:
	PopupManager() : height(90), width(200), offset(0), id(0) {
		if(WinUtil::getOsMajor() >= 5) {
			user32lib = LoadLibrary(_T("user32"));
			_d_SetLayeredWindowAttributes = (LPFUNC)GetProcAddress(user32lib, "SetLayeredWindowAttributes");
		}
	
		TimerManager::getInstance()->addListener(this);
	}

	
	~PopupManager() {
		TimerManager::getInstance()->removeListener(this);
		hBitmap.Destroy();
		
		if(WinUtil::getOsMajor() >= 5) {
			FreeLibrary(user32lib);
		}
	}

	enum { BALLOON, CUSTOM, SPLASH, WINDOW };
	
	//call this with a preformatted message
	void Show(const tstring &aMsg, const tstring &aTitle, int Icon, HICON hIcon = NULL, bool force = false);

	//remove first popup in list and move everyone else
	void Remove(uint32_t pos = 0);

	//remove the popups that are scheduled to be removed
	void AutoRemove();

private:
	typedef list< PopupWnd* > PopupList;
	typedef PopupList::iterator PopupIter;
	PopupList popups;

	typedef BOOL (CALLBACK* LPFUNC)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
	LPFUNC _d_SetLayeredWindowAttributes;
	HMODULE user32lib;

	//size of the popup window
	uint16_t height;
	uint16_t width;

	//if we have multiple windows displayed, 
	//keep track of where the new one will be displayed
	uint16_t offset;

	//for custom popups
	ExCImage hBitmap;
	string PopupImage;
	int popuptype;

	//id of the popup to keep track of them
	uint32_t id;
 	
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t tick) noexcept;

};

#endif