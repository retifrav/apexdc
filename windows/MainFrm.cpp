/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#include "MainFrm.h"
#include "AboutDlg.h"
#include "HubFrame.h"
#include "SearchFrm.h"
#include "PublicHubsFrm.h"
#include "PropertiesDlg.h"
#include "UsersFrame.h"
#include "DirectoryListingFrm.h"
#include "RecentsFrm.h"
#include "FavoritesFrm.h"
#include "NotepadFrame.h"
#include "QueueFrame.h"
#include "SpyFrame.h"
#include "FinishedFrame.h"
#include "ADLSearchFrame.h"
#include "FinishedULFrame.h"
#include "TextFrame.h"
#include "UpdateDlg.h"
#include "StatsFrame.h"
#include "UploadQueueFrame.h"
#include "HashProgressDlg.h"
#include "PrivateFrame.h"
#include "WinUtil.h"
#include "CDMDebugFrame.h"
#include "HashToolDlg.h"
#include "PopupManager.h"
#include "InstallDlg.h"
#include "ResourceLoader.h"
#include "SystemFrame.h"
#include "ExMessageBox.h"
#include "BetaDlg.h"

#include "../client/ConnectionManager.h"
#include "../client/ConnectivityManager.h"
#include "../client/DownloadManager.h"
#include "../client/HashManager.h"
#include "../client/UploadManager.h"
#include "../client/StringTokenizer.h"
#include "../client/SimpleXML.h"
#include "../client/ShareManager.h"
#include "../client/LogManager.h"
#include "../client/WebServerManager.h"
#include "../client/Thread.h"
#include "../client/ThrottleManager.h"
#include "../client/MappingManager.h"
#include "../client/PluginManager.h"

#include "../dht/dht.h"

MainFrame* MainFrame::anyMF = NULL;
bool MainFrame::bShutdown = false;
uint64_t MainFrame::iCurrentShutdownTime = 0;
bool MainFrame::isShutdownStatus = false;

map<tstring, function<void ()>, noCaseStringLess> MainFrame::commands;
CMenuHandle MainFrame::plugins;

MainFrame::MainFrame() : CSplitterImpl<MainFrame>(false), trayMessage(0), tbButtonMessage(0), maximized(false), lastUpdate(0), lastSettingsSave(GET_TICK()),
	lastUp(0), lastDown(0), lastTTHdir(Util::emptyStringT), awaybyminimize(false), missedAutoConnect(false), bTrayIcon(false), bAppMinimized(false),
	bIsPM(false), m_bDisableAutoComplete(false), oldshutdown(false), closing(false), updated(false), tabPos(1), stopperThread(NULL),
	QuickSearchBoxContainer(WC_COMBOBOX, this, QUICK_SEARCH_MAP), QuickSearchEditContainer(WC_EDIT, this, QUICK_SEARCH_MAP)
{ 
		if(WinUtil::getOsMajor() >= 6) {
			user32lib = LoadLibrary(_T("user32"));
			_d_ChangeWindowMessageFilter = (LPFUNC)GetProcAddress(user32lib, "ChangeWindowMessageFilter");
		}

		memzero(statusSizes, sizeof(statusSizes));
		anyMF = this;
}

MainFrame::~MainFrame() {
	m_CmdBar.m_hImageList = NULL;

	images.Destroy();
	largeImages.Destroy();
	largeImagesHot.Destroy();

	WinUtil::uninit();

	if(updated && !updateCommand.first.empty()) {
		if(oldshutdown) updateCommand.second += _T(" -restart");
		ShellExecute(NULL, _T("runas"), updateCommand.first.c_str(), updateCommand.second.c_str(), NULL, SW_HIDE);
	}

	if(WinUtil::getOsMajor() >= 6)
		FreeLibrary(user32lib);
}

DWORD WINAPI MainFrame::stopper(void* p) {
	MainFrame* mf = (MainFrame*)p;
	HWND wnd, wnd2 = NULL;

	while( (wnd=::GetWindow(mf->m_hWndMDIClient, GW_CHILD)) != NULL) {
		if(wnd == wnd2)
			Sleep(100);
		else { 
			::PostMessage(wnd, WM_CLOSE, 0, 0);
			wnd2 = wnd;
		}
	}

	mf->PostMessage(WM_CLOSE);
	return 0;
}

class ListMatcher : public Thread {
public:
	ListMatcher(StringList files_) : files(files_) {

	}
	int run() {
		for(StringIter i = files.begin(); i != files.end(); ++i) {
			UserPtr u = DirectoryListing::getUserFromFilename(*i);
			if(!u)
				continue;
				
			HintedUser user(u, Util::emptyString);
			DirectoryListing dl(user);
			try {
				dl.loadFile(*i);
				string tmp = STRING_F(MATCHED_FILES, QueueManager::getInstance()->matchListing(dl));
				LogManager::getInstance()->message(Util::toString(ClientManager::getInstance()->getNicks(user)) + ": " + tmp, LogManager::LOG_INFO);
			} catch(const Exception&) {

			}
		}
		delete this;
		return 0;
	}
	StringList files;
};

LRESULT MainFrame::onMatchAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ListMatcher* matcher = new ListMatcher(File::findFiles(Util::getListPath(), "*.xml*"));
	try {
		matcher->start();
	} catch(const ThreadException&) {
		///@todo add error message
		delete matcher;
	}
	
	return 0;
}

LRESULT MainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	PopupManager::newInstance();

	TimerManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);
	WebServerManager::getInstance()->addListener(this);
	UpdateManager::getInstance()->addListener(this);

	WinUtil::init(m_hWnd);
	PluginManager::getInstance()->runHook(HOOK_UI_CREATED, m_hWnd, NULL);
	refreshPluginMenu();

	trayMessage = RegisterWindowMessage(_T("TaskbarCreated"));

	if(WinUtil::getOsMajor() >= 6) {
		// 1 == MSGFLT_ADD
		_d_ChangeWindowMessageFilter(trayMessage, 1);
		_d_ChangeWindowMessageFilter(WMU_WHERE_ARE_YOU, 1);

		if(WinUtil::getOsMajor() > 6 || WinUtil::getOsMinor() >= 1) {
			tbButtonMessage = RegisterWindowMessage(_T("TaskbarButtonCreated"));
			_d_ChangeWindowMessageFilter(tbButtonMessage, 1);
			_d_ChangeWindowMessageFilter(WM_COMMAND, 1);
		}
	}

	TimerManager::getInstance()->start();

	// Set window name
	SetWindowText(Text::toT(COMPLETEVERSIONSTRING(Util::emptyString)).c_str());

	// Load images
	// create command bar window
	hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);

	m_hMenu = WinUtil::mainMenu;

	hShutdownIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SHUTDOWN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	// attach menu
	m_CmdBar.AttachMenu(m_hMenu);
	// load command bar images
	ResourceLoader::LoadImageList(IDR_TOOLBAR, images, 16, 16);
	m_CmdBar.m_hImageList = images;

	m_CmdBar.m_arrCommand.Add(ID_FILE_CONNECT);
	m_CmdBar.m_arrCommand.Add(ID_FILE_RECONNECT);
	m_CmdBar.m_arrCommand.Add(IDC_FOLLOW);
	m_CmdBar.m_arrCommand.Add(IDC_RECENTS);
	m_CmdBar.m_arrCommand.Add(IDC_FAVORITES);
	m_CmdBar.m_arrCommand.Add(IDC_FAVUSERS);
	m_CmdBar.m_arrCommand.Add(IDC_QUEUE);
	m_CmdBar.m_arrCommand.Add(IDC_FINISHED);
	m_CmdBar.m_arrCommand.Add(IDC_UPLOAD_QUEUE);
	m_CmdBar.m_arrCommand.Add(IDC_FINISHED_UL);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_FILE_ADL_SEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_SEARCH_SPY);
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_FILE_LIST);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SETTINGS);
	m_CmdBar.m_arrCommand.Add(IDC_NOTEPAD);
	m_CmdBar.m_arrCommand.Add(IDC_NET_STATS);
	m_CmdBar.m_arrCommand.Add(IDC_CDMDEBUG_WINDOW);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_CASCADE);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_TILE_HORZ);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_TILE_VERT);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_MINIMIZE_ALL);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_RESTORE_ALL);
	m_CmdBar.m_arrCommand.Add(ID_GET_HASH);	
	m_CmdBar.m_arrCommand.Add(IDC_UPDATE);	

	// use Vista-styled menus on Vista/Win7
	if(WinUtil::getOsMajor() >= 6)
		m_CmdBar._AddVistaBitmapsFromImageList(0, m_CmdBar.m_arrCommand.GetSize());

	// remove old menu
	SetMenu(NULL);

	tbarcreated = false;
	HWND hWndToolBar = createToolbar();
	HWND hWndQuickSearchBar = createQuickSearchBar();

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
	AddSimpleReBarBand(hWndQuickSearchBar, NULL, FALSE, 200, TRUE);
	CreateSimpleStatusBar();
	
	RECT toolRect = {0};
	::GetWindowRect(hWndToolBar, &toolRect);

	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatus.SetSimple(FALSE);
	int w[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ctrlStatus.SetParts(11, w);
	statusSizes[0] = WinUtil::getTextWidth(TSTRING(AWAY), ctrlStatus.m_hWnd); // for "AWAY" segment

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);

	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlLastLines.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	ctrlLastLines.AddTool(&ti);
	ctrlLastLines.SetDelayTime(TTDT_AUTOPOP, 15000);

	CreateMDIClient();
	m_CmdBar.SetMDIClient(m_hWndMDIClient);
	WinUtil::mdiClient = m_hWndMDIClient;

	ctrlTab.Create(m_hWnd, rcDefault);
	WinUtil::tabCtrl = &ctrlTab;
	tabPos = SETTING(TABS_POS);

	transferView.Create(m_hWnd);

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(m_hWndMDIClient, transferView.m_hWnd);
	SetSplitterPosPct(SETTING(TRANSFER_SPLIT_SIZE) / 100);
	
	UIAddToolBar(hWndToolBar);
	UIAddToolBar(hWndQuickSearchBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UISetCheck(ID_VIEW_TRANSFER_VIEW, 1);
	UISetCheck(ID_TOGGLE_QSEARCH, 1);
	
	// load bars settings
	WinUtil::loadReBarSettings(m_hWndToolBar);	

	UISetCheck(IDC_LIMITER, BOOLSETTING(THROTTLE_ENABLE));
	UISetCheck(IDC_TOPMOST, BOOLSETTING(TOPMOST));
	UISetCheck(IDC_LOCK_TOOLBARS, BOOLSETTING(LOCK_TOOLBARS));
	
	if(BOOLSETTING(TOPMOST)) toggleTopmost();
	if(BOOLSETTING(LOCK_TOOLBARS)) toggleLockToolbars();

	trayMenu.CreatePopupMenu();
	trayMenu.AppendMenu(MF_STRING, IDC_TRAY_SHOW, CTSTRING(MENU_SHOW));
	trayMenu.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	trayMenu.AppendMenu(MF_STRING, IDC_LIMITER, CTSTRING(ENABLE_LIMITING));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, IDC_HELP_HOMEPAGE, CTSTRING(MENU_HOMEPAGE));
	trayMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));
	trayMenu.SetMenuDefaultItem(IDC_TRAY_SHOW);

	tbMenu.CreatePopupMenu();
	tbMenu.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, CTSTRING(MENU_TOOLBAR));
	tbMenu.AppendMenu(MF_STRING, ID_TOGGLE_QSEARCH, CTSTRING(TOGGLE_QSEARCH));
	tbMenu.AppendMenu(MF_SEPARATOR);
	tbMenu.AppendMenu(MF_STRING, IDC_LOCK_TOOLBARS, CTSTRING(LOCK_TOOLBARS));

	if(SETTING(PROTECT_START) && SETTING(PASSWORD) != "LWPNACQDBZRYXW3VHJVCJ64QBZNGHOHHHZWCLNQ" && !SETTING(PASSWORD).empty()) {
		LineDlg dlg;
		dlg.description = TSTRING(PASSWORD_DESC);
		dlg.title = TSTRING(PASSWORD_TITLE);
		dlg.password = true;
		dlg.disable = true;
		if(dlg.DoModal(m_hWnd) == IDOK ) {
			if(TTH(Text::fromT(dlg.line)) != SETTING(PASSWORD)) {
				ExitProcess(1);
			}
		}
	}

#if BETA
	SetWindowText(Text::toT(COMPLETEVERSIONSTRING(STRING(DISABLED))).c_str());

	if (SETTING(CLIENTAUTH_USER).empty() || SETTING(CLIENTAUTH_PASSWORD).empty()) {
		BetaDlg dlg;
		dlg.DoModal(m_hWnd);
	}
#endif

	UpdateManager::getInstance()->clientLogin(APP_AUTH_URL, Text::fromT(WinUtil::tth));
	UpdateManager::getInstance()->checkUpdates(VERSION_URL, Text::fromT(WinUtil::tth), false);

	if(BOOLSETTING(UPDATE_IP))
		UpdateManager::getInstance()->updateIP(SETTING(IP_SERVER));

	if(!Util::fileExists(Util::getPath(Util::PATH_USER_CONFIG) + "DCPlusPlus.xml") && !Util::fileExists(Util::getPath(Util::PATH_GLOBAL_CONFIG) + "dcppboot.xml")) {
		HKEY hk;
		bool isInstalled = false;
		TCHAR* sSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{43D1A6DC-F2D3-4EBC-8851-CC8B9C0C8763}_is1");
		if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, sSubKey, NULL, KEY_READ, &hk) == ERROR_SUCCESS) {
			::RegCloseKey(hk);
			isInstalled = true;
		}

		if(!isInstalled) {
			InstallDlg dlg;
			dlg.DoModal(m_hWnd);
		} else {
			OSVERSIONINFOEX ver;
			WinUtil::getVersionInfo(ver);
			if(((ver.dwMajorVersion >= 5 && ver.dwMinorVersion >= 1 && ver.wServicePackMajor >= 2) || (ver.dwMajorVersion >= 6)) && WinUtil::isUserAdmin()) {
				if(MessageBox(CTSTRING(FIREWALL_QUESTION), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION) == IDYES) {
					WinUtil::alterWinFirewall(true);
				}
			}
		}
	}

	if(BOOLSETTING(OPEN_SYSTEM_LOG)) PostMessage(WM_COMMAND, IDC_SYSTEM_LOG);
	if(BOOLSETTING(OPEN_PUBLIC)) PostMessage(WM_COMMAND, ID_FILE_CONNECT);
	if(BOOLSETTING(OPEN_FAVORITE_HUBS)) PostMessage(WM_COMMAND, IDC_FAVORITES);
	if(BOOLSETTING(OPEN_FAVORITE_USERS)) PostMessage(WM_COMMAND, IDC_FAVUSERS);
	if(BOOLSETTING(OPEN_RECENT_HUBS)) PostMessage(WM_COMMAND, IDC_RECENTS);	
	if(BOOLSETTING(OPEN_QUEUE)) PostMessage(WM_COMMAND, IDC_QUEUE);
	if(BOOLSETTING(OPEN_FINISHED_DOWNLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED);
	if(BOOLSETTING(OPEN_UPLOAD_QUEUE)) PostMessage(WM_COMMAND, IDC_UPLOAD_QUEUE);
	if(BOOLSETTING(OPEN_FINISHED_UPLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED_UL);
	if(BOOLSETTING(OPEN_SEARCH_SPY)) PostMessage(WM_COMMAND, IDC_SEARCH_SPY);
	if(BOOLSETTING(OPEN_NETWORK_STATISTICS)) PostMessage(WM_COMMAND, IDC_NET_STATS);
	if(BOOLSETTING(OPEN_NOTEPAD)) PostMessage(WM_COMMAND, IDC_NOTEPAD);
	if(BOOLSETTING(OPEN_CDMDEBUG)) PostMessage(WM_COMMAND, IDC_CDMDEBUG_WINDOW);

	if(!BOOLSETTING(SHOW_STATUSBAR)) PostMessage(WM_COMMAND, ID_VIEW_STATUS_BAR);
	if(!BOOLSETTING(SHOW_TOOLBAR)) PostMessage(WM_COMMAND, ID_VIEW_TOOLBAR);
	if(!BOOLSETTING(SHOW_TRANSFERVIEW))	PostMessage(WM_COMMAND, ID_VIEW_TRANSFER_VIEW);
	if(!BOOLSETTING(SHOW_QUICK_SEARCH))	PostMessage(WM_COMMAND, ID_TOGGLE_QSEARCH);

#if !BETA
	if(!WinUtil::isShift() && !BOOLSETTING(UPDATE_IP))
		PostMessage(WM_SPEAKER, AUTO_CONNECT);
#endif

	PostMessage(WM_SPEAKER, PARSE_COMMAND_LINE);

	try {
		File::ensureDirectory(SETTING(LOG_DIRECTORY));
	} catch (const FileException) {	}

	if(BOOLSETTING(WEBSERVER)) {
		try {
			WebServerManager::getInstance()->start();
		} catch(const Exception& e) {
			MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		}
	}

	try {
		ConnectivityManager::getInstance()->setup(true);
	} catch (const Exception&) {
		MessageBox(CTSTRING(PORT_BUSY), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
	}
	
	// Different app icons for different instances
	HICON trayIcon = NULL;
	if(Util::fileExists(Util::getPath(Util::PATH_RESOURCES) + "app.ico")) {
		HICON appIcon = (HICON)::LoadImage(NULL, Text::toT(Util::getPath(Util::PATH_RESOURCES) + "app.ico").c_str(), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR | LR_LOADFROMFILE);
		trayIcon = (HICON)::LoadImage(NULL, Text::toT(Util::getPath(Util::PATH_RESOURCES) + "app.ico").c_str(), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_LOADFROMFILE);

		if(WinUtil::getOsMajor() < 6 || (WinUtil::getOsMajor() == 6 && WinUtil::getOsMinor() < 1))
			DestroyIcon((HICON)SetClassLongPtr(m_hWnd, GCLP_HICON, (LONG_PTR)appIcon));

		DestroyIcon((HICON)SetClassLongPtr(m_hWnd, GCLP_HICONSM, (LONG_PTR)trayIcon));
	}

	normalicon.hIcon = (!trayIcon) ? (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR) : trayIcon;
	pmicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY_PM), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	updateTray(true);

	Util::setAway(BOOLSETTING(AWAY));

	ctrlToolbar.CheckButton(IDC_AWAY,BOOLSETTING(AWAY));
	ctrlToolbar.CheckButton(IDC_DISABLE_SOUNDS, BOOLSETTING(SOUNDS_DISABLED));

#if !BETA
	if(SETTING(NICK).empty()) {
		PostMessage(WM_COMMAND, ID_FILE_SETTINGS);
	}
#endif

	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 1;
}

HWND MainFrame::createQuickSearchBar() {
	ctrlQuickSearchBar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, ATL_IDW_TOOLBAR);

	TBBUTTON tb[1];
	memzero(&tb, sizeof(tb));

	tb[0].iBitmap = 200;
	tb[0].fsStyle = TBSTYLE_SEP;

	ctrlQuickSearchBar.SetButtonStructSize();
	ctrlQuickSearchBar.AddButtons(1, tb);
	ctrlQuickSearchBar.AutoSize();

	CRect rect;
	ctrlQuickSearchBar.GetItemRect(0, &rect);
	rect.bottom += 100;
	rect.left += 2;

	QuickSearchBox.Create(ctrlQuickSearchBar.m_hWnd, rect , NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL , 0);
	updateQuickSearches();
	QuickSearchBox.SetWindowText(CTSTRING(QSEARCH_STR));
	QuickSearchBoxContainer.SubclassWindow(QuickSearchBox.m_hWnd);
	QuickSearchBox.SetExtendedUI();

	QuickSearchBox.SetFont(WinUtil::systemFont, FALSE);

	POINT pt;
	pt.x = 10;
	pt.y = 10;
	HWND hWnd = QuickSearchBox.ChildWindowFromPoint(pt);
	if(hWnd != NULL && !QuickSearchEdit.IsWindow() && hWnd != QuickSearchBox.m_hWnd) {
		QuickSearchEdit.Attach(hWnd);
		QuickSearchEditContainer.SubclassWindow(QuickSearchEdit.m_hWnd);
	}

	return ctrlQuickSearchBar.m_hWnd;
}

LRESULT MainFrame::onQuickSearchChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled){
	if(uMsg == WM_CHAR)
		if(wParam == VK_BACK)
			m_bDisableAutoComplete = true;
		else
			m_bDisableAutoComplete = false;

	switch(wParam) {
		case VK_DELETE:
			if(uMsg == WM_KEYDOWN) {
				m_bDisableAutoComplete = true;
			}
			bHandled = FALSE;
			break;
		case VK_RETURN:
			if( WinUtil::isShift() || WinUtil::isCtrl() || WinUtil::isAlt() ) {
				bHandled = FALSE;
			} else {
				if(uMsg == WM_KEYDOWN) {
					SearchManager::TypeModes type = SearchManager::TYPE_ANY;
					tstring s(QuickSearchEdit.GetWindowTextLength() + 1, _T('\0'));
					QuickSearchEdit.GetWindowText(&s[0], s.size());
					s.resize(s.size()-1);

					if(BOOLSETTING(SEARCH_DETECT_TTH)) {
						if(s.size() == 39) {
							try {
								boost::wregex reg(_T("^[A-Z2-7]{38}[QYAI]$"), boost::regex_constants::icase);
								if(boost::regex_match(s, reg))
									type = SearchManager::TYPE_TTH;
							} catch(...) { }
						}
					}

					SearchFrame::openWindow(s, 0, SearchManager::SIZE_ATLEAST, type);

					if(BOOLSETTING(CLEAR_SEARCH))
						QuickSearchBox.SetWindowText(CTSTRING(QSEARCH_STR));
				}
			}
			break;
		default:
			bHandled = FALSE;
	}
	return 0;
}

LRESULT MainFrame::onQuickSearchColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HDC hDC = (HDC)wParam;
	::SetBkColor(hDC, WinUtil::bgColor);
	::SetTextColor(hDC, WinUtil::textColor);
	return (LRESULT)WinUtil::bgBrush;
}

LRESULT MainFrame::onQuickSearchEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	uint32_t nTextLen = 0, nMatchedTextLen = 0;
	HWND hWndCombo = QuickSearchBox.m_hWnd;
	_TCHAR *pStrMatchedText = NULL, *pEnteredText = NULL;
	DWORD dwStartSel = 0, dwEndSel = 0;

	// Get the text length from the combobox, then copy it into a newly allocated buffer.
	nTextLen = ::SendMessage(hWndCombo, WM_GETTEXTLENGTH, NULL, NULL);
	pEnteredText = new _TCHAR[nTextLen + 1];
	::SendMessage(hWndCombo, WM_GETTEXT, (WPARAM)nTextLen + 1, (LPARAM)pEnteredText);
	::SendMessage(hWndCombo, CB_GETEDITSEL, (WPARAM)&dwStartSel, (LPARAM)&dwEndSel);

	// Check to make sure autocompletion isn't disabled due to a backspace or delete
	// Also, the user must be typing at the end of the string, not somewhere in the middle.
	if (! m_bDisableAutoComplete && (dwStartSel == dwEndSel) && (dwStartSel == nTextLen)) {
		// Try and find a string that matches the typed substring.  If one is found,
		// set the text of the combobox to that string and set the selection to mask off
		// the end of the matched string.
		int nMatch = ::SendMessage(hWndCombo, CB_FINDSTRING, (WPARAM)-1, (LPARAM)pEnteredText);
		if (nMatch != CB_ERR) {
			nMatchedTextLen = ::SendMessage(hWndCombo, CB_GETLBTEXTLEN, (WPARAM)nMatch, 0);
			if (nMatchedTextLen != CB_ERR) {
				// Since the user may be typing in the same string, but with different case (e.g. "/port --> /PORT")
				// we copy whatever the user has already typed into the beginning of the matched string,
				// then copy the whole shebang into the combobox.  We then set the selection to mask off
				// the inferred portion.
				pStrMatchedText = new _TCHAR[nMatchedTextLen + 1];
				::SendMessage(hWndCombo, CB_GETLBTEXT, (WPARAM)nMatch, (LPARAM)pStrMatchedText);				
				memcpy((void*)pStrMatchedText, (void*)pEnteredText, nTextLen * sizeof(_TCHAR));
				::SendMessage(hWndCombo, WM_SETTEXT, 0, (WPARAM)pStrMatchedText);
				::SendMessage(hWndCombo, CB_SETEDITSEL, 0, MAKELPARAM(nTextLen, -1));
				delete[] pStrMatchedText;
			}
		}
	}

	delete[] pEnteredText;
	bHandled = TRUE;	

    return 0;
}

void MainFrame::updateQuickSearches() {
	QuickSearchBox.ResetContent();
	if(!SearchFrame::getLastSearches().empty()) {
		for(std::set<tstring>::const_iterator i = SearchFrame::getLastSearches().begin(); i != SearchFrame::getLastSearches().end(); ++i) {
			QuickSearchBox.InsertString(0, i->c_str());
		}
	} else {
		const std::set<tstring>& lst = SearchManager::getInstance()->getSearchHistory();
		for(std::set<tstring>::const_iterator i = lst.begin(); i != lst.end(); ++i) {
			QuickSearchBox.InsertString(0, i->c_str());
		}
	}
	
	if(BOOLSETTING(CLEAR_SEARCH) && ::IsWindow(QuickSearchEdit.m_hWnd)) {
		QuickSearchBox.SetWindowText(CTSTRING(QSEARCH_STR));
	}
}

HWND MainFrame::createToolbar() {
	if(!tbarcreated) {
		if(SETTING(TOOLBARIMAGE) == "") {
			ResourceLoader::LoadImageList(IDR_TOOLBAR20, largeImages, 22, 22);
		} else {
			int size = SETTING(TB_IMAGE_SIZE);
			ResourceLoader::LoadImageList(Text::toT(SETTING(TOOLBARIMAGE)).c_str(), largeImages, size, size);
		}

		if(SETTING(TOOLBARHOTIMAGE) == "") {
			ResourceLoader::LoadImageList(IDR_TOOLBAR20_HOT, largeImagesHot, 22, 22);
		} else {
			int size = SETTING(TB_IMAGE_SIZE_HOT);
			ResourceLoader::LoadImageList(Text::toT(SETTING(TOOLBARHOTIMAGE)).c_str(), largeImagesHot, size, size);
		}

		ctrlToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
		ctrlToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
		ctrlToolbar.SetImageList(largeImages);
		ctrlToolbar.SetHotImageList(largeImagesHot);
		tbarcreated = true;
	}

	while(ctrlToolbar.GetButtonCount()>0)
		ctrlToolbar.DeleteButton(0);

	ctrlToolbar.SetButtonStructSize();
	StringTokenizer<string> t(SETTING(TOOLBAR), ',');
	StringList& l = t.getTokens();
	
	int buttonsCount = sizeof(ToolbarButtons) / sizeof(ToolbarButtons[0]);
	for(StringList::const_iterator k = l.begin(); k != l.end(); ++k) {
		int i = Util::toInt(*k);		
		
		TBBUTTON nTB;
		memzero(&nTB, sizeof(TBBUTTON));

		if(i == -1) {
			nTB.fsStyle = TBSTYLE_SEP;			
		} else if(i >= 0 && i < buttonsCount) {
			nTB.iBitmap = ToolbarButtons[i].image;
			nTB.idCommand = ToolbarButtons[i].id;
			nTB.fsState = TBSTATE_ENABLED;
			nTB.fsStyle = TBSTYLE_AUTOSIZE | ((ToolbarButtons[i].check == true)? TBSTYLE_CHECK : TBSTYLE_BUTTON);
			nTB.iString = ctrlToolbar.AddStrings(CTSTRING_I((ResourceManager::Strings)ToolbarButtons[i].tooltip));
		} else {
			continue;
		}

		ctrlToolbar.AddButtons(1, &nTB);
	}	

	ctrlToolbar.AutoSize();

	// This way correct button height is preserved...
	TBBUTTONINFO tbi;
	memzero(&tbi, sizeof(TBBUTTONINFO));
	tbi.cbSize = sizeof(TBBUTTONINFO);
	tbi.dwMask = TBIF_STYLE;

	if(ctrlToolbar.GetButtonInfo(ID_FILE_CONNECT, &tbi) != -1) {
		tbi.fsStyle |= TBSTYLE_DROPDOWN;
		ctrlToolbar.SetButtonInfo(ID_FILE_CONNECT, &tbi);
	}

	if(ctrlToolbar.GetButtonInfo(IDC_FAVORITES, &tbi) != -1) {
		tbi.fsStyle |= TBSTYLE_DROPDOWN;
		ctrlToolbar.SetButtonInfo(IDC_FAVORITES, &tbi);
	}

	return ctrlToolbar.m_hWnd;
}



LRESULT MainFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		
	if(wParam == DOWNLOAD_LISTING) {
		unique_ptr<DirectoryListInfo> i(reinterpret_cast<DirectoryListInfo*>(lParam));
		DirectoryListingFrame::openWindow(i->file, i->dir, i->user, i->speed);
	} else if(wParam == BROWSE_LISTING) {
		unique_ptr<DirectoryBrowseInfo> i(reinterpret_cast<DirectoryBrowseInfo*>(lParam));
		DirectoryListingFrame::openWindow(i->user, i->text, 0);
	} else if(wParam == VIEW_FILE_AND_DELETE) {
		unique_ptr<tstring> file(reinterpret_cast<tstring*>(lParam));
		TextFrame::openWindow(*file);
		File::deleteFile(Text::fromT(*file));
	} else if(wParam == STATS) {
		unique_ptr<TStringList> pstr(reinterpret_cast<TStringList*>(lParam));
		const TStringList& str = *pstr;
		if(ctrlStatus.IsWindow()) {
			bool u = false;
			ctrlStatus.SetText(1, str[0].c_str());
			for(int i = 1; i < 9; i++) {
				int w = WinUtil::getTextWidth(str[i], ctrlStatus.m_hWnd);
				
				if(statusSizes[i] < w) {
					statusSizes[i] = w;
					u = true;
				}
				ctrlStatus.SetText(i+1, str[i].c_str());
			}

			if(u)
				UpdateLayout(TRUE);

			if (bShutdown) {
				uint64_t iSec = GET_TICK() / 1000;
				if(!isShutdownStatus) {
					ctrlStatus.SetIcon(10, hShutdownIcon);
					isShutdownStatus = true;
				}
				if (DownloadManager::getInstance()->getDownloadCount() > 0) {
					iCurrentShutdownTime = iSec;
					ctrlStatus.SetText(10, _T(""));
				} else {
					int64_t timeLeft = SETTING(SHUTDOWN_TIMEOUT) - (iSec - iCurrentShutdownTime);
					ctrlStatus.SetText(10, (_T(" ") + Util::formatSeconds(timeLeft, timeLeft < 3600)).c_str(), SBT_POPOUT);
					if (iCurrentShutdownTime + SETTING(SHUTDOWN_TIMEOUT) <= iSec) {
						bool bDidShutDown = false;
						bDidShutDown = WinUtil::shutDown(SETTING(SHUTDOWN_ACTION));
						if (bDidShutDown) {
							// Should we go faster here and force termination?
							// We "could" do a manual shutdown of this app...
						} else {
							LogManager::getInstance()->message(STRING(FAILED_TO_SHUTDOWN), LogManager::LOG_ERROR);
							ctrlStatus.SetText(10, _T(""));
						}
						// We better not try again. It WON'T work...
						bShutdown = false;
					}
				}
			} else {
				if(isShutdownStatus) {
					ctrlStatus.SetIcon(10, NULL);
					isShutdownStatus = false;
				}
				ctrlStatus.SetText(10, _T(""));
			}
		}
	} else if(wParam == AUTO_CONNECT) {
		autoConnect(FavoriteManager::getInstance()->getFavoriteHubs());
	} else if(wParam == PARSE_COMMAND_LINE) {
		parseCommandLine(GetCommandLine());
	} else if(wParam == STATUS_MESSAGE) {
		tstring* msg = (tstring*)lParam;
		if(ctrlStatus.IsWindow()) {
			tstring line = Text::toT("[" + Util::getShortTimeString() + "] ") + *msg;

			ctrlStatus.SetText(0, line.c_str());
			while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
				lastLinesList.erase(lastLinesList.begin());
			if (line.find(_T('\r')) == tstring::npos) {
				lastLinesList.push_back(line);
			} else {
				lastLinesList.push_back(line.substr(0, line.find(_T('\r'))));
			}
		}
		delete msg;
	} else if(wParam == SHOW_POPUP) {
		Popup* msg = (Popup*)lParam;
		PopupManager::getInstance()->Show(msg->Message, msg->Title, msg->Icon, msg->hIcon, msg->force);
		if(msg->hIcon) DestroyIcon(msg->hIcon);
		delete msg;
	} else if(wParam == WM_CLOSE) {
		PopupManager::getInstance()->Remove((int)lParam);
	} else if(wParam == REMOVE_POPUP){
		PopupManager::getInstance()->AutoRemove();
	} else if(wParam == SET_PM_TRAY_ICON) {
		if(bIsPM == false && (!WinUtil::isAppActive || bAppMinimized)) {
			bIsPM = true;

			if(taskbarList)
				taskbarList->SetOverlayIcon(m_hWnd, pmicon.hIcon, NULL);

			if(bTrayIcon == true) {
				NOTIFYICONDATA nid;
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON;
				nid.hIcon = pmicon.hIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
    }

	return 0;
}

void MainFrame::parseCommandLine(const tstring& cmdLine)
{
	string::size_type i = 0;
	string::size_type j;

	if( (j = cmdLine.find(_T("dchub://"), i)) != string::npos) {
		WinUtil::parseDchubUrl(cmdLine.substr(j), false);
	}
	if( (j = cmdLine.find(_T("nmdcs://"), i)) != string::npos) {
		WinUtil::parseDchubUrl(cmdLine.substr(j), true);
	}	
	if( (j = cmdLine.find(_T("adc://"), i)) != string::npos) {
		WinUtil::parseADChubUrl(cmdLine.substr(j), false);
	}
	if( (j = cmdLine.find(_T("adcs://"), i)) != string::npos) {
		WinUtil::parseADChubUrl(cmdLine.substr(j), true);
	}
	if( (j = cmdLine.find(_T("magnet:?"), i)) != string::npos) {
		WinUtil::parseMagnetUri(cmdLine.substr(j));
	}

	if( (j = cmdLine.find(_T("/rebuild"), i)) != string::npos) {
		HashManager::getInstance()->rebuild();
	}
	if( (j = cmdLine.find(_T("/updated"), i)) != string::npos) {
		ShowPopup(CTSTRING(UPDATER_END), CTSTRING(UPDATER), NIIF_INFO, true);
	}
}

LRESULT MainFrame::onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	tstring cmdLine = (LPCTSTR) (((COPYDATASTRUCT *)lParam)->lpData);
	parseCommandLine(Text::toT(WinUtil::getAppName() + " ") + cmdLine);
	return true;
}

LRESULT MainFrame::onHashProgress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HashProgressDlg(false).DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	AboutDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::OnAppLogin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(UpdateManager::getInstance()->isLoggedIn())
		UpdateManager::getInstance()->resetLogin();

	BetaDlg dlg;
	dlg.DoModal(m_hWnd);

	UpdateManager::getInstance()->clientLogin(APP_AUTH_URL, Text::fromT(WinUtil::tth));
	return 0;
}

LRESULT MainFrame::OnAppInstall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	InstallDlg dlg(true);
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::OnAppGeoUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!UpdateManager::getInstance()->isUpdating()) {
		UpdateManager::getInstance()->updateGeoIP(GEOIPFILE, WinUtil::getAppName());
		ShowPopup(CTSTRING(UPDATER_START), CTSTRING(UPDATER), NIIF_INFO, true);
	} else {
			MessageBox(updated ? CTSTRING(UPDATER_PENDING_RESTART) : CTSTRING(UPDATER_IN_PROGRESS), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}

LRESULT MainFrame::onOpenWindows(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID) {
		case ID_FILE_SEARCH: SearchFrame::openWindow(); break;
		case ID_FILE_CONNECT: PublicHubsFrame::openWindow(); break;
		case IDC_FAVORITES: FavoriteHubsFrame::openWindow(); break;
		case IDC_FAVUSERS: UsersFrame::openWindow(); break;
		case IDC_NOTEPAD: NotepadFrame::openWindow(); break;
		case IDC_QUEUE: QueueFrame::openWindow(); break;
		case IDC_SEARCH_SPY: SpyFrame::openWindow(); break;
		case IDC_FILE_ADL_SEARCH: ADLSearchFrame::openWindow(); break;
		case IDC_NET_STATS: StatsFrame::openWindow(); break; 
		case IDC_FINISHED: FinishedFrame::openWindow(); break;
		case IDC_FINISHED_UL: FinishedULFrame::openWindow(); break;
		case IDC_UPLOAD_QUEUE: UploadQueueFrame::openWindow(); break;
		case IDC_CDMDEBUG_WINDOW: CDMDebugFrame::openWindow(); break;
		case IDC_RECENTS: RecentHubsFrame::openWindow(); break;
		case IDC_SYSTEM_LOG: SystemFrame::openWindow(); break;
		default: dcassert(0); break;
	}
	return 0;
}

LRESULT MainFrame::OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PropertiesDlg dlg(m_hWnd, SettingsManager::getInstance());

	unsigned short lastTCP = static_cast<unsigned short>(SETTING(TCP_PORT));
	unsigned short lastUDP = static_cast<unsigned short>(SETTING(UDP_PORT));
	unsigned short lastTLS = static_cast<unsigned short>(SETTING(TLS_PORT));
	unsigned short lastDHT = static_cast<unsigned short>(SETTING(DHT_PORT));
	
	int lastConn = SETTING(INCOMING_CONNECTIONS);
	bool lastDHTConn = BOOLSETTING(USE_DHT);
	string lastMapper = SETTING(MAPPER);
	string lastBind = SETTING(BIND_ADDRESS);

	bool lastSortFavUsersFirst = BOOLSETTING(SORT_FAVUSERS_FIRST);
	bool lastCheckNewUsers = BOOLSETTING(CHECK_NEW_USERS);
	bool lastZionTabs = BOOLSETTING(ZION_TABS);
	bool lastGlobalMiniTabs = BOOLSETTING(GLOBAL_MINI_TABS);

	bool lastWebServerState = BOOLSETTING(WEBSERVER);
	int lastWebServerPort = SETTING(WEBSERVER_PORT);

	if(dlg.DoModal(m_hWnd) == IDOK) 
	{
		SettingsManager::getInstance()->save();
		if(missedAutoConnect && !SETTING(NICK).empty()) {
			PostMessage(WM_SPEAKER, AUTO_CONNECT);
		}

		if(BOOLSETTING(WEBSERVER) != lastWebServerState || SETTING(WEBSERVER_PORT) != lastWebServerPort) {
			if(BOOLSETTING(WEBSERVER)) {
				if(SETTING(WEBSERVER_PORT) != lastWebServerPort) {
					WebServerManager::getInstance()->restart();
				} else WebServerManager::getInstance()->start();
			} else WebServerManager::getInstance()->stop();
		}

		try {
			ConnectivityManager::getInstance()->setup(SETTING(INCOMING_CONNECTIONS) != lastConn || SETTING(TCP_PORT) != lastTCP || SETTING(UDP_PORT) != lastUDP || SETTING(TLS_PORT) != lastTLS ||
				SETTING(DHT_PORT) != lastDHT || BOOLSETTING(USE_DHT) != lastDHTConn || SETTING(MAPPER) != lastMapper || SETTING(BIND_ADDRESS) != lastBind ||
				BOOLSETTING(WEBSERVER) != lastWebServerState || SETTING(WEBSERVER_PORT) != lastWebServerPort);
		} catch (const Exception&) {
			MessageBox(CTSTRING(PORT_BUSY), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		}
 
		if(BOOLSETTING(SORT_FAVUSERS_FIRST) != lastSortFavUsersFirst)
			HubFrame::resortUsers();

		if(BOOLSETTING(CHECK_NEW_USERS) != lastCheckNewUsers)
			ClientManager::getInstance()->toggleChecks(!lastCheckNewUsers);

		if(BOOLSETTING(URL_HANDLER)) {
			WinUtil::registerDchubHandler();
			WinUtil::registerADChubHandler();
			WinUtil::urlDcADCRegistered = true;
		} else if(WinUtil::urlDcADCRegistered) {
			WinUtil::unRegisterDchubHandler();
			WinUtil::unRegisterADChubHandler();
			WinUtil::urlDcADCRegistered = false;
		}
		if(BOOLSETTING(MAGNET_REGISTER)) {
			WinUtil::registerMagnetHandler();
			WinUtil::urlMagnetRegistered = true; 
		} else if(WinUtil::urlMagnetRegistered) {
			WinUtil::unRegisterMagnetHandler();
			WinUtil::urlMagnetRegistered = false;
		}

		UISetCheck(IDC_LIMITER, BOOLSETTING(THROTTLE_ENABLE));
		UISetCheck(IDC_DISABLE_SOUNDS,BOOLSETTING(SOUNDS_DISABLED)); 
		UISetCheck(IDC_AWAY, Util::getAway());
		UISetCheck(IDC_SHUTDOWN, getShutDown());

		if(tabPos != SETTING(TABS_POS)) {
			tabPos = SETTING(TABS_POS);
			ctrlTab.updateTabs();
			UpdateLayout();
		}

		if(BOOLSETTING(ZION_TABS) != lastZionTabs)
			WinUtil::tabCtrl->changeTabsStyle();

		if(BOOLSETTING(GLOBAL_MINI_TABS) != lastGlobalMiniTabs)
			WinUtil::tabCtrl->updateMiniTabs();

		ClientManager::getInstance()->infoUpdated();
	}
	return 0;
}

void MainFrame::on(UpdateListener::UpdateAvailable, const UpdateCheckData& ui) noexcept {
	if(closing || oldshutdown)
		return;

	UpdateDlg dlg(ui);
	auto closeCommand = dlg.DoModal(m_hWnd);

	if(closeCommand == IDC_UPDATE_DOWNLOAD) {
		if(!UpdateManager::getInstance()->isUpdating()) {
			UpdateManager::getInstance()->downloadUpdate(ui.updateUrl, WinUtil::getAppName());
			ShowPopup(CTSTRING(UPDATER_START), CTSTRING(UPDATER), NIIF_INFO, true);
		} else {
			MessageBox(updated ? CTSTRING(UPDATER_PENDING_RESTART) : CTSTRING(UPDATER_IN_PROGRESS), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION);
		}
	} else if(closeCommand == IDCLOSE) {
		if(ui.isForced) {
			oldshutdown = true;
			PostMessage(WM_CLOSE);
		}
	}
}

void MainFrame::on(UpdateListener::UpdateComplete, const string& updater, const string& args) noexcept {
	updated = true;
	updateCommand = make_pair(Text::toT(updater), Text::toT(args));

	if(MessageBox(CTSTRING(UPDATER_RESTART), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1) == IDYES) {
		if(updater != WinUtil::getAppName())
			WinUtil::openLink(_T(HOMEPAGE));

		oldshutdown = true;
		PostMessage(WM_CLOSE);
	} else if(updater != WinUtil::getAppName())
		WinUtil::openLink(_T(HOMEPAGE));
}

void MainFrame::on(UpdateListener::UpdateFailed, const string& line) noexcept {
	ShowPopup(Text::toT(STRING(UPDATER_FAILED) + " " + line), CTSTRING(UPDATER), NIIF_ERROR, true);
}

void MainFrame::on(UpdateListener::AuthSuccess, const string& title, const string& message) noexcept {
	SetWindowText(Text::toT(COMPLETEVERSIONSTRING(SETTING(CLIENTAUTH_USER))).c_str());

	if(!message.empty())
		MessageBox(Text::toT(Text::toDOS(message)).c_str(), Text::toT(title).c_str(), MB_OK | MB_ICONINFORMATION);

#if BETA
		// Since we left these out before do them now (bah have to do it like this to avoid freeze)
		if(!WinUtil::isShift() && (!BOOLSETTING(UPDATE_IP) || missedAutoConnect))
			PostMessage(WM_SPEAKER, AUTO_CONNECT);

		if(SETTING(NICK).empty())
			PostMessage(WM_COMMAND, ID_FILE_SETTINGS);
#endif
}

void MainFrame::on(UpdateListener::AuthFailure, const string& message) noexcept {
#if BETA
	if(!message.empty())
		MessageBox(Text::toT(Text::toDOS(message)).c_str(), _T("Authorization Failure - ") _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION);

	UpdateManager::getInstance()->resetLogin();

	oldshutdown = true;
	PostMessage(WM_CLOSE);
#else
	if(!message.empty())
		LogManager::getInstance()->message("User Login Error: " + message, LogManager::LOG_ERROR);
#endif
}

void MainFrame::on(UpdateListener::SettingUpdated, size_t key, const string& /*value*/) noexcept {
	if (key == SettingsManager::EXTERNAL_IP && !WinUtil::isShift() && ::FindWindowEx(WinUtil::mdiClient, NULL, _T("HubFrame"), NULL) == NULL)
		PostMessage(WM_SPEAKER, AUTO_CONNECT);
}

LRESULT MainFrame::onDropDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTOOLBAR tb = (LPNMTOOLBAR)pnmh;
	if(dropMenu != NULL) {
		dropMenu.DestroyMenu();
		dropMenu.m_hMenu = NULL;
	}

	dropMenu.CreatePopupMenu();

	UINT menuItems = 0;
	if(tb->iItem == IDC_FAVORITES) {
		const FavoriteHubEntryList& hubs = FavoriteManager::getInstance()->getFavoriteHubs();
		for(FavoriteHubEntryList::const_iterator i = hubs.begin(); i != hubs.end(); ++i) {
			if(menuItems >= 20) break; // allow max 20 items
			dropMenu.AppendMenu(MF_STRING, ID_DROPMENU + menuItems, Text::toT((*i)->getServer()).c_str());
			if((*i)->getConnect()) dropMenu.CheckMenuItem(menuItems, MF_BYPOSITION | MF_CHECKED);
			menuItems++;
		}
	} else if(tb->iItem == ID_FILE_CONNECT) {
		const StringList& lists = FavoriteManager::getInstance()->getHubLists();
		for(StringList::const_iterator i = lists.begin(); i != lists.end(); ++i) {
			if(menuItems >= 20) break; // allow max 20 items
			dropMenu.AppendMenu(MF_STRING, ID_DROPMENU + menuItems, Text::toT(*i).c_str());
			if(menuItems == (UINT)FavoriteManager::getInstance()->getSelectedHubList()) 
				dropMenu.CheckMenuItem(menuItems, MF_BYPOSITION | MF_CHECKED);
			menuItems++;
		}
	}

	POINT pt;
	pt.x = tb->rcButton.left;
	pt.y = tb->rcButton.bottom;
	::MapWindowPoints(pnmh->hwndFrom, HWND_DESKTOP, &pt, 1);
	UINT idCommand = m_CmdBar.TrackPopupMenu(dropMenu.m_hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_VERTICAL, pt.x, pt.y);

	// This way we don't need to handle WM_MENUCOMMAND, and thus can use CMDICommandBarCtrl's TrackPopupMenu
	if(idCommand >= ID_DROPMENU && idCommand <= ID_DROPMENU_MAX) {
		if(tb->iItem == IDC_FAVORITES) {
			FavoriteHubEntry* entry = FavoriteManager::getInstance()->getFavoriteHubs()[idCommand - ID_DROPMENU];
			if(entry) HubFrame::openWindow(Text::toT(entry->getServer()), entry->getChatUserSplit(), entry->getUserListState());
		} else if(tb->iItem == ID_FILE_CONNECT) {
			PublicHubsFrame::openWindow(idCommand - ID_DROPMENU);
		}
	}

	return TBDDRET_DEFAULT;
}

LRESULT MainFrame::onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
	pDispInfo->szText[0] = 0;

	if(!((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))) {
		// if we're really in the status bar, this should be detected intelligently
		lastLines.clear();
		for(TStringIter i = lastLinesList.begin(); i != lastLinesList.end(); ++i) {
			lastLines += *i;
			lastLines += _T("\r\n");
		}
		if(lastLines.size() > 2) {
			lastLines.erase(lastLines.size() - 2);
		}
		pDispInfo->lpszText = const_cast<TCHAR*>(lastLines.c_str());
	}
	return 0;
}

void MainFrame::autoConnect(const FavoriteHubEntry::List fl) {
	missedAutoConnect = false;
	for(FavoriteHubEntry::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		FavoriteHubEntry* entry = *i;
		if(entry->getConnect()) {
 			if(!entry->getNick().empty() || !SETTING(NICK).empty()) {
				RecentHubEntry r;
				r.setName(entry->getName());
				r.setDescription(entry->getDescription());
				r.setUsers("*");
				r.setShared("*");
				r.setServer(entry->getServer());
				FavoriteManager::getInstance()->addRecent(r);
				missedAutoConnect = !HubFrame::openWindow(Text::toT(entry->getServer())
					, entry->getChatUserSplit(), entry->getUserListState());
 			} else missedAutoConnect = true;
 		}
	}
}

void MainFrame::updateTray(bool add /* = true */) {
	if(add) {
		if(bTrayIcon == false) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
			nid.uCallbackMessage = WM_APP + 242;
			nid.hIcon = normalicon.hIcon;
			_tcsncpy(nid.szTip, _T(APPNAME), 64);
			nid.szTip[63] = '\0';
			lastMove = GET_TICK() - 1000;
			bTrayIcon = (::Shell_NotifyIcon(NIM_ADD, &nid) != FALSE);
		}
	} else {
		if(bTrayIcon) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = 0;
			::Shell_NotifyIcon(NIM_DELETE, &nid);
			bTrayIcon = false;
		}
	}
}

LRESULT MainFrame::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam == SIZE_MINIMIZED) {
		SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
		if(BOOLSETTING(AUTO_AWAY)) {
			if(bAppMinimized == false)
			if(Util::getAway() == true) {
				awaybyminimize = false;
			} else {
				awaybyminimize = true;
				Util::setAway(true);
				ctrlToolbar.CheckButton(IDC_AWAY, TRUE);
				ClientManager::getInstance()->infoUpdated();
			}
		}

		if(BOOLSETTING(MINIMIZE_TRAY) != WinUtil::isShift()) {
			ShowWindow(SW_HIDE);
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		}
		bAppMinimized = true;
		maximized = IsZoomed() > 0;
	} else if( (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) ) {
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		if(BOOLSETTING(AUTO_AWAY)) {
			if(awaybyminimize == true) {
				awaybyminimize = false;
				Util::setAway(false);
				ctrlToolbar.CheckButton(IDC_AWAY, FALSE);
				ClientManager::getInstance()->infoUpdated();
			}
		}

		if(bIsPM) {
			bIsPM = false;

			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd,
					Util::fileExists(Util::getPath(Util::PATH_RESOURCES) + "app.ico") ? normalicon.hIcon : NULL, NULL);
			}

			if(bTrayIcon == true) {
				NOTIFYICONDATA nid;
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON;
				nid.hIcon = normalicon.hIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
		bAppMinimized = false;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);
	
	CRect rc;
	GetWindowRect(rc);
	
	if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, rc.left);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, rc.top);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, rc.Width());
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, rc.Height());
	}
	if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, (int)wp.showCmd);
	
	QueueManager::getInstance()->saveQueue();
	SettingsManager::getInstance()->save();
	LogManager::getInstance()->saveCache();
	
	return 0;
}

LRESULT MainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closing) {
		if((SETTING(PROTECT_CLOSE)) && (!oldshutdown) && SETTING(PASSWORD) != "LWPNACQDBZRYXW3VHJVCJ64QBZNGHOHHHZWCLNQ" && !SETTING(PASSWORD).empty()) {
			LineDlg dlg;
			dlg.description = TSTRING(PASSWORD_DESC);
			dlg.title = TSTRING(PASSWORD_TITLE);
			dlg.password = true;
			dlg.disable = true;
			if(dlg.DoModal(m_hWnd) == IDOK) {
				if(TTH(Text::fromT(dlg.line)) != SETTING(PASSWORD)) {
					bHandled = TRUE;
					return 0;
				}
			}
		}

		if(!updated && UpdateManager::getInstance()->isUpdating()) {
			MessageBox(CTSTRING(UPDATER_IN_PROGRESS), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION);

			bHandled = TRUE;
			return 0;
		}

		UINT checkState = BOOLSETTING(CONFIRM_EXIT) ? BST_CHECKED : BST_UNCHECKED;
		if(oldshutdown || SETTING(PROTECT_CLOSE) || (checkState == BST_UNCHECKED) || (::MessageBox(m_hWnd, CTSTRING(REALLY_EXIT), _T(APPNAME) _T(" ") _T(VERSIONSTRING), CTSTRING(ALWAYS_ASK), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2, checkState) == IDYES)) {
			updateTray(false);
			string tmp1;
			string tmp2;

			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);

			CRect rc;
			GetWindowRect(rc);
			if(BOOLSETTING(SHOW_TRANSFERVIEW)) {
				SettingsManager::getInstance()->set(SettingsManager::TRANSFER_SPLIT_SIZE, m_nProportionalPos);
			}
			if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, rc.left);
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, rc.top);
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, rc.Width());
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, rc.Height());
			}
			if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, (int)wp.showCmd);

			// save bars settings
			WinUtil::saveReBarSettings(m_hWndToolBar);
			
			ShowWindow(SW_HIDE);
			transferView.prepareClose();

			UpdateManager::getInstance()->removeListener(this);
			WebServerManager::getInstance()->removeListener(this);
			SearchManager::getInstance()->disconnect();
			ConnectionManager::getInstance()->disconnect();
			LogManager::getInstance()->saveCache();

			listQueue.shutdown();
			DWORD id;
			stopperThread = CreateThread(NULL, 0, stopper, this, 0, &id);
			closing = true;
		}

		// Let's update the setting checked box means we bug user again...
		SettingsManager::getInstance()->set(SettingsManager::CONFIRM_EXIT, checkState != BST_UNCHECKED);
		bHandled = TRUE;
	} else {
		// This should end immediately, as it only should be the stopper that sends another WM_CLOSE
		WaitForSingleObject(stopperThread, 60*1000);
		CloseHandle(stopperThread);
		stopperThread = NULL;
		DestroyIcon(normalicon.hIcon);
		DestroyIcon(hShutdownIcon);
		DestroyIcon(pmicon.hIcon);
		bHandled = FALSE;
	}

	return 0;
}

LRESULT MainFrame::onLink(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring site;
	bool isFile = false;
	switch(wID) {
		case IDC_HELP_HOMEPAGE: site = _T(HOMEPAGE); break;
		case IDC_HELP_DISCUSS: site = _T(DISCUSS); break;
		case IDC_HELP_DONATE: site = _T(DONATE); break;
		case IDC_GUIDES: site = _T(GUIDES); break;
		default: dcassert(0);
	}

	if(isFile)
		WinUtil::openFile(site);
	else
		WinUtil::openLink(site);

	return 0;
}

int MainFrame::run() {
	tstring file;
	if(WinUtil::browseFile(file, m_hWnd, false, lastTTHdir) == IDOK) {
		WinUtil::mainMenu.EnableMenuItem(ID_GET_HASH, MF_GRAYED);
		Thread::setThreadPriority(Thread::LOW);
		lastTTHdir = Util::getFilePath(file);

		boost::scoped_array<char> buf(new char[512 * 1024]);

		try {
			File f(Text::fromT(file), File::READ, File::OPEN);

			SimpleHasher<TigerTree> tth(true);
			SimpleHasher<SHA1Hash> sha1(true);
			SimpleHasher<MD5Hash> md5(false);

			if(f.getSize() > 0) {
				size_t n = 512*1024;
				while((n = f.read(&buf[0], n)) > 0) {
					tth.update(&buf[0], n);
					sha1.update(&buf[0], n);
					md5.update(&buf[0], n);
					n = 512*1024;
				}
			} else {
				tth.update("", 0);
				sha1.update("", 0);
				md5.update("", 0);
			}

			tth.finalize();
			sha1.finalize();
			md5.finalize();

			HashToolDlg report(m_hWnd);

			string SHA1 = sha1.toString();
			string TTH = tth.toString();
			string MD5 = md5.toString();
			string magnetlink = "magnet:?xt=urn:bitprint:"+SHA1+"."+TTH+"&xt=urn:md5:"+MD5+"&xl="+Util::toString(f.getSize())+"&dn="+Util::encodeURI(Text::fromT(Util::getFileName(file)));

			f.close();

			report.DoModal(file.c_str(), Text::toT(TTH + "\r\n" + SHA1 + "\r\n" + MD5).c_str(), Text::toT(magnetlink).c_str());
		} catch(...) { }
		Thread::setThreadPriority(Thread::NORMAL);
		WinUtil::mainMenu.EnableMenuItem(ID_GET_HASH, MF_ENABLED);
	}
	return 0;
}

LRESULT MainFrame::onGetHash(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Thread::start();
	return 0;
}

void MainFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow() && ctrlLastLines.IsWindow()) {
		CRect sr;
		int w[11];
		ctrlStatus.GetClientRect(sr);
		w[10] = sr.right - 20;
		w[9] = w[10] - 60;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(8); setw(7); setw(6); setw(5); setw(4); setw(3); setw(2); setw(1); setw(0);

		ctrlStatus.SetParts(11, w);
		ctrlLastLines.SetMaxTipWidth(w[0]);
	}
	
	CRect rc = rect;
	CRect rc2 = rect;

	if(tabPos == 0) {
		rc.bottom = rc.top + ctrlTab.getHeight();
		rc2.top = rc.bottom;
	} else if(tabPos == 1) {
		rc.top = rc.bottom - ctrlTab.getHeight();
		rc2.bottom = rc.top;
	} else if(tabPos == 2) {
		rc.left = 0;
		rc.right = 150;
		rc2.left = rc.right;
	} else if(tabPos == 3) {
		rc.left = rc.right - 150;
		rc2.right = rc.left;
	}
	
	if(ctrlTab.IsWindow())
		ctrlTab.MoveWindow(rc);

	SetSplitterRect(rc2);
}

static const TCHAR types[] = _T("File Lists\0*.xml.bz2\0All Files\0*.*\0");

LRESULT MainFrame::onOpenFileList(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring file;
	
	if(wID == IDC_OPEN_MY_LIST){
		if(!ShareManager::getInstance()->getOwnListFile().empty()){
			DirectoryListingFrame::openWindow(Text::toT(ShareManager::getInstance()->getOwnListFile()), Text::toT(Util::emptyString), HintedUser(ClientManager::getInstance()->getMe(), Util::emptyString), 0);
		}
		return 0;
	}

	if(WinUtil::browseFile(file, m_hWnd, false, Text::toT(Util::getListPath()), types)) {
		UserPtr u = DirectoryListing::getUserFromFilename(Text::fromT(file));
		if(u) {
			DirectoryListingFrame::openWindow(file, Text::toT(Util::emptyString), HintedUser(u, Util::emptyString), 0);
		} else {
			MessageBox(CTSTRING(INVALID_LISTNAME), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
		}
	}
	return 0;
}

LRESULT MainFrame::onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(true);
	return 0;
}

LRESULT MainFrame::onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	if (lParam == (!BOOLSETTING(OLD_TRAY_BEHAV) ? WM_LBUTTONDBLCLK : WM_LBUTTONUP)) {
		if(bAppMinimized) {
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
		} else {
			if(!BOOLSETTING(OLD_TRAY_BEHAV)) {
				ShowWindow(SW_MINIMIZE);
			} else {
				SetForegroundWindow(m_hWnd);
			}
		}
	} else if(lParam == WM_MOUSEMOVE && ((lastMove + 1000) < GET_TICK()) ) {
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = NIF_TIP;
		_tcsncpy(nid.szTip, (_T("D: ") + Util::formatBytesT(DownloadManager::getInstance()->getRunningAverage()) + _T("/s (") + 
			Util::toStringT(DownloadManager::getInstance()->getDownloadCount()) + _T(")\r\nU: ") +
			Util::formatBytesT(UploadManager::getInstance()->getRunningAverage()) + _T("/s (") + 
			Util::toStringT(UploadManager::getInstance()->getUploadCount()) + _T(")")
			+ _T("\r\nUptime: ") + Util::formatSeconds(time(NULL) - Util::getStartTime())).c_str(), 64);
		
		::Shell_NotifyIcon(NIM_MODIFY, &nid);
		lastMove = GET_TICK();
	} else if (lParam == WM_RBUTTONUP) {
		CPoint pt(GetMessagePos());		
		SetForegroundWindow(m_hWnd);
		trayMenu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);		
		PostMessage(WM_NULL, 0, 0);
	} else if(lParam == NIN_BALLOONUSERCLICK) {
		if(bAppMinimized) {
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
		} else {
			SetForegroundWindow(m_hWnd);
		}
	}
	return 0;
}

void MainFrame::ShowPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags, HICON hIcon, bool force) {
	Popup* p = new Popup;
	p->Title = szTitle;
	p->Message = szMsg;
	p->Icon = dwInfoFlags;
	p->hIcon = hIcon;
	p->force = force;

	PostMessage(WM_SPEAKER, SHOW_POPUP, (LPARAM)p);
}

LRESULT MainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewQuickSearchBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 2);	// toolbar is 3rd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_TOGGLE_QSEARCH, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_QUICK_SEARCH, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewTopmost(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	UISetCheck(IDC_TOPMOST, !BOOLSETTING(TOPMOST));
	SettingsManager::getInstance()->set(SettingsManager::TOPMOST, !BOOLSETTING(TOPMOST));
	toggleTopmost();
	return 0;
}

LRESULT MainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, bVisible);
	return 0;
}

LRESULT MainFrame::OnViewTransferView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !transferView.IsWindowVisible();
	if(!bVisible) {	
		if(GetSinglePaneMode() == SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_TOP);
	} else { 
		if(GetSinglePaneMode() != SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_NONE);
	}
	UISetCheck(ID_VIEW_TRANSFER_VIEW, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, bVisible);
	return 0;
}

LRESULT MainFrame::onCloseWindows(WORD , WORD wID, HWND , BOOL& ) {
	switch(wID) {
	case IDC_CLOSE_DISCONNECTED:		HubFrame::closeDisconnected();		break;
	case IDC_CLOSE_ALL_HUBS:			HubFrame::closeAll();				break;
    case IDC_CLOSE_HUBS_NO_USR:			HubFrame::closeAll(2);				break;
	case IDC_CLOSE_ALL_PM:				PrivateFrame::closeAll();			break;
	case IDC_CLOSE_ALL_OFFLINE_PM:		PrivateFrame::closeAllOffline();	break;
	case IDC_CLOSE_ALL_DIR_LIST:		DirectoryListingFrame::closeAll();	break;
	case IDC_CLOSE_ALL_SEARCH_FRAME:	SearchFrame::closeAll();			break;
	case IDC_RECONNECT_DISCONNECTED:	HubFrame::reconnectDisconnected();	break;
	}
	return 0;
}

LRESULT MainFrame::onLimiter(WORD , WORD , HWND , BOOL& ) {
	UISetCheck(IDC_LIMITER, !BOOLSETTING(THROTTLE_ENABLE));
	SettingsManager::getInstance()->set(SettingsManager::THROTTLE_ENABLE, !BOOLSETTING(THROTTLE_ENABLE));
	ClientManager::getInstance()->infoUpdated();
	return 0;
}

LRESULT MainFrame::onQuickConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	LineDlg dlg;
	dlg.description = TSTRING(HUB_ADDRESS);
	dlg.title = TSTRING(QUICK_CONNECT);
	if(dlg.DoModal(m_hWnd) == IDOK){
		if(SETTING(NICK).empty()) {
			MessageBox(CTSTRING(ENTER_NICK), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
			return 0;
		}

		tstring tmp = dlg.line;
		// Strip out all the spaces
		string::size_type i;
		while((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);

		if(!tmp.empty()) {
			RecentHubEntry r;
			r.setName("*");
			r.setDescription("*");
			r.setUsers("*");
			r.setShared("*");
			r.setServer(Text::fromT(tmp));
			FavoriteManager::getInstance()->addRecent(r);
			HubFrame::openWindow(tmp);
		}
	}
	return 0;
}

void MainFrame::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	if(aTick == lastUpdate)	// FIXME: temp fix for new TimerManager
		return;

	int64_t diff = (int64_t)((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
	int64_t updiff = Socket::getTotalUp() - lastUp;
	int64_t downdiff = Socket::getTotalDown() - lastDown;

	TStringList* str = new TStringList();
	str->push_back(Util::getAway() ? TSTRING(AWAY) : _T(""));
	
	dht::DHT* dhtManager = DHT::getInstance();
	str->push_back(_T("DHT: ") + (dhtManager->isConnected() ? Util::toStringT(dhtManager->getNodesCount()) : _T("-")));
	str->push_back(TSTRING(SHARED) + _T(": ") + Util::formatBytesT(ShareManager::getInstance()->getSharedSize()));
	str->push_back(_T("H: ") + Text::toT(Client::getCounts()));
	str->push_back(TSTRING(SLOTS) + _T(": ") + Util::toStringT(UploadManager::getInstance()->getFreeSlots()) + _T('/') + Util::toStringT(UploadManager::getInstance()->getSlots()) + _T(" (") + Util::toStringT(UploadManager::getInstance()->getFreeExtraSlots()) + _T('/') + Util::toStringT(SETTING(EXTRA_SLOTS)) + _T(")"));
	str->push_back(_T("D: ") + Util::formatBytesT(Socket::getTotalDown()));
	str->push_back(_T("U: ") + Util::formatBytesT(Socket::getTotalUp()));
	
	tstring down = _T("D: [") + Util::toStringT(DownloadManager::getInstance()->getDownloadCount()) + _T("][");
	tstring up = _T("U: [") + Util::toStringT(UploadManager::getInstance()->getUploadCount()) + _T("][");
	if(BOOLSETTING(THROTTLE_ENABLE)) {
		down += Util::formatBytesT(ThrottleManager::getInstance()->getDownLimit() * 1024) + _T("/s");
		up += Util::formatBytesT(ThrottleManager::getInstance()->getUpLimit() * 1024) + _T("/s");
	} else {
		down += _T("-");
		up += _T("-");
	}
	
	str->push_back(down + _T("] ") + Util::formatBytesT(downdiff*1000I64/diff) + _T("/s"));
	str->push_back(up + _T("] ") + Util::formatBytesT(updiff*1000I64/diff) + _T("/s"));
	
	PostMessage(WM_SPEAKER, STATS, (LPARAM)str);

	SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + updiff);
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + downdiff);
	lastUpdate = aTick;
	lastUp = Socket::getTotalUp();
	lastDown = Socket::getTotalDown();

	if(SETTING(DISCONNECT_SPEED) < 1) {
		SettingsManager::getInstance()->set(SettingsManager::DISCONNECT_SPEED, 1);
	}

	if(SETTING(SETTINGS_SAVE_INTERVAL) != 0 && (aTick >= (lastSettingsSave + SETTING(SETTINGS_SAVE_INTERVAL) * 60 * 1000))) {
		SettingsManager::getInstance()->save();
		lastSettingsSave = aTick;
	}
}

void MainFrame::on(PartialList, const HintedUser& aUser, const string& text) noexcept {
	PostMessage(WM_SPEAKER, BROWSE_LISTING, (LPARAM)new DirectoryBrowseInfo(aUser, text));
}

void MainFrame::on(QueueManagerListener::Finished, const QueueItem* qi, const string& dir, const Download* download) noexcept {
	if(qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
			// This is a file listing, show it...
			DirectoryListInfo* i = new DirectoryListInfo(qi->getDownloads()[0]->getHintedUser(), Text::toT(qi->getListName()), Text::toT(dir), static_cast<int64_t>(download->getAverageSpeed()));

			PostMessage(WM_SPEAKER, DOWNLOAD_LISTING, (LPARAM)i);
		} else if(qi->isSet(QueueItem::FLAG_TEXT)) {
			PostMessage(WM_SPEAKER, VIEW_FILE_AND_DELETE, (LPARAM) new tstring(Text::toT(qi->getTarget())));
		}
	} else if(qi->isSet(QueueItem::FLAG_USER_CHECK)) {
		DirectoryListInfo* i = new DirectoryListInfo(qi->getDownloads()[0]->getHintedUser(), Text::toT(qi->getListName()), Text::toT(dir), static_cast<int64_t>(download->getAverageSpeed()));
		
		if(listQueue.stop) {
			listQueue.stop = false;
			listQueue.start();
		}
		{
			Lock l(listQueue.cs);
			listQueue.fileLists.push_back(i);
		}
		listQueue.s.signal();
	}	

	if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
		// Finished file sound
		if(!SETTING(FINISHFILE).empty() && !BOOLSETTING(SOUNDS_DISABLED))
			PlaySound(Text::toT(SETTING(FINISHFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

		// Antivirus check
		if(BOOLSETTING(USE_ANTIVIR) && (!SETTING(ANTIVIR_PATH).empty() && !SETTING(ANTIVIR_PARAMS).empty())) {
			StringMap params;
			params["file"] = "\"" + qi->getTarget() + "\"";
			params["dir"] = "\"" + Util::getFilePath(qi->getTarget()) + "\"";

			::ShellExecute(NULL, _T("open"), Text::toT(SETTING(ANTIVIR_PATH)).c_str(), Text::toT(Util::formatParams(SETTING(ANTIVIR_PARAMS), params, false)).c_str(), NULL, BOOLSETTING(HIDE_ANTIVIR) ? SW_HIDE : SW_SHOWNORMAL);
		}
	}
}

LRESULT MainFrame::onActivateApp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	WinUtil::isAppActive = (wParam == 1);	//wParam == TRUE if window is activated, FALSE if deactivated
	if(wParam == 1) {
		if(bIsPM) {
			bIsPM = false;

			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd,
					Util::fileExists(Util::getPath(Util::PATH_RESOURCES) + "app.ico") ? normalicon.hIcon : NULL, NULL);
			}

			if(bTrayIcon == true) {
				NOTIFYICONDATA nid;
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON;
				nid.hIcon = normalicon.hIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
	}
	return 0;
}

LRESULT MainFrame::onAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	if(GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD) {
		ctrlTab.SwitchTo();
	} else if(GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_BACKWARD) {
		ctrlTab.SwitchTo(false);
	} else {
		bHandled = FALSE;
	}
	
	return FALSE;
}

LRESULT MainFrame::onAway(WORD , WORD , HWND, BOOL& ) {
	if(Util::getAway()) { 
		ctrlToolbar.CheckButton(IDC_AWAY, FALSE);
		Util::setAway(false);
	} else {
		ctrlToolbar.CheckButton(IDC_AWAY, TRUE);
		Util::setAway(true);
	}
	ClientManager::getInstance()->infoUpdated();
	return 0;
}

LRESULT MainFrame::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateManager::getInstance()->checkUpdates(VERSION_URL, Text::fromT(WinUtil::tth), true);
	return 0;
}

LRESULT MainFrame::onDisableSounds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/) {
	if(hWndCtl != ctrlToolbar.m_hWnd) {
		ctrlToolbar.CheckButton(IDC_DISABLE_SOUNDS, !BOOLSETTING(SOUNDS_DISABLED));
	}
	SettingsManager::getInstance()->set(SettingsManager::SOUNDS_DISABLED, !BOOLSETTING(SOUNDS_DISABLED));
	return 0;
}


void MainFrame::on(WebServerListener::ShutdownPC, int action) noexcept {
	WinUtil::shutDown(action);
}

int MainFrame::FileListQueue::run() {
	setThreadPriority(Thread::LOW);

	while(true) {
		s.wait(15000);
		if(stop || fileLists.empty()) {
			break;
		}

		DirectoryListInfo* i;
		{
			Lock l(cs);
			i = fileLists.front();
			fileLists.pop_front();
		}
		if(Util::fileExists(Text::fromT(i->file))) {
			DirectoryListing* dl = new DirectoryListing(i->user);
			try {
				dl->loadFile(Text::fromT(i->file));
				ADLSearchManager::getInstance()->matchListing(*dl);
				ClientManager::getInstance()->checkCheating(i->user, dl);
			} catch(...) {
			}
			delete dl;
			if(BOOLSETTING(DELETE_CHECKED)) {
				File::deleteFile(Text::fromT(i->file));
			}
		}
		delete i;
	}
	stop = true;
	return 0;
}

LRESULT MainFrame::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	LogManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);

	PopupManager::deleteInstance();

	updateTray(false);
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onLockToolbars(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UISetCheck(IDC_LOCK_TOOLBARS, !BOOLSETTING(LOCK_TOOLBARS));
	SettingsManager::getInstance()->set(SettingsManager::LOCK_TOOLBARS, !BOOLSETTING(LOCK_TOOLBARS));
	toggleLockToolbars();
	return 0;
}

LRESULT MainFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(reinterpret_cast<HWND>(wParam) == m_hWndToolBar) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; // location of mouse click
		tbMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		return TRUE; 
	}

	bHandled = FALSE;
	return FALSE;
}

void MainFrame::toggleTopmost() const {
	CRect rc;
	GetWindowRect(rc);
	HWND order = (GetWindowLongPtr(GWL_EXSTYLE) & WS_EX_TOPMOST) ? HWND_NOTOPMOST : HWND_TOPMOST;
	::SetWindowPos(m_hWnd, order, rc.left, rc.top, rc.Width(), rc.Height(), SWP_SHOWWINDOW);
}

void MainFrame::toggleLockToolbars() const {
	CReBarCtrl rbc = m_hWndToolBar;
	REBARBANDINFO rbi;
	rbi.cbSize = sizeof(rbi);
	rbi.fMask  = RBBIM_STYLE;
	int nCount  = rbc.GetBandCount();
	for (int i  = 0; i < nCount; i++) {
		rbc.GetBandInfo(i, &rbi);
		rbi.fStyle ^= RBBS_NOGRIPPER | RBBS_GRIPPERALWAYS;
		rbc.SetBandInfo(i, &rbi);
	}
}

LRESULT MainFrame::onPluginCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[256];
	memset(&buf, 0, sizeof(buf));
	plugins.GetMenuString(wID, buf, 256, MF_BYCOMMAND);

	auto i = commands.end();
	if((i = commands.find(buf)) != commands.end())
		i->second();
	return TRUE;
}

void MainFrame::addPluginCommand(const tstring& text, function<void ()> command) {
	commands[text.substr(0, 255)] = command;

	if(WinUtil::mainWnd && !anyMF->closing)
		anyMF->refreshPluginMenu();
}

void MainFrame::removePluginCommand(const tstring& text) {
	auto i = commands.find(text.substr(0, 255));
	if(i == commands.end()) return;
	commands.erase(i);

	if(WinUtil::mainWnd && !anyMF->closing) {
		anyMF->refreshPluginMenu();
	}
}

void MainFrame::refreshPluginMenu() {
	auto files = WinUtil::mainMenu.GetSubMenu(0);
	if(!plugins.IsMenu())
	{
		plugins.CreatePopupMenu();
		files.InsertMenu(ID_FILE_SETTINGS, MF_POPUP, (UINT_PTR)(HMENU)plugins, CTSTRING(SETTINGS_PLUGINS));
	}

	while(plugins.GetMenuItemCount() > 0)
		plugins.RemoveMenu(0, MF_BYPOSITION);

	auto idx = 0;
	for(auto i = commands.begin(); i != commands.end(); ++i, ++idx)
		plugins.AppendMenu(MF_STRING, IDC_PLUGIN_COMMANDS + idx, i->first.c_str());
}

/**
 * @file
 * $Id: MainFrm.cpp,v 1.20 2004/07/21 13:15:15 bigmuscle Exp
 */