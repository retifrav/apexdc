#ifndef CLIENTSPAGE_H
#define CLIENTSPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "ExListViewCtrl.h"

#include "../client/DetectionEntry.h"
#include "../client/HttpConnection.h"
#include "../client/HttpManager.h"
#include "../client/File.h"
#include "../client/DetectionManager.h"

class ClientsPage : public CPropertyPage<IDD_DETECTION_LIST_PAGE>, public PropPage {
public:
	ClientsPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_FAKEDETECT) + _T('\\') + TSTRING(SETTINGS_CLIENTS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	};

	~ClientsPage() { 
		ctrlProfiles.Detach();
		free(title);
	};

	BEGIN_MSG_MAP(ClientsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_ADD_CLIENT, onAddClient)
		COMMAND_ID_HANDLER(IDC_REMOVE_CLIENT, onRemoveClient)
		COMMAND_ID_HANDLER(IDC_CHANGE_CLIENT, onChangeClient)
		COMMAND_ID_HANDLER(IDC_MOVE_CLIENT_UP, onMoveClientUp)
		COMMAND_ID_HANDLER(IDC_MOVE_CLIENT_DOWN, onMoveClientDown)
		COMMAND_ID_HANDLER(IDC_RELOAD_CLIENTS, onReload)
		COMMAND_ID_HANDLER(IDC_UPDATE, onUpdate)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, NM_DBLCLK, onDblClick)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, LVN_GETINFOTIP, onInfoTip)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, LVN_ITEMCHANGED, onItemchangedDirectories)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, LVN_KEYDOWN, onKeyDown)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);

	LRESULT onAddClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChangeClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveClientUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveClientDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT onDblClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

		if(item->iItem >= 0) {
			PostMessage(WM_COMMAND, IDC_CHANGE_CLIENT, 0);
		} else if(item->iItem == -1) {
			PostMessage(WM_COMMAND, IDC_ADD_CLIENT, 0);
		}

		return 0;
	}

	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
private:
	ExListViewCtrl ctrlProfiles;

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	void addEntry(const DetectionEntry& de, int pos);
	void reload();
	void reloadFromHttp();

	void profilesDownload(const HttpConnection* conn, const string& downBuf, Flags::MaskType stFlags) noexcept {
		if ((stFlags & HttpManager::CONN_FAILED) == HttpManager::CONN_FAILED) {
			string msg = "Client profiles download failed.\r\n" + conn->getStatus();
			MessageBox(Text::toT(msg).c_str(), _T("Updating client profiles failed"), MB_OK);
			::EnableWindow(GetDlgItem(IDC_UPDATE), true);
		} else if(!downBuf.empty()) {
			string fname = Util::getPath(Util::PATH_USER_CONFIG) + "Profiles.xml";
			File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
			f.write(downBuf);
			f.close();
			File::deleteFile(fname);
			File::renameFile(fname + ".tmp", fname);
			reloadFromHttp();
			MessageBox(_T("Client profiles now updated."), _T("Client profiles updated"), MB_OK);
		}
	}
};

#endif //CLIENTSPAGE_H

/**
 * @file
 * $Id$
 */
