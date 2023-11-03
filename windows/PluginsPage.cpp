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

#include <client/SettingsManager.h>
#include <client/LogManager.h>

#include "PluginsPage.h"
#include "ResourceLoader.h"

PropPage::TextItem PluginsPage::texts[] = {
	{ IDC_PLUGIN_INFORMATION, ResourceManager::PLUGIN_INFORMATION },
	{ IDC_PLUGINS_GROUP, ResourceManager::SETTINGS_PLUGINS },
	{ IDC_PLUGIN_NAME_TEXT, ResourceManager::PLUGIN_NAME },
	{ IDC_PLUGIN_VERSION_TEXT, ResourceManager::PLUGIN_VERSION },
	{ IDC_PLUGIN_AUTHOR_TEXT, ResourceManager::PLUGIN_AUTHOR },
	{ IDC_PLUGIN_DESCRIPTION_TEXT, ResourceManager::PLUGIN_DESCRIPTION },
	{ IDC_PLUGIN_WEB_TEXT, ResourceManager::PLUGIN_WEB },
	{ IDC_PLUGIN_LOAD, ResourceManager::PLUGIN_LOAD },
	{ IDC_MOVE_UP, ResourceManager::MOVE_UP },
	{ IDC_PLUGIN_CONFIG, ResourceManager::PLUGIN_CONFIG },
	{ IDC_MOVE_DOWN, ResourceManager::MOVE_DOWN },
	{ IDC_PLUGIN_UNLOAD, ResourceManager::PLUGIN_UNLOAD },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT PluginsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);

	CRect rc;

	// Completely useless but looks nice :P
	ResourceLoader::LoadImageList(IDR_O_SETTINGS_DLG, images, 16, 16);

	ctrlPluginList.Attach(GetDlgItem(IDC_PLUGINLIST));
	ctrlPluginList.SetImageList(images, LVSIL_SMALL);
	ctrlPluginList.GetClientRect(rc);

	ctrlPluginList.InsertColumn(0, _T("Name"), LVCFMT_LEFT, (rc.Width() - GetSystemMetrics(SM_CXVSCROLL)), 0);
	ctrlPluginList.InsertColumn(1, _T("GUID"), LVCFMT_LEFT, 0, 0);
	ctrlPluginList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	// Do specialized reading here
	refreshList();

	::ShowWindow(GetDlgItem(IDC_PLUGIN_NAME_TEXT), SW_HIDE);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_VERSION_TEXT), SW_HIDE);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_AUTHOR_TEXT), SW_HIDE);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_DESCRIPTION_TEXT), SW_HIDE);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_WEB_TEXT), SW_HIDE);

	::ShowWindow(GetDlgItem(IDC_PLUGIN_NAME), SW_HIDE);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_VERSION), SW_HIDE);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_AUTHOR), SW_HIDE);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_DESCRIPTION), SW_HIDE);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_WEB), SW_HIDE);

	url.SubclassWindow(GetDlgItem(IDC_PLUGIN_WEB));
	url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

	return 0;
}

void PluginsPage::write() {
	// Do specialized writing here
}

string PluginsPage::selectedGuid() {
	if(ctrlPluginList.GetSelectedCount() == 1) {
		auto i = ctrlPluginList.GetSelectedIndex();
		tstring buf;
		buf.resize(39);
		buf.resize(ctrlPluginList.GetItemText(i, 1, &buf[0], buf.size()));
		return Text::fromT(buf);
	}

	return Util::emptyString;
}

void PluginsPage::addEntry(const Plugin& p, int pos /* = -1*/) {
	if (pos == -1)
		pos = ctrlPluginList.GetItemCount();

	TStringList data;
	data.push_back(Text::toT(p.name));
	data.push_back(Text::toT(p.guid));
	ctrlPluginList.insert(pos, data, 27);
}

void PluginsPage::refreshList() {
	ctrlPluginList.SetRedraw(FALSE);
	ctrlPluginList.DeleteAllItems();
	const auto& list = PluginManager::getInstance()->getPluginList();
	for(auto i = list.cbegin(); i != list.cend(); ++i) {
		addEntry(PluginManager::getInstance()->getPlugin(*i));
	}
	ctrlPluginList.SetRedraw(TRUE);
}

void PluginsPage::moveSelected(int delta) {
	auto guid = selectedGuid();
	if(!guid.empty()) {
		int i = ctrlPluginList.GetNextItem(-1, LVNI_SELECTED);
		if (i + delta < 0 || i + delta >= ctrlPluginList.GetItemCount())
			return;
		PluginManager::getInstance()->movePlugin(guid, delta);
		ctrlPluginList.SetRedraw(FALSE);
		ctrlPluginList.DeleteItem(i);
		addEntry(PluginManager::getInstance()->getPlugin(guid), i + delta);
		ctrlPluginList.SelectItem(i + delta);
		ctrlPluginList.EnsureVisible(i + delta, FALSE);
		ctrlPluginList.SetRedraw(TRUE);
	}
}

LRESULT PluginsPage::onLoadPlugin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	static const TCHAR types[] = _T("DC Plugin Package\0*.dcext\0DLL Files\0*.dll\0All Files\0*.*\0");
	static const TCHAR defExt[] = _T(".dcext");
	tstring path = Util::emptyStringT;

	// We assume here that we are loading the chosen plugin for the very first time...
	if(WinUtil::browseFile(path, m_hWnd, false, Text::toT(Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Plugins"), types, defExt) == IDOK) {
		try {
			if (Util::getFileExt(path) == _T(".dcext")) {
				auto info = PluginManager::getInstance()->extract(Text::fromT(path));
				PluginManager::getInstance()->install(info);
			} else PluginManager::getInstance()->addPlugin(Text::fromT(path));

			refreshList();
		} catch (const Exception& e) {
			LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);
		}
	}
	return 0;
}

LRESULT PluginsPage::onConfigPlugin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlPluginList.GetSelectedCount() == 1) {
		auto guid = selectedGuid();
		if(!PluginManager::getInstance()->configPlugin(guid, m_hWnd)) {
			::MessageBox(m_hWnd, CTSTRING(PLUGIN_NO_CONFIG), Text::toT(PluginManager::getInstance()->getPlugin(guid).name).c_str(), MB_OK | MB_ICONINFORMATION);
		}
	}
	return 0;
}

LRESULT PluginsPage::onUnloadPlugin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlPluginList.GetSelectedCount() == 1) {
		PluginManager::getInstance()->removePlugin(selectedGuid());
		ctrlPluginList.DeleteItem(ctrlPluginList.GetSelectedIndex());
	}
	return 0;
}

LRESULT PluginsPage::onItemChangedPlugins(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	BOOL bEnable = ctrlPluginList.GetItemState(l->iItem, LVIS_SELECTED);
	int showCmd = bEnable ? SW_SHOW : SW_HIDE;

	::EnableWindow(GetDlgItem(IDC_MOVE_UP), bEnable);
	::EnableWindow(GetDlgItem(IDC_PLUGIN_CONFIG), bEnable);
	::EnableWindow(GetDlgItem(IDC_MOVE_DOWN), bEnable);
	::EnableWindow(GetDlgItem(IDC_PLUGIN_UNLOAD), bEnable);

	::ShowWindow(GetDlgItem(IDC_PLUGIN_NAME_TEXT), showCmd);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_VERSION_TEXT), showCmd);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_AUTHOR_TEXT), showCmd);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_DESCRIPTION_TEXT), showCmd);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_WEB_TEXT), showCmd);

	::ShowWindow(GetDlgItem(IDC_PLUGIN_NAME), showCmd);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_VERSION), showCmd);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_AUTHOR), showCmd);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_DESCRIPTION), showCmd);
	::ShowWindow(GetDlgItem(IDC_PLUGIN_WEB), showCmd);

	if(l->iItem != -1) {
		const auto& p = PluginManager::getInstance()->getPlugin(selectedGuid());
		if(ctrlPluginList.GetItemState(l->iItem, LVIS_SELECTED)) {
			::SetWindowText(GetDlgItem(IDC_PLUGIN_NAME), Text::toT(p.name).c_str());
			::SetWindowText(GetDlgItem(IDC_PLUGIN_VERSION), Util::toStringT(p.version).c_str());
			::SetWindowText(GetDlgItem(IDC_PLUGIN_AUTHOR), Text::toT(p.author).c_str());
			::SetWindowText(GetDlgItem(IDC_PLUGIN_DESCRIPTION), Text::toT(p.description).c_str());

			tstring pluginWebsite = Text::toT(p.website);
			if(pluginWebsite.find(_T("://")) != string::npos) {
				url.SetLabel(pluginWebsite.c_str());
				url.SetHyperLink(pluginWebsite.c_str());
				url.m_bVisited = false;
			} else ::ShowWindow(GetDlgItem(IDC_PLUGIN_WEB), SW_HIDE);
		}
	}

	return 0;
}

LRESULT PluginsPage::onDblClickPlugins(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;
	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_PLUGIN_CONFIG, 0);
	}
	return 0;
}
