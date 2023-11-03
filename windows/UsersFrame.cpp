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

#include "../client/StringTokenizer.h"

#include "UsersFrame.h"
#include "LineDlg.h"
#include "HubFrame.h"
#include "TextFrame.h"
#include "ResourceLoader.h"

int UsersFrame::columnIndexes[] = { COLUMN_NICK, COLUMN_HUB, COLUMN_SEEN, COLUMN_DESCRIPTION, COLUMN_SUPERUSER };
int UsersFrame::columnSizes[] = { 200, 300, 150, 200, 100 };
static ResourceManager::Strings columnNames[] = { ResourceManager::AUTO_GRANT, ResourceManager::LAST_HUB, ResourceManager::LAST_SEEN, ResourceManager::DESCRIPTION, ResourceManager::SUPER_USER };

LRESULT UsersFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_USERS);
	ctrlUsers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	ResourceLoader::LoadImageList(IDR_FAVSTATES, images, 16, 16);
	ctrlUsers.SetImageList(images, LVSIL_SMALL);

	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(WinUtil::textColor);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(USERSFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(USERSFRAME_WIDTHS), COLUMN_LAST);
	
	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		ctrlUsers.InsertColumn(j, CTSTRING_I(columnNames[j]), LVCFMT_LEFT, columnSizes[j], j);
	}
	
	ctrlUsers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlUsers.setSortColumn(COLUMN_NICK);

	FavoriteManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	FavoriteManager::FavoriteMap ul = FavoriteManager::getInstance()->getFavoriteUsers();
	ctrlUsers.SetRedraw(FALSE);
	for(FavoriteManager::FavoriteMap::const_iterator i = ul.begin(); i != ul.end(); ++i) {
		addUser(i->second);
	}
	ctrlUsers.SetRedraw(TRUE);

	startup = false;

	bHandled = FALSE;
	return TRUE;

}

LRESULT UsersFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlUsers && ctrlUsers.GetSelectedCount() > 0 ) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlUsers, pt);
		}
	
		OMenu usersMenu;
		usersMenu.CreatePopupMenu();
		usersMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CTSTRING(OPEN_USER_LOG));
		usersMenu.AppendMenu(MF_SEPARATOR);
		appendUserItems(usersMenu, Util::emptyString); // TODO: hubhint
		usersMenu.AppendMenu(MF_SEPARATOR);
		usersMenu.AppendMenu(MF_STRING, IDC_EDIT, CTSTRING(PROPERTIES));
		usersMenu.AppendMenu(MF_STRING, IDC_SUPER_USER, CTSTRING(SUPER_USER));
		usersMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
		
		tstring x;
		if (ctrlUsers.GetSelectedCount() == 1) {
			x = ctrlUsers.getItemData(WinUtil::getFirstSelectedIndex(ctrlUsers))->columns[COLUMN_NICK];
		} else {
			x = _T("");
		}

		if (!x.empty())
			usersMenu.InsertSeparatorFirst(x);
		
		usersMenu.CheckMenuItem(IDC_SUPER_USER, MF_BYCOMMAND | MF_UNCHECKED);
		if (ctrlUsers.GetSelectedCount() == 1) {
			int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
			FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
			FavoriteUser & u = favUsers.find(ctrlUsers.getItemData(i)->user->getCID())->second;
			if (u.isSet(FavoriteUser::FLAG_SUPERUSER))
				usersMenu.CheckMenuItem(IDC_SUPER_USER, MF_BYCOMMAND | MF_CHECKED);
		}
		
		usersMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		return TRUE; 
	}
	bHandled = FALSE;
	return FALSE; 
}
	
void UsersFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
		
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[3];
		ctrlStatus.GetClientRect(sr);
		int tmp = (sr.Width()) > 316 ? 216 : ((sr.Width() > 116) ? sr.Width()-100 : 16);
			
		w[0] = sr.right - tmp;
		w[1] = w[0] + (tmp-16)/2;
		w[2] = w[0] + (tmp-16);
			
		ctrlStatus.SetParts(3, w);
	}
		
	CRect rc = rect;
	ctrlUsers.MoveWindow(rc);
}

LRESULT UsersFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ctrlUsers.getItemData(i)->remove();
	}
	return 0;
}

LRESULT UsersFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlUsers.GetSelectedCount() == 1) {
		int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
		UserInfo* ui = ctrlUsers.getItemData(i);
		dcassert(i != -1);
		LineDlg dlg;
		dlg.description = TSTRING(DESCRIPTION);
		dlg.title = ui->columns[COLUMN_NICK];
		dlg.line = ui->columns[COLUMN_DESCRIPTION];
		if(dlg.DoModal(m_hWnd)) {
			FavoriteManager::getInstance()->setUserDescription(ui->user, Text::fromT(dlg.line));
			ui->columns[COLUMN_DESCRIPTION] = dlg.line;
			ctrlUsers.updateItem(i);
		}	
	}
	return 0;
}

LRESULT UsersFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	if(!startup && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		FavoriteManager::getInstance()->setAutoGrant(ctrlUsers.getItemData(l->iItem)->user, ctrlUsers.GetCheckState(l->iItem) != FALSE);
 	}
  	return 0;
}

LRESULT UsersFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		PostMessage(WM_COMMAND, IDC_GETLIST, 0);
	} else {
		bHandled = FALSE;
	}

	return 0;
}

LRESULT UsersFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	case VK_RETURN:
		PostMessage(WM_COMMAND, IDC_EDIT, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

void UsersFrame::addUser(const FavoriteUser& aUser) {
	int i = ctrlUsers.insertItem(new UserInfo(aUser), 0);
	bool b = aUser.isSet(FavoriteUser::FLAG_GRANTSLOT);
	ctrlUsers.SetCheckState(i, b);
	updateUser(aUser.getUser());
}

void UsersFrame::updateUser(const UserPtr& aUser) {
	for(int i = 0; i < ctrlUsers.GetItemCount(); ++i) {
		UserInfo *ui = ctrlUsers.getItemData(i);
		if(ui->user == aUser) {
			ui->columns[COLUMN_SEEN] = aUser->isOnline() ? TSTRING(ONLINE) : Text::toT(Util::formatTime("%Y-%m-%d %H:%M", FavoriteManager::getInstance()->getLastSeen(aUser)));
			if(aUser->isOnline()) {
				// TODO: if(aUser->isSet(User::AWAY))
				//	ctrlUsers.SetItem(i,0,LVIF_IMAGE, NULL, 1, 0, 0, NULL);
				//else
					ctrlUsers.SetItem(i,0,LVIF_IMAGE, NULL, 0, 0, 0, NULL);
			} else
				ctrlUsers.SetItem(i,0,LVIF_IMAGE, NULL, 2, 0, 0, NULL);
			ctrlUsers.updateItem(i);
		}
	}
}

void UsersFrame::removeUser(const FavoriteUser& aUser) {
	for(int i = 0; i < ctrlUsers.GetItemCount(); ++i) {
		const UserInfo *ui = ctrlUsers.getItemData(i);
		if(ui->user == aUser.getUser()) {
			ctrlUsers.DeleteItem(i);
			delete ui;
			return;
		}
	}
}

LRESULT UsersFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FavoriteManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_FAVUSERS, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlUsers, SettingsManager::USERSFRAME_ORDER, 
			SettingsManager::USERSFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
	
		for(int i = 0; i < ctrlUsers.GetItemCount(); ++i) {
			delete ctrlUsers.getItemData(i);
		}

		bHandled = FALSE;
		return 0;
	}
}

void UsersFrame::UserInfo::update(const FavoriteUser& u) {
	columns[COLUMN_NICK] = Text::toT(u.getNick());
	columns[COLUMN_HUB] = user->isOnline() ? WinUtil::getHubNames(u.getUser(), Util::emptyString).first : Text::toT(u.getUrl());
	columns[COLUMN_SEEN] = user->isOnline() ? TSTRING(ONLINE) : Text::toT(Util::formatTime("%Y-%m-%d %H:%M", u.getLastSeen()));
	columns[COLUMN_DESCRIPTION] = Text::toT(u.getDescription());
	columns[COLUMN_SUPERUSER] = u.isSet(FavoriteUser::FLAG_SUPERUSER) ? CTSTRING(YES) : CTSTRING(NO);
}

LRESULT UsersFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == USER_UPDATED) {
		updateUser(((Identity*)lParam)->getUser());
	}
	return 0;
}
			
LRESULT UsersFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlUsers.GetSelectedCount() == 1) {
		int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
		const UserInfo* ui = ctrlUsers.getItemData(i);
		dcassert(i != -1);

		StringMap params;
		params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(ui->getUser()->getCID(), Util::emptyString));
		params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(ui->getUser()->getCID(), Util::emptyString));
		params["userCID"] = ui->getUser()->getCID().toBase32(); 
		params["userNI"] = Util::toString(ClientManager::getInstance()->getNicks(ui->getUser()->getCID(), Util::emptyString));
		params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();

		string file = Util::validateFileName(LogManager::getInstance()->getPath(LogManager::PM, params));
		if(Util::fileExists(file)) {
			if(BOOLSETTING(OPEN_LOGS_INTERNAL) == false) {
				ShellExecute(NULL, NULL, Text::toT(file).c_str(), NULL, NULL, SW_SHOWNORMAL);
			} else {
				TextFrame::openWindow(Text::toT(file).c_str());
			}
		} else {
			MessageBox(CTSTRING(NO_LOG_FOR_USER), CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
		}	
	}
		return 0;
}

LRESULT UsersFrame::onSuperUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
   int i = -1;
   while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
	   UserInfo *ui = ctrlUsers.getItemData(i);
	   FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
	   FavoriteUser & u = favUsers.find(ui->user->getCID())->second;
	   ui->columns[COLUMN_SUPERUSER] = u.isSet(FavoriteUser::FLAG_SUPERUSER) ? CTSTRING(NO) : CTSTRING(YES);
	   FavoriteManager::getInstance()->setSuperUser(u.getUser(), !u.isSet(FavoriteUser::FLAG_SUPERUSER));
	   ctrlUsers.updateItem(i);
   }
   return 0;
}

void UsersFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlUsers.GetBkColor() != WinUtil::bgColor) {
		ctrlUsers.SetBkColor(WinUtil::bgColor);
		ctrlUsers.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlUsers.GetTextColor() != WinUtil::textColor) {
		ctrlUsers.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

/**
 * @file
 * $Id$
 */
