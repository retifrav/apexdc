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

#include <client/Util.h>
#include <client/SettingsManager.h>
#include <client/version.h>

#include "RawPage.h"
#include "LineDlg.h"
#include "RawDlg.h"

PropPage::TextItem RawPage::texts[] = {
	{ IDC_RAW_PAGE_ACTION_CTN, ResourceManager::ACTION },
	{ IDC_RAW_PAGE_RAW_CTN, ResourceManager::RAW },
	{ IDC_MOVE_RAW_UP, ResourceManager::MOVE_UP },
	{ IDC_MOVE_RAW_DOWN, ResourceManager::MOVE_DOWN },
	{ IDC_ADD_ACTION, ResourceManager::ADD },
	{ IDC_RENAME_ACTION, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ IDC_REMOVE_ACTION, ResourceManager::REMOVE },
	{ IDC_ADD_RAW, ResourceManager::ADD },
	{ IDC_CHANGE_RAW, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE_RAW, ResourceManager::REMOVE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT RawPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	CRect rc;
	int scroll = GetSystemMetrics(SM_CXVSCROLL);

	ctrlAction.Attach(GetDlgItem(IDC_RAW_PAGE_ACTION));
	ctrlAction.GetClientRect(rc);
	ctrlAction.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, rc.Width() - scroll, 0);
	ctrlAction.SetExtendedListViewStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	Action::List lst = RawManager::getInstance()->getActionList();

	for(Action::Iter i = lst.begin(); i != lst.end(); ++i) {
		addEntryAction(i->first, i->second->getName(), i->second->getActive(), ctrlAction.GetItemCount());
	}

	ctrlRaw.Attach(GetDlgItem(IDC_RAW_PAGE_RAW));
	ctrlRaw.GetClientRect(rc);
	ctrlRaw.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, rc.Width() - scroll, 0);
	ctrlRaw.SetExtendedListViewStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	nosave = false;

	return TRUE;
}

LRESULT RawPage::onAddAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		LineDlg name;
		name.title = TSTRING(ADD_ACTION);
		name.description = TSTRING(SETTINGS_NAME);
		name.line = TSTRING(ACTION);
		if(name.DoModal(m_hWnd) == IDOK) {
			addEntryAction(RawManager::getInstance()->addAction(0,	Text::fromT(name.line),	true), Text::fromT(name.line), true, ctrlAction.GetItemCount(), true);
		}
	} catch(const Exception& e) {
		MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
	}

	return 0;
}

LRESULT RawPage::onRenameAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];
	LVITEM item;
	memzero(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;

	if(ctrlAction.GetSelectedCount() == 1) {
		int i = ctrlAction.GetNextItem(-1, LVNI_SELECTED);
		item.iItem = i;
		item.iSubItem = 0;
		ctrlAction.GetItem(&item);

		try {
			LineDlg name;
			name.title = TSTRING(ADD_ACTION);
			name.description = TSTRING(SETTINGS_NAME);
			name.line = tstring(buf);
			if(name.DoModal(m_hWnd) == IDOK) {
				if (stricmp(buf, name.line.c_str()) != 0) {
					RawManager::getInstance()->renameAction(Text::fromT(buf), Text::fromT(name.line));
					ctrlAction.SetItemText(i, 0, name.line.c_str());
				}
			}
		} catch(const Exception& e) {
			MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		}
	}

	return 0;
}

LRESULT RawPage::onRemoveAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAction.GetSelectedCount() == 1) {
		int i = ctrlAction.GetNextItem(-1, LVNI_SELECTED);
		RawManager::getInstance()->removeAction(ctrlAction.GetItemData(i));
		ctrlAction.DeleteItem(i);
		ctrlAction.SelectItem(i-1);
	}

	return 0;
}

LRESULT RawPage::onAddRaw(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAction.GetSelectedCount() == 1) {
		int i = ctrlAction.GetNextItem(-1, LVNI_SELECTED);
		try {
			RawDlg raw;
			raw.name = "Raw";
			raw.raw = Util::emptyString;
			raw.time = 0;
			if(raw.DoModal(m_hWnd) == IDOK) {
				addEntryRaw(RawManager::getInstance()->addRaw(
					ctrlAction.GetItemData(i),
					raw.name,
					raw.raw,
					raw.time
					), ctrlRaw.GetItemCount(), true);
			}
		} catch(const Exception& e) {
			MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		}
	}

	return 0;
}

LRESULT RawPage::onChangeRaw(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAction.GetSelectedCount() == 1) {
		int j = ctrlAction.GetNextItem(-1, LVNI_SELECTED);
		if(ctrlRaw.GetSelectedCount() == 1) {
			int i = ctrlRaw.GetNextItem(-1, LVNI_SELECTED);
			Action::Raw rawItem;
			RawManager::getInstance()->getRawItem(ctrlAction.GetItemData(j), ctrlRaw.GetItemData(i), rawItem);

			try {
				RawDlg raw;
				string name = rawItem.getName();
				raw.name = name;
				raw.raw = rawItem.getRaw();
				raw.time = rawItem.getTime();

				if(raw.DoModal() == IDOK) {
					RawManager::getInstance()->changeRaw(ctrlAction.GetItemData(j), name, raw.name, raw.raw, raw.time);
					ctrlRaw.SetItemText(i, 0, Text::toT(raw.name).c_str());
				}
			} catch(const Exception& e) {
				MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
			}
		}
	}

	return 0;
}

LRESULT RawPage::onRemoveRaw(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAction.GetSelectedCount() == 1) {
		int j = ctrlAction.GetNextItem(-1, LVNI_SELECTED);
		if(ctrlRaw.GetSelectedCount() == 1) {
			int i = ctrlRaw.GetNextItem(-1, LVNI_SELECTED);
			RawManager::getInstance()->removeRaw(ctrlAction.GetItemData(j), ctrlRaw.GetItemData(i));
			ctrlRaw.DeleteItem(i);
			ctrlRaw.SelectItem(i-1);
		}
	}

	return 0;
}

LRESULT RawPage::onMoveRawUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAction.GetSelectedCount() == 1) {
		int j = ctrlAction.GetNextItem(-1, LVNI_SELECTED);
		int i = ctrlRaw.GetSelectedIndex();
		if(i != -1 && i != 0) {
			int n = ctrlRaw.GetItemData(i);
			RawManager::getInstance()->moveRaw(ctrlAction.GetItemData(j), n, -1);
			ctrlRaw.SetRedraw(FALSE);
			ctrlRaw.DeleteItem(i);
			Action::Raw raw;
			RawManager::getInstance()->getRawItem(ctrlAction.GetItemData(j), n, raw);
			addEntryRaw(raw, i-1);
			ctrlRaw.SelectItem(i-1);
			ctrlRaw.EnsureVisible(i-1, FALSE);
			ctrlRaw.SetRedraw(TRUE);
		}
	}
	return 0;
}

LRESULT RawPage::onMoveRawDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAction.GetSelectedCount() == 1) {
		int j = ctrlAction.GetNextItem(-1, LVNI_SELECTED);
		int i = ctrlRaw.GetSelectedIndex();
		if(i != -1 && i != (ctrlRaw.GetItemCount()-1) ) {
			int n = ctrlRaw.GetItemData(i);
			RawManager::getInstance()->moveRaw(ctrlAction.GetItemData(j), n, 1);
			ctrlRaw.SetRedraw(FALSE);
			ctrlRaw.DeleteItem(i);
			Action::Raw raw;
			RawManager::getInstance()->getRawItem(ctrlAction.GetItemData(j), n, raw);
			addEntryRaw(raw, i+1);
			ctrlRaw.SelectItem(i+1);
			ctrlRaw.EnsureVisible(i+1, FALSE);
			ctrlRaw.SetRedraw(TRUE);
		}
	}
	return 0;
}

LRESULT RawPage::onKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		if(idCtrl == IDC_RAW_PAGE_ACTION) {
			PostMessage(WM_COMMAND, IDC_ADD_ACTION, 0);
		} else if(idCtrl == IDC_RAW_PAGE_RAW) {
			PostMessage(WM_COMMAND, IDC_ADD_RAW, 0);
		}
		break;
	case VK_DELETE:
		if(idCtrl == IDC_RAW_PAGE_ACTION) {
			PostMessage(WM_COMMAND, IDC_REMOVE_ACTION, 0);
		} else if(idCtrl == IDC_RAW_PAGE_RAW) {
			PostMessage(WM_COMMAND, IDC_REMOVE_RAW, 0);
		}
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

void RawPage::addEntryAction(int id, const string name, bool active, int pos, bool select /*= false*/) {
	TStringList lst;

	lst.push_back(Text::toT(name));
	int i = ctrlAction.insert(pos, lst, 0, (LPARAM)id);
	ctrlAction.SetCheckState(i, active);
	if(select) ctrlAction.SelectItem(i);
}

void RawPage::addEntryRaw(const Action::Raw& raw, int pos, bool select /*= false*/) {
	TStringList lst;

	lst.push_back(Text::toT(raw.getName()));
	int i = ctrlRaw.insert(pos, lst, 0, (LPARAM)raw.getId());
	ctrlRaw.SetCheckState(i, raw.getActive());
	if(select) ctrlRaw.SelectItem(i);
}

LRESULT RawPage::onItemChangedAction(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	if(!nosave && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) == (l->uOldState & LVIS_STATEIMAGEMASK))) {
		int j;
		for(j = 0; j < ctrlRaw.GetItemCount(); j++) {
			RawManager::getInstance()->setActiveRaw(ctrlAction.GetItemData(l->iItem), ctrlRaw.GetItemData(j), ctrlRaw.GetCheckState(j) == 1);
		}
		ctrlRaw.SetRedraw(FALSE);
		ctrlRaw.DeleteAllItems();

		if(ctrlAction.GetSelectedCount() == 1) {
			Action::RawList lst = RawManager::getInstance()->getRawList(ctrlAction.GetItemData(l->iItem));

			for(Action::RawIter i = lst.begin(); i != lst.end(); ++i) {
				const Action::Raw& raw = *i;	
				addEntryRaw(raw, ctrlRaw.GetItemCount());
			}
		}
		ctrlRaw.SetRedraw(TRUE);
	}

	::EnableWindow(GetDlgItem(IDC_RENAME_ACTION), (l->uNewState & LVIS_FOCUSED) || ((l->uNewState & LVIS_STATEIMAGEMASK) && ctrlAction.GetSelectedIndex() != -1));
	::EnableWindow(GetDlgItem(IDC_REMOVE_ACTION), (l->uNewState & LVIS_FOCUSED) || ((l->uNewState & LVIS_STATEIMAGEMASK) && ctrlAction.GetSelectedIndex() != -1));
	::EnableWindow(GetDlgItem(IDC_RAW_PAGE_RAW),  (l->uNewState & LVIS_FOCUSED) || ((l->uNewState & LVIS_STATEIMAGEMASK) && ctrlAction.GetSelectedIndex() != -1));
	::EnableWindow(GetDlgItem(IDC_ADD_RAW),  ::IsWindowEnabled(GetDlgItem(IDC_RAW_PAGE_RAW)));
	if(!::IsWindowEnabled(GetDlgItem(IDC_RAW_PAGE_RAW)) && ::IsWindowEnabled(GetDlgItem(IDC_MOVE_RAW_UP))) {
		::EnableWindow(GetDlgItem(IDC_MOVE_RAW_UP), false);
		::EnableWindow(GetDlgItem(IDC_CHANGE_RAW), false);
		::EnableWindow(GetDlgItem(IDC_MOVE_RAW_DOWN), false);
		::EnableWindow(GetDlgItem(IDC_REMOVE_RAW), false);
	}

	return 0;
}

LRESULT RawPage::onItemChangedRaw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	::EnableWindow(GetDlgItem(IDC_MOVE_RAW_UP), (l->uNewState & LVIS_FOCUSED) || ((l->uNewState & LVIS_STATEIMAGEMASK) && ctrlRaw.GetSelectedIndex() != -1));
	::EnableWindow(GetDlgItem(IDC_CHANGE_RAW), (l->uNewState & LVIS_FOCUSED) || ((l->uNewState & LVIS_STATEIMAGEMASK) && ctrlRaw.GetSelectedIndex() != -1));
	::EnableWindow(GetDlgItem(IDC_MOVE_RAW_DOWN), (l->uNewState & LVIS_FOCUSED) || ((l->uNewState & LVIS_STATEIMAGEMASK) && ctrlRaw.GetSelectedIndex() != -1));
	::EnableWindow(GetDlgItem(IDC_REMOVE_RAW), (l->uNewState & LVIS_FOCUSED) || ((l->uNewState & LVIS_STATEIMAGEMASK) && ctrlRaw.GetSelectedIndex() != -1));

	return 0;
}

void RawPage::write() {
	int i;
	for(i = 0; i < ctrlAction.GetItemCount(); i++) {
		RawManager::getInstance()->setActiveAction(ctrlAction.GetItemData(i), ctrlAction.GetCheckState(i) == 1);
	}
	if(ctrlAction.GetSelectedCount() == 1) {
		int j = ctrlAction.GetNextItem(-1, LVNI_SELECTED);
		int l;
		for(l = 0; l < ctrlRaw.GetItemCount(); l++) {
			RawManager::getInstance()->setActiveRaw(ctrlAction.GetItemData(j), ctrlRaw.GetItemData(l), ctrlRaw.GetCheckState(l) == 1);
		}
	}

	RawManager::getInstance()->saveActionRaws();
}