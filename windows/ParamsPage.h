#ifndef PARAMS_PAGE_H
#define PARAMS_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class ParamsPage : public CPropertyPage<IDD_PARAMS>, public PropPage
{
public:
	ParamsPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_FAKEDETECT) + _T('\\') + TSTRING(SETTINGS_CLIENTS) + _T('\\') + TSTRING(SETTINGS_PARAMS)).c_str());
		SetTitle(title);
	};

	~ParamsPage() { 
		ctrlParams.Detach();
		free(title);
	};

	BEGIN_MSG_MAP_EX(ParamsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_CHANGE, onChange)
		NOTIFY_HANDLER(IDC_PARAM_ITEMS, NM_DBLCLK, onDblClick)
		NOTIFY_HANDLER(IDC_PARAM_ITEMS, LVN_ITEMCHANGED, onItemchangedDirectories)
		NOTIFY_HANDLER(IDC_PARAM_ITEMS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_PARAM_ITEMS, NM_CUSTOMDRAW, ctrlParams.onCustomDraw)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);

	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);


	LRESULT onDblClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

		if(item->iItem >= 0) {
			PostMessage(WM_COMMAND, IDC_CHANGE, 0);
		} else if(item->iItem == -1) {
			PostMessage(WM_COMMAND, IDC_ADD, 0);
		}

		return 0;
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:
	ExListViewCtrl ctrlParams;

	static Item items[];
	static TextItem texts[];
	TCHAR* title;
};

#endif //PARAMS_PAGE_H