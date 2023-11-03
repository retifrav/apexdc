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

#ifndef RAWPAGE_H
#define RAWPAGE_H

#include "../client/RawManager.h"

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class RawPage : public CPropertyPage<IDD_RAW_PAGE>, public PropPage
{
public:
	RawPage(SettingsManager *s) : PropPage(s), nosave(true) { 
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_FAKEDETECT) + _T('\\') + TSTRING(ACTION_RAW)).c_str());
		SetTitle(title);
	};

	~RawPage() {
		ctrlAction.Detach();
		ctrlRaw.Detach();
		free(title);
	};

	BEGIN_MSG_MAP_EX(RawPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ADD_ACTION, onAddAction)
		COMMAND_ID_HANDLER(IDC_RENAME_ACTION, onRenameAction)
		COMMAND_ID_HANDLER(IDC_REMOVE_ACTION, onRemoveAction)
		COMMAND_ID_HANDLER(IDC_ADD_RAW, onAddRaw)
		COMMAND_ID_HANDLER(IDC_CHANGE_RAW, onChangeRaw)
		COMMAND_ID_HANDLER(IDC_REMOVE_RAW, onRemoveRaw)
		COMMAND_ID_HANDLER(IDC_MOVE_RAW_UP, onMoveRawUp)
		COMMAND_ID_HANDLER(IDC_MOVE_RAW_DOWN, onMoveRawDown)
		NOTIFY_HANDLER(IDC_RAW_PAGE_ACTION, LVN_ITEMCHANGED, onItemChangedAction)
		NOTIFY_HANDLER(IDC_RAW_PAGE_RAW, LVN_ITEMCHANGED, onItemChangedRaw)
		NOTIFY_HANDLER(IDC_RAW_PAGE_ACTION, NM_DBLCLK, onDblClick)
		NOTIFY_HANDLER(IDC_RAW_PAGE_RAW, NM_DBLCLK, onDblClick)
		NOTIFY_HANDLER(IDC_RAW_PAGE_ACTION, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_RAW_PAGE_RAW, LVN_KEYDOWN, onKeyDown)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onAddAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRenameAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddRaw(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChangeRaw(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveRaw(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveRawUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveRawDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChangedAction(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onItemChangedRaw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT onDblClick(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

		if(idCtrl == IDC_RAW_PAGE_ACTION) {
			if(item->iItem >= 0) {
				PostMessage(WM_COMMAND, IDC_RENAME_ACTION, 0);
			} else if(item->iItem == -1) {
				PostMessage(WM_COMMAND, IDC_ADD_ACTION, 0);
			}
		} else if(idCtrl == IDC_RAW_PAGE_RAW) {
			if(item->iItem >= 0) {
				PostMessage(WM_COMMAND, IDC_CHANGE_RAW, 0);
			} else if(item->iItem == -1) {
				PostMessage(WM_COMMAND, IDC_ADD_RAW, 0);
			}
		}

		return 0;
	}
	
	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

protected:
	static TextItem texts[];

	ExListViewCtrl ctrlAction;
	ExListViewCtrl ctrlRaw;

	bool nosave;

	void addEntryAction(int id, const string name, bool active, int pos, bool select = false);
	void addEntryRaw(const Action::Raw& raw, int pos, bool select = false);

	TCHAR* title;
};

#endif //RAWPAGE_H