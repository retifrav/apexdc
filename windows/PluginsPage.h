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

#ifndef PLUGINPAGE_H
#define PLUGINPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/PluginManager.h"

#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "resource.h"

#include <atlctrlx.h>

class PluginsPage : public CPropertyPage<IDD_PLUGINSPAGE>, public PropPage
{
public:
	PluginsPage(SettingsManager *s) : PropPage(s) {
		SetTitle(CTSTRING(SETTINGS_PLUGINS));
	};

	~PluginsPage() {
		ctrlPluginList.Detach();
		images.Destroy();
	};

	BEGIN_MSG_MAP(PluginsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_PLUGIN_LOAD, onLoadPlugin)
		COMMAND_ID_HANDLER(IDC_MOVE_UP, onPluginUp)
		COMMAND_ID_HANDLER(IDC_PLUGIN_CONFIG, onConfigPlugin)
		COMMAND_ID_HANDLER(IDC_MOVE_DOWN, onPluginDown)
		COMMAND_ID_HANDLER(IDC_PLUGIN_UNLOAD, onUnloadPlugin)
		COMMAND_ID_HANDLER(IDC_PLUGIN_WEB, onLink)
		NOTIFY_HANDLER(IDC_PLUGINLIST, LVN_ITEMCHANGED, onItemChangedPlugins)
		NOTIFY_HANDLER(IDC_PLUGINLIST, NM_DBLCLK, onDblClickPlugins)
		NOTIFY_HANDLER(IDC_PLUGINLIST, NM_CUSTOMDRAW, ctrlPluginList.onCustomDraw)
	END_MSG_MAP()
	
	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onLoadPlugin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onConfigPlugin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUnloadPlugin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChangedPlugins(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onDblClickPlugins(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onPluginUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		moveSelected(-1);
		return 0;
	}

	LRESULT onPluginDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		moveSelected(1);
		return 0;
	}

	LRESULT onLink (WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		url.Navigate();
		return 0;
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
private:
	string selectedGuid();
	void addEntry(const Plugin& p, int pos = -1);
	void refreshList();
	void moveSelected(int delta);

	CHyperLink url;

	CImageList images;
	ExListViewCtrl ctrlPluginList;
	static TextItem texts[];
};


#endif