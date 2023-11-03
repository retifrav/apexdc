#include "stdafx.h"
#include "Resource.h"

#include <client/SettingsManager.h>
#include <client/DetectionManager.h>
#include <client/version.h>

#include "ParamsPage.h"
#include "ParamDlg.h"

#define BUFLEN 1024

PropPage::TextItem ParamsPage::texts[] = {
	{ IDC_ADD, ResourceManager::ADD },
	{ IDC_CHANGE, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE, ResourceManager::REMOVE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item ParamsPage::items[] = {
	{ 0, 0, PropPage::T_END }
};

LRESULT ParamsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CRect rc;

	ctrlParams.Attach(GetDlgItem(IDC_PARAM_ITEMS));
	ctrlParams.GetClientRect(rc);

	ctrlParams.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width() / 3, 0);
	ctrlParams.InsertColumn(1, CTSTRING(REGEXP), LVCFMT_LEFT, (rc.Width() / 3) * 2, 1);

	ctrlParams.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	// Do specialized reading here

	StringMap& pm = DetectionManager::getInstance()->getParams();
	TStringList cols;
	for(StringMap::iterator j = pm.begin(); j != pm.end(); ++j) {
		cols.push_back(Text::toT(j->first));
		cols.push_back(Text::toT(j->second));
		ctrlParams.insert(cols);
		cols.clear();
	}

	return TRUE;
}

LRESULT ParamsPage::onAdd(WORD , WORD , HWND , BOOL& ) {
	ParamDlg dlg;

	if(dlg.DoModal() == IDOK) {
		if(ctrlParams.find(Text::toT(dlg.name)) == -1) {
			TStringList lst;
			lst.push_back(Text::toT(dlg.name));
			lst.push_back(Text::toT(dlg.regexp));
			ctrlParams.insert(lst);
		} else {
			MessageBox(CTSTRING(PARAM_EXISTS), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK);
		}
	}
	return 0;
}

LRESULT ParamsPage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_CHANGE), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	return 0;
}

LRESULT ParamsPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_ADD, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT ParamsPage::onChange(WORD , WORD , HWND , BOOL& ) {
	if(ctrlParams.GetSelectedCount() == 1) {
		int sel = ctrlParams.GetSelectedIndex();
		TCHAR buf[BUFLEN];
		ParamDlg dlg;
		ctrlParams.GetItemText(sel, 0, buf, BUFLEN);
		dlg.name = Text::fromT(buf);
		ctrlParams.GetItemText(sel, 1, buf, BUFLEN);
		dlg.regexp = Text::fromT(buf);

		if(dlg.DoModal() == IDOK) {
			int idx = ctrlParams.find(Text::toT(dlg.name));
			if(idx == -1 || idx == sel) {
				ctrlParams.SetItemText(sel, 0, Text::toT(dlg.name).c_str());
				ctrlParams.SetItemText(sel, 1, Text::toT(dlg.regexp).c_str());
			} else {
				MessageBox(CTSTRING(PARAM_EXISTS), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK);
			}
		}
	}
	return 0;
}

LRESULT ParamsPage::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlParams.GetSelectedCount() == 1) {
		ctrlParams.DeleteItem(ctrlParams.GetNextItem(-1, LVNI_SELECTED));
	}
	return 0;
}

void ParamsPage::write() {
	int it = ctrlParams.GetItemCount();
	TCHAR buf[BUFLEN];
	string name, regexp;
	StringMap& pm = DetectionManager::getInstance()->getParams();
	pm.clear();
	for(int i = 0; i < it; ++i) {
		ctrlParams.GetItemText(i, 0, buf, BUFLEN);
		name = Text::fromT(buf);
		ctrlParams.GetItemText(i, 1, buf, BUFLEN);
		regexp = Text::fromT(buf);
		pm[name] = regexp;
	}
	DetectionManager::getInstance()->save();
	PropPage::write((HWND)*this, items);
}