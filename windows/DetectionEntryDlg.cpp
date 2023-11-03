/*
 * Copyright (C) 2006-2011 Crise, crise<at>mail.berlios.de
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

#include "DetectionEntryDlg.h"
#include "Resource.h"
#include "WinUtil.h"
#include "ParamDlg.h"

#undef GET_TEXT
#define GET_TEXT(ctrl, var) \
	len = ctrl.GetWindowTextLength() + 1; \
	buf.resize(len); \
	buf.resize(ctrl.GetWindowText(&buf[0], len)); \
	var = Text::fromT(buf);

#undef ATTACH
#define ATTACH(id, var) var.Attach(GetDlgItem(id))

LRESULT DetectionEntryDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	if(origId < 1) {
		::EnableWindow(GetDlgItem(IDC_BACK), false);
		::EnableWindow(GetDlgItem(IDC_NEXT), false);
	}

	ATTACH(IDC_NAME, ctrlName);
	ATTACH(IDC_COMMENT, ctrlComment);
	ATTACH(IDC_CHEAT, ctrlCheat);
	ATTACH(IDC_PARAMS, ctrlParams);
	ATTACH(IDC_RAW, ctrlRaw);
	ATTACH(IDC_LEVEL, ctrlLevel);
	ATTACH(IDC_INFMAP_TYPE, ctrlProtocol);
	ATTACH(IDC_REGEX_TESTER, ctrlExpTest);

	CRect rc;
	ctrlParams.GetClientRect(rc);
	ctrlParams.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width() / 10, 0);
	ctrlParams.InsertColumn(1, CTSTRING(REGEXP), LVCFMT_LEFT, ((rc.Width() / 10) * 9) - GetSystemMetrics(SM_CXVSCROLL), 1);
	ctrlParams.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	createList();
	for(Iter i = idAction.begin(); i != idAction.end(); ++i) {
		ctrlRaw.AddString(RawManager::getInstance()->getNameActionId(i->second).c_str());
	}

	ctrlLevel.AddString(_T("Green"));
	ctrlLevel.AddString(_T("Yellow"));
	ctrlLevel.AddString(_T("Red"));

	ctrlProtocol.AddString(_T("Shared Fields"));
	ctrlProtocol.AddString(_T("NMDC Fields"));
	ctrlProtocol.AddString(_T("ADC Fields"));
	ctrlProtocol.SetCurSel(0);

	updateControls();

	CenterWindow(GetParent());
	return FALSE;
}

LRESULT DetectionEntryDlg::onAdd(WORD , WORD , HWND , BOOL& ) {
	ParamDlg dlg;

	if(dlg.DoModal() == IDOK) {
		if(ctrlParams.find(Text::toT(dlg.name)) == -1) {
			TStringList lst;
			lst.push_back(Text::toT(dlg.name));
			lst.push_back(Text::toT(dlg.regexp));
			ctrlParams.insert(lst);
				switch(ctrlProtocol.GetCurSel()) {
					case 0: sharedMap.push_back(make_pair(dlg.name, dlg.regexp)); break;
					case 1: nmdcMap.push_back(make_pair(dlg.name, dlg.regexp)); break;
					case 2: adcMap.push_back(make_pair(dlg.name, dlg.regexp)); break;
				}
		} else {
			::MessageBox(m_hWnd, CTSTRING(PARAM_EXISTS), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK);
		}
	}
	return 0;
}

LRESULT DetectionEntryDlg::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_CHANGE), ctrlParams.GetItemState(lv->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_REMOVE), ctrlParams.GetItemState(lv->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_MATCH), ctrlParams.GetItemState(lv->iItem, LVIS_SELECTED) && ctrlParams.GetSelectedCount() == 1);
	return 0;
}

LRESULT DetectionEntryDlg::onChange(WORD , WORD , HWND , BOOL& ) {
	if(ctrlParams.GetSelectedCount() == 1) {
		int sel = ctrlParams.GetSelectedIndex();
		TCHAR buf[1024];
		ParamDlg dlg;
		ctrlParams.GetItemText(sel, 0, buf, 1024);
		dlg.name = Text::fromT(buf);
		ctrlParams.GetItemText(sel, 1, buf, 1024);
		dlg.regexp = Text::fromT(buf);

		StringPair oldData = make_pair(dlg.name, dlg.regexp);
		if(dlg.DoModal() == IDOK) {
			int idx = ctrlParams.find(Text::toT(dlg.name));
			if(idx == -1 || idx == sel) {
				DetectionEntry::INFMap::iterator i;
				ctrlParams.SetItemText(sel, 0, Text::toT(dlg.name).c_str());
				ctrlParams.SetItemText(sel, 1, Text::toT(dlg.regexp).c_str());
				switch(ctrlProtocol.GetCurSel()) {
					case 0:
						i = std::find(sharedMap.begin(), sharedMap.end(), oldData);
						i->first = dlg.name;
						i->second = dlg.regexp;
						break;
					case 1:
						i = std::find(nmdcMap.begin(), nmdcMap.end(), oldData);
						i->first = dlg.name;
						i->second = dlg.regexp;
						break;
					case 2:
						i = std::find(adcMap.begin(), adcMap.end(), oldData);
						i->first = dlg.name;
						i->second = dlg.regexp;
						break;
				}
			} else {
				::MessageBox(m_hWnd, CTSTRING(PARAM_EXISTS), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK);
			}
		}
	}
	return 0;
}

LRESULT DetectionEntryDlg::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlParams.GetSelectedCount() == 1) {
		DetectionEntry::INFMap::iterator i;
		string name, regexp;

		int sel = ctrlParams.GetNextItem(-1, LVNI_SELECTED);
		TCHAR buf[1024];
		ctrlParams.GetItemText(sel, 0, buf, 1024);
		name = Text::fromT(buf);
		ctrlParams.GetItemText(sel, 1, buf, 1024);
		regexp = Text::fromT(buf);

		StringPair oldData = make_pair(name, regexp);
		ctrlParams.DeleteItem(sel);
		switch(ctrlProtocol.GetCurSel()) {
			case 0:
				i = std::find(sharedMap.begin(), sharedMap.end(), oldData);
				sharedMap.erase(i);
				break;
			case 1:
				i = std::find(nmdcMap.begin(), nmdcMap.end(), oldData);
				nmdcMap.erase(i);
				break;
			case 2:
				i = std::find(adcMap.begin(), adcMap.end(), oldData);
				adcMap.erase(i);
				break;
		}
	}
	return 0;
}

LRESULT DetectionEntryDlg::onNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	updateVars();
	try {
		DetectionManager::getInstance()->updateDetectionItem(origId, curEntry);
	} catch(const Exception& e) {
		::MessageBox(m_hWnd, Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		return 0;
	}

	if(DetectionManager::getInstance()->getNextDetectionItem(curEntry.Id, ((wID == IDC_NEXT) ? 1 : -1), curEntry) && idChanged) {
		idChanged = false;
		::EnableWindow(GetDlgItem(IDC_DETECT_ID), false);
		::EnableWindow(GetDlgItem(IDC_ID_EDIT), true);
	}

	origId = curEntry.Id;
	ctrlParams.DeleteAllItems();
	ctrlProtocol.SetCurSel(0);

	updateControls();
	return 0;
}

LRESULT DetectionEntryDlg::onMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlParams.GetSelectedCount() == 1) {
		TCHAR buf[1024]; // Note: TCHAR required!
		tstring::size_type i, j, k;

		ctrlExpTest.GetWindowText(buf, 1024);
		tstring test = buf;
		ctrlParams.GetItemText(ctrlParams.GetSelectedIndex(), 1, buf, 1024);
		tstring formattedExp = buf;

		// Clean up from params
		const StringMap& params = DetectionManager::getInstance()->getParams();

		i = 0;
		while((j = formattedExp.find(_T("%["), i)) != tstring::npos) {
			if((formattedExp.size() < j + 2) || ((k = formattedExp.find(_T(']'), j + 2)) == tstring::npos)) {
				break;
			}

			string name = Text::fromT(formattedExp.substr(j + 2, k - j - 2));
			StringMap::const_iterator smi = params.find(name);
			if(smi != params.end()) {
				formattedExp.replace(j, k-j + 1, Text::toT(smi->second));
				i = j + smi->second.size();
			} else {
				formattedExp.replace(j, k-j + 1, _T(".*"));
				i = j + 2;
			}
		}

		::MessageBox(m_hWnd, matchRegExp(formattedExp, test).c_str(), CTSTRING(REGEXP_TESTER), MB_ICONINFORMATION | MB_OK);
	}
	return 0;
}

LRESULT DetectionEntryDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) { 
		updateVars();
		try {
			if(origId < 1) {
				DetectionManager::getInstance()->addDetectionItem(curEntry);
			} else {
				DetectionManager::getInstance()->updateDetectionItem(origId, curEntry);
			}
		} catch(const Exception& e) {
			::MessageBox(m_hWnd, Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
			return 0;
		}
	}

	EndDialog(wID);
	return 0;
}

tstring DetectionEntryDlg::matchRegExp(const tstring& aExp, const tstring& aPattern) {
	try {
		const boost::wregex reg(aExp);
		if(boost::regex_search(aPattern.begin(), aPattern.end(), reg)) {
			return TSTRING(REGEXP_MATCH);
		} else {
			 return TSTRING(REGEXP_MISMATCH);
		}
	} catch(const boost::regex_error& e) {
		return Text::toT(STRING(REGEXP_INVALID) + "(Error: " + e.what() + ")");
	}
}

void DetectionEntryDlg::updateVars() {
	tstring buf;
	int len;

	GET_TEXT(ctrlName, curEntry.name);
	GET_TEXT(ctrlComment, curEntry.comment);
	GET_TEXT(ctrlCheat, curEntry.cheat);

	// params...
	curEntry.defaultMap.clear();
	curEntry.defaultMap = sharedMap;
	sharedMap.clear();

	curEntry.nmdcMap.clear();
	curEntry.nmdcMap = nmdcMap;
	nmdcMap.clear();

	curEntry.adcMap.clear();
	curEntry.adcMap = adcMap;
	adcMap.clear();

	if(idChanged) {
		len = ::GetWindowTextLength(GetDlgItem(IDC_DETECT_ID)) + 1;
		buf.resize(len);
		buf.resize(GetDlgItemText(IDC_DETECT_ID, &buf[0], len));
		uint32_t newId = Util::toUInt32(Text::fromT(buf).c_str());
		if(newId != origId) curEntry.Id = newId;
	}

	curEntry.checkMismatch = IsDlgButtonChecked(IDC_CHECK_MISMATCH) == BST_CHECKED;
	curEntry.isEnabled = IsDlgButtonChecked(IDC_ENABLE) == BST_CHECKED;
	curEntry.rawToSend = getIdAction(ctrlRaw.GetCurSel());
	curEntry.clientFlag = ctrlLevel.GetCurSel() + 1;
}

void DetectionEntryDlg::updateControls() {
	if(origId < 0) return;
	ctrlName.SetWindowText(Text::toT(curEntry.name).c_str());
	ctrlComment.SetWindowText(Text::toT(curEntry.comment).c_str());
	ctrlCheat.SetWindowText(Text::toT(curEntry.cheat).c_str());

	// params...
	if(!curEntry.defaultMap.empty()) {
		TStringList cols;
		sharedMap = curEntry.defaultMap;
		if(ctrlProtocol.GetCurSel() == 0) {
			for(DetectionEntry::INFMap::const_iterator j = sharedMap.begin(); j != sharedMap.end(); ++j) {
				cols.push_back(Text::toT(j->first));
				cols.push_back(Text::toT(j->second));
				ctrlParams.insert(cols);
				cols.clear();
			}
		}
	}

	if(!curEntry.nmdcMap.empty()) {
		TStringList cols;
		nmdcMap = curEntry.nmdcMap;
		if(ctrlProtocol.GetCurSel() == 1) {
			for(DetectionEntry::INFMap::const_iterator j = nmdcMap.begin(); j != nmdcMap.end(); ++j) {
				cols.push_back(Text::toT(j->first));
				cols.push_back(Text::toT(j->second));
				ctrlParams.insert(cols);
				cols.clear();
			}
		}
	}

	if(!curEntry.adcMap.empty()) {
		TStringList cols;
		adcMap = curEntry.adcMap;
		if(ctrlProtocol.GetCurSel() == 2) {
			for(DetectionEntry::INFMap::const_iterator j = adcMap.begin(); j != adcMap.end(); ++j) {
				cols.push_back(Text::toT(j->first));
				cols.push_back(Text::toT(j->second));
				ctrlParams.insert(cols);
				cols.clear();
			}
		}
	}

	SetDlgItemText(IDC_DETECT_ID, Util::toStringT(curEntry.Id).c_str());

	CheckDlgButton(IDC_CHECK_MISMATCH, curEntry.checkMismatch ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_ENABLE, curEntry.isEnabled ? BST_CHECKED : BST_UNCHECKED);
	ctrlRaw.SetCurSel(getId(curEntry.rawToSend));
	ctrlLevel.SetCurSel(curEntry.clientFlag - 1);

	{
		BOOL h = FALSE;
		onEnable(0,0,0,h);
	}
}

LRESULT DetectionEntryDlg::onEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool state = IsDlgButtonChecked(IDC_ENABLE) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_CHECK_MISMATCH), state);
	::EnableWindow(GetDlgItem(IDC_RAW), state);
	::EnableWindow(GetDlgItem(IDC_INFMAP_TYPE), state);
	::EnableWindow(GetDlgItem(IDC_LEVEL), state);
	::EnableWindow(GetDlgItem(IDC_PARAMS), state);
	::EnableWindow(GetDlgItem(IDC_CHEAT), state);
	::EnableWindow(GetDlgItem(IDC_COMMENT), state);
	::EnableWindow(GetDlgItem(IDC_NAME), state);
	::EnableWindow(GetDlgItem(IDC_ADD), state);
	::EnableWindow(GetDlgItem(IDC_REMOVE), state);
	::EnableWindow(GetDlgItem(IDC_CHANGE), state);
	::EnableWindow(GetDlgItem(IDC_MATCH), state && ctrlParams.GetSelectedCount() == 1);
	::EnableWindow(GetDlgItem(IDC_REGEX_TESTER), state);
	return 0;
}
