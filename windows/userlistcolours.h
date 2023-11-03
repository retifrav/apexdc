#ifndef UserListColours_H
#define UserListColours_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"

class UserListColours : public CPropertyPage<IDD_USERLIST_COLOURS>, public PropPage
{
public:

	UserListColours(SettingsManager *s) : PropPage(s) { 
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_TEXT_STYLES) + _T('\\') + TSTRING(SETTINGS_USER_COLORS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	};

	~UserListColours() { free(title); };

	BEGIN_MSG_MAP(UserListColours)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_CHANGE_COLOR, BN_CLICKED, onChangeColour)
		COMMAND_HANDLER(IDC_IMAGEBROWSE, BN_CLICKED, onImageBrowse)
		COMMAND_ID_HANDLER(IDC_USERLIST_IMAGE, onEdit)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onChangeColour(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onImageBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(::GetWindowTextLength(GetDlgItem(IDC_USERLIST_IMAGE)) <= 0) {
			::EnableWindow(GetDlgItem(IDC_OLD_ICONS_MODE), false);
			::EnableWindow(GetDlgItem(IDC_ICONS_MODE), false);
		} else {
			::EnableWindow(GetDlgItem(IDC_OLD_ICONS_MODE), true);
			::EnableWindow(GetDlgItem(IDC_ICONS_MODE), true);
		}
		return 0;
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

	CRichEditCtrl n_Preview;
private:
	void BrowseForPic(int DLGITEM);

	void refreshPreview();

	CListBox n_lsbList;
	int normalColour;
	int favoriteColour;
	int reservedSlotColour;
	int ignoredColour;
	int fastColour;
	int serverColour;
	int pasiveColour;
	int opColour; 
	int protectedColour;
	int fullCheckedColour;
	int badClientColour;
	int badFilelistColour;

	CComboBox iconsMode;

protected:
	static Item items[];
	static TextItem texts[];
	TCHAR* title;
};

#endif //UserListColours_H

/**
 * @file
 * $Id$
 */