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

#include "SystemFrame.h"
#include "WinUtil.h"

LRESULT SystemFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	ctrlLog.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_CLIENT);
	ctrlLog.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	ctrlLog.SetBkColor(WinUtil::bgColor);
	ctrlLog.SetTextBkColor(WinUtil::bgColor);
	ctrlLog.SetTextColor(WinUtil::textColor);

	ctrlLog.InsertColumn(0, CTSTRING(TIME), LVCFMT_LEFT);
	ctrlLog.InsertColumn(1, CTSTRING(MESSAGE), LVCFMT_LEFT);

	// Technically, we might miss a message or two here, but who cares...
	const LogManager::List& oldMessages = LogManager::getInstance()->getLastLogs();

	LogManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	// addEntry does too much stuff not needed at this point...
	int idx = ctrlLog.GetItemCount();
	for(auto i = oldMessages.cbegin(); i != oldMessages.cend(); ++i) {
		TStringList lst;
		lst.push_back(Text::toT(Util::getShortTimeString(i->second.time)));
		lst.push_back(Text::toT(i->first));

		idx = ctrlLog.insert(idx, lst) + 1;
	}
	ctrlLog.EnsureVisible(idx, FALSE);

	ctxMenu.CreatePopupMenu();
	ctxMenu.AppendMenu(MF_STRING, IDC_COPY, CTSTRING(COPY));
	ctxMenu.AppendMenu(MF_STRING, IDC_LOG_FILE, CTSTRING(LOG_FILE));
	ctxMenu.AppendMenu(MF_STRING, IDC_AUTO_SCROLL, CTSTRING(AUTO_SCROLL));
	ctxMenu.AppendMenu(MF_SEPARATOR);
	ctxMenu.AppendMenu(MF_STRING, IDC_CLEAR, CTSTRING(CLEAR));

	bHandled = FALSE;
	return TRUE;
}

LRESULT SystemFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	SettingsManager::getInstance()->removeListener(this);
	LogManager::getInstance()->removeListener(this);

	bHandled = FALSE;
	return 0;
}

void SystemFrame::UpdateLayout(BOOL bResizeBars /*= TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	UpdateBarsPosition(rect, bResizeBars);

	CRect rc = rect;
	rc.bottom -= 1;
	rc.top += 1;
	rc.left +=1;
	rc.right -=1;

	ctrlLog.MoveWindow(rc);

	// Column sizes
	ctrlLog.SetColumnWidth(0, LVSCW_AUTOSIZE);
	ctrlLog.SetColumnWidth(1, LVSCW_AUTOSIZE);
}

LRESULT SystemFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	unique_ptr<pair<time_t, tstring> > msg((pair<time_t, tstring>*)wParam);

	addEntry(msg->first, msg->second);
	if(BOOLSETTING(BOLD_SYSTEM_LOG))
		setDirty();
	return 0;
}

LRESULT SystemFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(reinterpret_cast<HWND>(wParam) == ctrlLog.m_hWnd) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlLog, pt);
		}

		ctxMenu.EnableMenuItem(IDC_COPY, MF_BYCOMMAND | ((ctrlLog.GetSelectedCount() > 0) ? MFS_ENABLED : MFS_DISABLED));
		ctxMenu.CheckMenuItem(IDC_LOG_FILE, MF_BYCOMMAND | (BOOLSETTING(LOG_SYSTEM) ? MF_CHECKED : MF_UNCHECKED));
		ctxMenu.CheckMenuItem(IDC_AUTO_SCROLL, MF_BYCOMMAND | (scroll ? MF_CHECKED : MF_UNCHECKED));

		ctxMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);			
		return TRUE; 
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT SystemFrame::onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	int i = -1;

	while((i = ctrlLog.GetNextItem(i, LVNI_SELECTED)) != -1) {
		TCHAR buf[128];
		ctrlLog.GetItemText(i, 0, buf, 128);
		sCopy += _T("[") + tstring(buf) + _T("] ");

		tstring msgBuf;
		msgBuf.resize(128);
		int ret = 0;
		do {
			msgBuf.resize(msgBuf.size() * 2);
			ret = ctrlLog.GetItemText(i, 1, &msgBuf[0], msgBuf.size());
		} while(ret > msgBuf.size());
		sCopy += msgBuf + _T("\r\n");
	}

	if(!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}

void SystemFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlLog.GetBkColor() != WinUtil::bgColor) {
		ctrlLog.SetBkColor(WinUtil::bgColor);
		ctrlLog.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlLog.GetTextColor() != WinUtil::textColor) {
		ctrlLog.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void SystemFrame::addEntry(time_t t, const tstring& msg) {
	ctrlLog.SetRedraw(FALSE);
	int idx = ctrlLog.GetItemCount();

	// We should have some limit, so 1000 will do...
	while(idx >= 1000) {
		ctrlLog.DeleteItem(0);
		idx -= 1;
	}

	TStringList lst;
	lst.push_back(Text::toT(Util::getShortTimeString(t)));
	lst.push_back(msg);

	idx = ctrlLog.insert(idx, lst);

	// Now lets see if we need to auto scroll
	if(scroll && (idx != -1 && (ctrlLog.GetWindowLongPtr(GWL_STYLE) & WS_VSCROLL))) {
		ctrlLog.EnsureVisible(idx, FALSE);
	}

	ctrlLog.SetColumnWidth(1, LVSCW_AUTOSIZE);
	ctrlLog.SetRedraw(TRUE);
}
