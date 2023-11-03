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

#include "../client/SettingsManager.h"
#include "../client/Socket.h"

#include "GeneralPage.h"
#include "WinUtil.h"

PropPage::TextItem GeneralPage::texts[] = {
	{ IDC_SETTINGS_PERSONAL_INFORMATION, ResourceManager::SETTINGS_PERSONAL_INFORMATION },
	{ IDC_SETTINGS_NICK, ResourceManager::NICK },
	{ IDC_SETTINGS_EMAIL, ResourceManager::EMAIL },
	{ IDC_SETTINGS_DESCRIPTION, ResourceManager::DESCRIPTION },
	{ IDC_SETTINGS_UPLOAD_LINE_SPEED, ResourceManager::SETTINGS_UPLOAD_LINE_SPEED },
	{ IDC_SETTINGS_MEBIBITS, ResourceManager::MBITSPS },
	{ IDC_CHECK_SHOW_LIMITER, ResourceManager::SHOW_LIMIT },
	{ IDC_SETTINGS_DEFAULT_AWAY_MSG, ResourceManager::SETTINGS_DEFAULT_AWAY_MSG },
	{ IDC_TIME_AWAY, ResourceManager::SET_SECONDARY_AWAY },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item GeneralPage::items[] = {
	{ IDC_NICK,			SettingsManager::NICK,			PropPage::T_STR }, 
	{ IDC_EMAIL,		SettingsManager::EMAIL,			PropPage::T_STR }, 
	{ IDC_DESCRIPTION,	SettingsManager::DESCRIPTION,	PropPage::T_STR }, 
	{ IDC_CONNECTION,	SettingsManager::UPLOAD_SPEED,	PropPage::T_STR },
	{ IDC_CHECK_SHOW_LIMITER, SettingsManager::SHOW_DESCRIPTION_LIMIT, PropPage::T_BOOL },
	{ IDC_DEFAULT_AWAY_MESSAGE, SettingsManager::DEFAULT_AWAY_MESSAGE, PropPage::T_STR },
	{ IDC_SECONDARY_AWAY_MESSAGE, SettingsManager::SECONDARY_AWAY_MESSAGE, PropPage::T_STR },
	{ IDC_TIME_AWAY, SettingsManager::AWAY_TIME_THROTTLE, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

void GeneralPage::fixControls() {
	bool state = (IsDlgButtonChecked(IDC_TIME_AWAY) != 0);
	::EnableWindow(GetDlgItem(IDC_AWAY_START_TIME), state);
	::EnableWindow(GetDlgItem(IDC_AWAY_END_TIME), state);
	::EnableWindow(GetDlgItem(IDC_SECONDARY_AWAY_MESSAGE), state);
	::EnableWindow(GetDlgItem(IDC_SECONDARY_AWAY_MSG), state);
	::EnableWindow(GetDlgItem(IDC_AWAY_TO), state);
}

void GeneralPage::write()
{
	PropPage::write((HWND)(*this), items);

	settings->set(SettingsManager::AWAY_START, timeCtrlBegin.GetCurSel());
	settings->set(SettingsManager::AWAY_END, timeCtrlEnd.GetCurSel());
}

LRESULT GeneralPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	ctrlConnection.Attach(GetDlgItem(IDC_CONNECTION));
	
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i)
		ctrlConnection.AddString(Text::toT(*i).c_str());

	timeCtrlBegin.Attach(GetDlgItem(IDC_AWAY_START_TIME));
	timeCtrlEnd.Attach(GetDlgItem(IDC_AWAY_END_TIME));

	timeCtrlBegin.AddString(CTSTRING(MIDNIGHT));
	timeCtrlEnd.AddString(CTSTRING(MIDNIGHT));
	for (int i = 1; i < 12; ++i)
	{
		timeCtrlBegin.AddString((Util::toStringT(i) + CTSTRING(AM)).c_str());
		timeCtrlEnd.AddString((Util::toStringT(i) + CTSTRING(AM)).c_str());
	}
	timeCtrlBegin.AddString(CTSTRING(NOON));
	timeCtrlEnd.AddString(CTSTRING(NOON));
	for (int i = 1; i < 12; ++i)
	{
		timeCtrlBegin.AddString((Util::toStringT(i) + CTSTRING(PM)).c_str());
		timeCtrlEnd.AddString((Util::toStringT(i) + CTSTRING(PM)).c_str());
	}

	PropPage::read((HWND)(*this), items);

	fixControls();

	ctrlConnection.SetCurSel(ctrlConnection.FindString(0, Text::toT(SETTING(UPLOAD_SPEED)).c_str()));
	timeCtrlBegin.SetCurSel(SETTING(AWAY_START));
	timeCtrlEnd.SetCurSel(SETTING(AWAY_END));

	ctrlConnection.Detach();
	
	nick.Attach(GetDlgItem(IDC_NICK));
	nick.LimitText(35);
	nick.Detach();
	desc.Attach(GetDlgItem(IDC_DESCRIPTION));
	desc.LimitText(35);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SETTINGS_EMAIL));
	desc.LimitText(35);
	desc.Detach();
	return TRUE;
}

LRESULT GeneralPage::onTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	TCHAR buf[SETTINGS_BUF_LEN];

	GetDlgItemText(wID, buf, SETTINGS_BUF_LEN);
	tstring old = buf;

	// Strip '$', '|', '<', '>' and ' ' from text
	TCHAR *b = buf, *f = buf, c;
	while( (c = *b++) != 0 )
	{
		if(c != '$' && c != '|' && (wID == IDC_DESCRIPTION || c != ' ') && ( (wID != IDC_NICK && wID != IDC_DESCRIPTION && wID != IDC_SETTINGS_EMAIL) || (c != '<' && c != '>') ) )
			*f++ = c;
	}

	*f = '\0';

	if(old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf);
		if(start > 0) start--;
		if(end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}

	return TRUE;
}

/**
 * @file
 * $Id$
 */
