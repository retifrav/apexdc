/* 
 * Copyright (C) 2005-2005 Virus27, Virus27@free.fr
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

#include "RawDlg.h"

LRESULT RawDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SetDlgItemText(IDC_RAW_NAME, CTSTRING(SETTINGS_NAME));

	ctrlName.Attach(GetDlgItem(IDC_NAME));
	ctrlRaw.Attach(GetDlgItem(IDC_RAW));
	ctrlTime.Attach(GetDlgItem(IDC_TIME));

	ctrlName.SetWindowText(Text::toT(name).c_str());
	ctrlRaw.SetWindowText(Text::toT(raw).c_str());
	ctrlTime.SetWindowText(Util::toStringT(time).c_str());
	
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT RawDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		TCHAR buf[512];
		GetDlgItemText(IDC_NAME, buf, 256);
		name = Text::fromT(buf);
		GetDlgItemText(IDC_RAW, buf, 512);
		raw = Text::fromT(buf);
		GetDlgItemText(IDC_TIME, buf, 256);
		time = Util::toInt(Text::fromT(buf));
	}
	EndDialog(wID);
	return 0;
}


