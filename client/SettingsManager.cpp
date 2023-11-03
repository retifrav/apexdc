/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "SettingsManager.h"

#include "ResourceManager.h"

#include "SimpleXML.h"
#include "Util.h"
#include "File.h"
#include "version.h"
#include "AdcHub.h"
#include "CID.h"
#include "SearchManager.h"
#include "StringTokenizer.h"

#include "../dht/DHT.h"

namespace dcpp {

StringList SettingsManager::connectionSpeeds;

const string SettingsManager::settingTags[] =
{
	// Strings
	"Nick", "UploadSpeed", "Description", "DownloadDirectory", "EMail", "ExternalIp",
	"Font", "MainFrameOrder", "MainFrameWidths", "HubFrameOrder", "HubFrameWidths", 
	"LanguageFile", "SearchFrameOrder", "SearchFrameWidths", "FavoritesFrameOrder", "FavoritesFrameWidths", 
	"HublistServers", "QueueFrameOrder", "QueueFrameWidths", "PublicHubsFrameOrder", "PublicHubsFrameWidths", 
	"UsersFrameOrder", "UsersFrameWidths", "HttpProxy", "LogDirectory", "LogFormatPostDownload", 
	"LogFormatPostUpload", "LogFormatMainChat", "LogFormatPrivateChat", "FinishedOrder", "FinishedWidths",	 
	"TempDownloadDirectory", "BindAddress", "SocksServer", "SocksUser", "SocksPassword", "ConfigVersion", 
	"DefaultAwayMessage", "TimeStampsFormat", "ADLSearchFrameOrder", "ADLSearchFrameWidths", 
	"FinishedULWidths", "FinishedULOrder", "CID", "SpyFrameWidths", "SpyFrameOrder", 
	"BeepFile", "BeginFile", "FinishedFile", "UploadFile", "FakerFile", "ChatNameFile", "WinampFormat",
	"KickMsgRecent01", "KickMsgRecent02", "KickMsgRecent03", "KickMsgRecent04", "KickMsgRecent05", 
	"KickMsgRecent06", "KickMsgRecent07", "KickMsgRecent08", "KickMsgRecent09", "KickMsgRecent10", 
	"KickMsgRecent11", "KickMsgRecent12", "KickMsgRecent13", "KickMsgRecent14", "KickMsgRecent15", 
	"KickMsgRecent16", "KickMsgRecent17", "KickMsgRecent18", "KickMsgRecent19", "KickMsgRecent20",
	"Toolbar", "ToolbarImage", "ToolbarHot", "UserListImage", "UploadQueueFrameOrder", "UploadQueueFrameWidths",
	"ProfilesURL", "SoundException", "SoundHubConnected", "SoundHubDisconnected", "SoundFavUserOnline", "SoundTypingNotify",
	"WebServerLogFormat", "WebServerUser", "WebServerPass", "LogFileMainChat", 
	"LogFilePrivateChat", "LogFileStatus", "LogFileUpload", "LogFileDownload", "LogFileSystem", "LogFormatSystem", 
	"LogFormatStatus", "LogFileWebServer", "DirectoryListingFrameOrder", "DirectoryListingFrameWidths", 
	"MainFrameVisible", "SearchFrameVisible", "QueueFrameVisible", "HubFrameVisible", "UploadQueueFrameVisible", 
	"EmoticonsFile", "TLSPrivateKeyFile", "TLSCertificateFile", "TLSTrustedCertificatesPath",
	"FinishedVisible", "FinishedULVisible", "DirectoryListingFrameVisible",
	"RecentFrameOrder", "RecentFrameWidths", "ToolbarSettings", "DHTKey", "Mapper",
	
	// ApexDC++
	"ClientAuthUser", "ClientAuthPass", "AuthPass", "SkiplistShare", "HighPrioFiles", "LowPrioFiles", "SecondaryAwayMsg", "ProtectedPrefixes",
	"PopupFont", "PopupTitleFont", "PopupFile", "AntiVirPath", "AntiVirParams", "FileOpenDirectory", "IpServer",
	"SENTRY",

	// Ints
	"IncomingConnections", "InPort", "Slots", "AutoFollow", "ClearSearch",
	"BackgroundColor", "TextColor", "ShareHidden", "FilterMessages", "MinimizeToTray",
	"AutoSearch", "TimeStamps", "ConfirmExit", "PopupHubPms", "PopupBotPms", "IgnoreHubPms", "IgnoreBotPms",
	"BufferSize", "DownloadSlots", "MaxDownloadSpeed", "LogMainChat", "LogPrivateChat",
	"LogDownloads", "LogUploads", "StatusInChat", "ShowJoins", "PrivateMessageBeep", "PrivateMessageBeepOpen",
	"UseSystemIcons", "PopupPMs", "MinUploadSpeed", "GetUserInfo", "UrlHandler", "MainWindowState", 
	"MainWindowSizeX", "MainWindowSizeY", "MainWindowPosX", "MainWindowPosY", "AutoAway",
	"SocksPort", "SocksResolve", "KeepLists", "AutoKick", "QueueFrameShowTree", 
	"CompressTransfers", "ShowProgressBars", "MaxTabRows",
	"MaxCompression", "AntiFragMethod", "NoAwayMsgToBots",
	"SkipZeroByte", "AdlsBreakOnFirst",
	"HubUserCommands", "AutoSearchAutoMatch", "DownloadBarColor", "UploadBarColor", "LogSystem",
	"LogFilelistTransfers", "ShowStatusbar", "BandwidthSettingMode", "ShowToolbar", "ShowTransferview", 
	"SearchPassiveAlways", "SetMinislotSize", "MaxFilelistSize", "ShutdownInterval", 
	"CzertHiddenSettingA", "CzertHiddenSettingB", "ExtraSlots", "ExtraPartialSlots",
	"TextGeneralBackColor", "TextGeneralForeColor", "TextGeneralBold", "TextGeneralItalic", 
	"TextMyOwnBackColor", "TextMyOwnForeColor", "TextMyOwnBold", "TextMyOwnItalic", 
	"TextPrivateBackColor", "TextPrivateForeColor", "TextPrivateBold", "TextPrivateItalic", 
	"TextSystemBackColor", "TextSystemForeColor", "TextSystemBold", "TextSystemItalic", 
	"TextServerBackColor", "TextServerForeColor", "TextServerBold", "TextServerItalic", 
	"TextTimestampBackColor", "TextTimestampForeColor", "TextTimestampBold", "TextTimestampItalic", 
	"TextMyNickBackColor", "TextMyNickForeColor", "TextMyNickBold", "TextMyNickItalic", 
	"TextFavBackColor", "TextFavForeColor", "TextFavBold", "TextFavItalic", 
	"TextOPBackColor", "TextOPForeColor", "TextOPBold", "TextOPItalic", 
	"TextURLBackColor", "TextURLForeColor", "TextURLBold", "TextURLItalic", 
	"BoldAuthorsMess", "ThrottleEnable", "UploadLimitTime", "DownloadLimitTime", "HubSlots",
	"TimeThrottle", "TimeLimitStart", "TimeLimitEnd", "RemoveForbidden",
	"ProgressTextDown", "ProgressTextUp", "ShowInfoTips", "ExtraDownloadSlots",
	"MinimizeOnStratup", "ConfirmDelete", "DefaultSearchFreeSlots", "SendUnknownCommands",
	"ErrorColor", "ExpandQueue", "TransferSplitSize",
	"DisconnectSpeed", "DisconnectFileSpeed", "DisconnectTime", "RemoveSpeed",
	"ProgressOverrideColors", "Progress3DDepth", "ProgressOverrideColors2",
	"MenubarTwoColors", "MenubarLeftColor", "MenubarRightColor", "MenubarBumped", 
	"DisconnectFileSize", "UploadQueueFrameShowTree",
	"SegmentsManual", "NumberOfSegments", "PercentFakeShareTolerated",
	"MaxHashSpeed", "GetUserCountry", "DisableCZDiacritic",
	"UseAutoPriorityByDefault", "UseOldSharingUI",
	"FavShowJoins", "LogStatusMessages", "SearchAlternateColour", "SoundsDisabled",
	"ReportFoundAlternates", "CheckNewUsers", "PMLogLines", "HubLogLines",
	"SearchTime", "DontBeginSegment", "DontBeginSegmentSpeed", "PopunderPm", "PopunderFilelist",
	"DropMultiSourceOnly", "DisplayCheatsInMainChat", "MagnetAsk", "MagnetAction", "MagnetRegister",
	"DisconnectRaw", "TimeoutRaw", "FakeShareRaw", "ListLenMismatch", "FileListTooSmall", "FileListUnavailable",
	"AddFinishedInstantly", "Away", "UseCTRLForLineHistory",
	"PopupHubConnected", "PopupHubDisconnected", "PopupFavoriteConnected", "PopupCheatingUser", "PopupDownloadStart", 
	"PopupDownloadFailed", "PopupDownloadFinished", "PopupUploadFinished", "PopupPm", "PopupNewPM", 
	"PopupType", "WebServer", "WebServerPort", "WebServerLog", "ShutdownAction", "MinimumSearchInterval",
	"PopupAway", "PopupMinimized", "ShowShareCheckedUsers", "MaxAutoMatchSource",
    "ReservedSlotColor", "IgnoredColor", "FavoriteColor",
	"NormalColour", "FireballColor", "ServerColor", "PasiveColor", "OpColor", 
	"FileListAndClientCheckedColour", "BadClientColour", "BadFilelistColour", "DontDLAlreadyShared",
	"ConfirmHubRemoval", "SuppressMainChat", "ProgressBackColor", "ProgressCompressColor", "ProgressSegmentColor",
	"OpenNewWindow", "FileSlots",  "UDPPort", "MultiChunk",
 	"UserListDoubleClick", "TransferListDoubleClick", "ChatDoubleClick",
	"ToggleActiveWindow", "ProgressbaroDCStyle", "SearchHistory", 
	"AcceptedDisconnects", "AcceptedTimeouts",
	"OpenPublic", "OpenFavoriteHubs", "OpenFavoriteUsers", "OpenRecentHubs", "OpenQueue", "OpenFinishedDownloads",
	"OpenFinishedUploads", "OpenSearchSpy", "OpenNetworkStatistics", "OpenNotepad", "OutgoingConnections",
	"NoIPOverride", "GroupSearchResults", "BoldFinishedDownloads", "BoldFinishedUploads", "BoldQueue", 
	"BoldHub", "BoldPm", "BoldSearch", "TabsPos", "SocketInBuffer", "SocketOutBuffer", 
	"ColorDownloaded", "ColorRunning", "ColorDone", "AutoRefreshTime", "UseTLS", "OpenUploadQueue",
	"BoldUploadQueue", "AutoSearchLimit", "AutoKickNoFavs", "PromptPassword", "SpyFrameIgnoreTthSearches",
 	"AllowUntrustedHubs", "AllowUntrustedClients", "TLSPort", "DownConnPerSec",
	"HighestPrioSize", "HighPrioSize", "NormalPrioSize", "LowPrioSize", "LowestPrio",
	"FilterEnter", "SortFavUsersFirst", "ShowShellMenu", "SendBloom", "OverlapChunks", "ShowQuickSearch",
	"UcSubMenu", "AutoSlots", "UseDHT", "DHTPort", "UpdateIP", "KeepFinishedFiles",
	"AllowNATTraversal", "UseExplorerTheme", "MDIMaximized", "AutoDetectIncomingConnection", "SettingsSaveInterval",

	 // ApexDC++
	"ShowDescriptionLimit", "ProtectTray", "ProtectStart", "ProtectClose",
	"StripTopic", "TbImageSize", "TbImageSizeHot", "OpenCdmDebug",
	"PreviewPm", "PopupTime", "AwayThrottle", "AwayStart", "AwayEnd", "OdcStyleBumped", "TopSpeed",
	"StealthyStyle", "IpInChat", "CountryInChat", "TopUpSpeed", "SettingsState", "Page",
	"EnableHubTopic", "ProtFavs", "MaxMsgLength", "PopupBackColor", "PopupTextColor",
	"PopupTitleTextColor", "OpenLogsInternal", "GlobalHubFrameConf", "UseAntiVir",
	"StealthyIndicateSpeeds", "OldTrayBehav", "OldNamingStyle", "OldIconsMode", "ColourDupe",
	"DeleteChecked", "Topmost", "LockToolbars",
	"SearchDetectHash", "ZionTabs", "NonHubsFront", "GlobalMiniTabs", "BlendOffline", "MaxResizeLines",
	"MaxFileLists", "CheckDelay", "SleepTime", "DelayedRawSending",
	"NatSort", "UseCustomListBackground", "ProtectedColour", "UseFavNames", "OpenSystemLog", "BoldSystemLog",
	"DotHiddenFiles", "HideAntiVir", "RandomSegments",
	"SearchResponsesActive", "SearchResponsesPassive",
	"SENTRY",
	// Int64
	"TotalUpload", "TotalDownload", "LastUpdateNotice", "LastAuthTime",
	"SENTRY"
};

SettingsManager::SettingsManager()
{
	connectionSpeeds.push_back("0.01");
	connectionSpeeds.push_back("0.02");
	connectionSpeeds.push_back("0.05");
	connectionSpeeds.push_back("0.1");
	connectionSpeeds.push_back("0.2");
	connectionSpeeds.push_back("0.5");
	connectionSpeeds.push_back("1");
	connectionSpeeds.push_back("2");
	connectionSpeeds.push_back("5");
	connectionSpeeds.push_back("10");
	connectionSpeeds.push_back("20");
	connectionSpeeds.push_back("50");
	connectionSpeeds.push_back("100");
	connectionSpeeds.push_back("1000");

	for(int i=0; i<SETTINGS_LAST; i++)
		isSet[i] = false;

	for(int i=0; i<INT_LAST-INT_FIRST; i++) {
		intDefaults[i] = 0;
		intSettings[i] = 0;
	}
	for(int i=0; i<INT64_LAST-INT64_FIRST; i++) {
		int64Defaults[i] = 0;
		int64Settings[i] = 0;
	}

	setDefault(DOWNLOAD_DIRECTORY, Util::getPath(Util::PATH_DOWNLOADS));
	setDefault(TEMP_DOWNLOAD_DIRECTORY, Util::getPath(Util::PATH_USER_LOCAL) + "Incomplete" PATH_SEPARATOR_STR);
	setDefault(SLOTS, 3);
	setDefault(TCP_PORT, 0);
	setDefault(UDP_PORT, 0);
	setDefault(TLS_PORT, 0);
	setDefault(DHT_PORT, DHT_UDPPORT);
	setDefault(INCOMING_CONNECTIONS, INCOMING_DIRECT);
	setDefault(OUTGOING_CONNECTIONS, OUTGOING_DIRECT);
	setDefault(AUTO_DETECT_CONNECTION, true);
	setDefault(AUTO_FOLLOW, true);
	setDefault(CLEAR_SEARCH, true);
	setDefault(SHARE_HIDDEN, false);
	setDefault(FILTER_MESSAGES, true);
	setDefault(MINIMIZE_TRAY, false);
	setDefault(AUTO_SEARCH, true);
	setDefault(TIME_STAMPS, true);
	setDefault(CONFIRM_EXIT, true);
	setDefault(POPUP_HUB_PMS, true);
	setDefault(POPUP_BOT_PMS, true);
	setDefault(IGNORE_HUB_PMS, false);
	setDefault(IGNORE_BOT_PMS, false);
	setDefault(BUFFER_SIZE, 64);
	setDefault(HUBLIST_SERVERS, "https://dchublist.org/hublist.xml.bz2;https://www.te-home.net/?do=hublist&get=hublist.xml.bz2;https://dchublist.ru/hublist.xml.bz2;https://tankafett.biz/?do=hublist&get=hublist.xml.bz2;http://hublist.eu/hublist.xml.bz2;http://hublist.flexhub.org/hublist.xml.bz2;");
	setDefault(DOWNLOAD_SLOTS, 50);
    setDefault(FILE_SLOTS, 15);
	setDefault(MAX_DOWNLOAD_SPEED, 0);
	setDefault(LOG_DIRECTORY, Util::getPath(Util::PATH_USER_LOCAL) + "Logs" PATH_SEPARATOR_STR);
	setDefault(LOG_UPLOADS, false);
	setDefault(LOG_DOWNLOADS, false);
	setDefault(LOG_PRIVATE_CHAT, false);
	setDefault(LOG_MAIN_CHAT, false);
	setDefault(STATUS_IN_CHAT, true);
	setDefault(SHOW_JOINS, false);
	setDefault(UPLOAD_SPEED, connectionSpeeds[3]);
	setDefault(PRIVATE_MESSAGE_BEEP, false);
	setDefault(PRIVATE_MESSAGE_BEEP_OPEN, false);
	setDefault(USE_SYSTEM_ICONS, true);
	setDefault(POPUP_PMS, true);
	setDefault(MIN_UPLOAD_SPEED, 0);
	setDefault(LOG_FORMAT_POST_DOWNLOAD, "%Y-%m-%d %H:%M: %[target] " + STRING(DOWNLOADED_FROM) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time]");
	setDefault(LOG_FORMAT_POST_UPLOAD, "%Y-%m-%d %H:%M: %[source] " + STRING(UPLOADED_TO) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time]");
	setDefault(LOG_FORMAT_MAIN_CHAT, "[%Y-%m-%d %H:%M%[extra]] %[message]");
	setDefault(LOG_FORMAT_PRIVATE_CHAT, "[%Y-%m-%d %H:%M%[extra]] %[message]");
	setDefault(LOG_FORMAT_STATUS, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_SYSTEM, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FILE_MAIN_CHAT, "%B - %Y\\%[hubURL].log");
	setDefault(LOG_FILE_STATUS, "%B - %Y\\%[hubURL]_status.log");
	setDefault(LOG_FILE_PRIVATE_CHAT, "PM\\%B - %Y\\%[userNI].log");
	setDefault(LOG_FILE_UPLOAD, "Uploads.log");
	setDefault(LOG_FILE_DOWNLOAD, "Downloads.log");
	setDefault(LOG_FILE_SYSTEM, "system.log");
	setDefault(LOG_FILE_WEBSERVER, "Webserver.log");
	setDefault(GET_USER_INFO, true);
	setDefault(URL_HANDLER, true);
	setDefault(AUTO_AWAY, false);
	setDefault(BIND_ADDRESS, "0.0.0.0");
	setDefault(SOCKS_PORT, 1080);
	setDefault(SOCKS_RESOLVE, 1);
	setDefault(CONFIG_VERSION, "0.181");		// 0.181 is the last version missing configversion
	setDefault(KEEP_LISTS, false);
	setDefault(AUTO_KICK, false);
	setDefault(QUEUEFRAME_SHOW_TREE, true);
	setDefault(COMPRESS_TRANSFERS, true);
	setDefault(SHOW_PROGRESS_BARS, true);
	setDefault(DEFAULT_AWAY_MESSAGE, "I'm away. State your business and I might answer later if you're lucky.");
	setDefault(TIME_STAMPS_FORMAT, "%H:%M:%S");
	setDefault(MAX_TAB_ROWS, 4);
	setDefault(MAX_COMPRESSION, 6);
	setDefault(ANTI_FRAG, true);
	setDefault(NO_AWAYMSG_TO_BOTS, true);
	setDefault(SKIP_ZERO_BYTE, false);
	setDefault(ADLS_BREAK_ON_FIRST, false);
	setDefault(HUB_USER_COMMANDS, true);
	setDefault(AUTO_SEARCH_AUTO_MATCH, false);
	setDefault(LOG_FILELIST_TRANSFERS, false);
	setDefault(LOG_SYSTEM, false);
	setDefault(SEND_UNKNOWN_COMMANDS, false);
	setDefault(MAX_HASH_SPEED, 0);
	setDefault(GET_USER_COUNTRY, true);
	setDefault(FAV_SHOW_JOINS, false);
	setDefault(LOG_STATUS_MESSAGES, false);
	setDefault(SHOW_TRANSFERVIEW, true);
	setDefault(SHOW_STATUSBAR, true);
	setDefault(SHOW_TOOLBAR, true);
	setDefault(POPUNDER_PM, true);
	setDefault(POPUNDER_FILELIST, false);
	setDefault(MAGNET_REGISTER, true);
	setDefault(MAGNET_ASK, true);
	setDefault(MAGNET_ACTION, MAGNET_AUTO_SEARCH);
	setDefault(ADD_FINISHED_INSTANTLY, false);
	setDefault(DONT_DL_ALREADY_SHARED, false);
	setDefault(CONFIRM_HUB_REMOVAL, true);
	setDefault(USE_CTRL_FOR_LINE_HISTORY, true);
	setDefault(JOIN_OPEN_NEW_WINDOW, false);
	setDefault(PM_LAST_LINES_LOG, 10);
	setDefault(HUB_LAST_LINES_LOG, 10);
	setDefault(CONFIRM_DELETE, true);
	setDefault(TOGGLE_ACTIVE_WINDOW, true);
	setDefault(SEARCH_HISTORY, 10);
	setDefault(SET_MINISLOT_SIZE, 512);
	setDefault(MAX_FILELIST_SIZE, 256);
	setDefault(PRIO_HIGHEST_SIZE, 64);
	setDefault(PRIO_HIGH_SIZE, 0);
	setDefault(PRIO_NORMAL_SIZE, 0);
	setDefault(PRIO_LOW_SIZE, 0);
	setDefault(PRIO_LOWEST, false);
	setDefault(OPEN_PUBLIC, false);
	setDefault(OPEN_FAVORITE_HUBS, false);
	setDefault(OPEN_FAVORITE_USERS, false);
	setDefault(OPEN_RECENT_HUBS, false);
	setDefault(OPEN_QUEUE, false);
	setDefault(OPEN_FINISHED_DOWNLOADS, false);
	setDefault(OPEN_FINISHED_UPLOADS, false);
	setDefault(OPEN_SEARCH_SPY, false);
	setDefault(OPEN_NETWORK_STATISTICS, false);
	setDefault(OPEN_NOTEPAD, false);
	setDefault(NO_IP_OVERRIDE, false);
	setDefault(SOCKET_IN_BUFFER, 64*1024);
	setDefault(SOCKET_OUT_BUFFER, 64*1024);
	setDefault(OPEN_UPLOAD_QUEUE, false);
	setDefault(TLS_TRUSTED_CERTIFICATES_PATH, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR);
	setDefault(TLS_PRIVATE_KEY_FILE, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR "client.key");
	setDefault(TLS_CERTIFICATE_FILE, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR "client.crt");
	setDefault(BOLD_FINISHED_DOWNLOADS, true);
	setDefault(BOLD_FINISHED_UPLOADS, true);
	setDefault(BOLD_QUEUE, true);
	setDefault(BOLD_HUB, true);
	setDefault(BOLD_PM, true);
	setDefault(BOLD_SEARCH, true);
	setDefault(BOLD_UPLOAD_QUEUE, true);
	setDefault(AUTO_REFRESH_TIME, 60);
	setDefault(USE_TLS, true);
	setDefault(AUTO_SEARCH_LIMIT, 15);
	setDefault(AUTO_KICK_NO_FAVS, false);
	setDefault(PROMPT_PASSWORD, true);
	setDefault(SPY_FRAME_IGNORE_TTH_SEARCHES, true);
	setDefault(ALLOW_UNTRUSTED_HUBS, true);
	setDefault(ALLOW_UNTRUSTED_CLIENTS, true);
	setDefault(SORT_FAVUSERS_FIRST, false);
	setDefault(SHOW_SHELL_MENU, false);
	setDefault(SEND_BLOOM, true);
	setDefault(NUMBER_OF_SEGMENTS, 3);
	setDefault(SEGMENTS_MANUAL, false);
	setDefault(HUB_SLOTS, 0);
	setDefault(PROFILES_URL, "");
	setDefault(EXTRA_SLOTS, 3);
	setDefault(EXTRA_PARTIAL_SLOTS, 1);
	setDefault(SHUTDOWN_TIMEOUT, 150);
	setDefault(SEARCH_PASSIVE, false);
	setDefault(MAX_UPLOAD_SPEED_LIMIT, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT, 0);
	setDefault(MAX_UPLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(TOOLBAR, "0,-1,1,2,-1,3,4,5,-1,6,7,8,9,-1,10,11,12,13,-1,14,15,16,17,-1,18,19,20,21");
	setDefault(WEBSERVER, false);
	setDefault(WEBSERVER_PORT, (int)Util::rand(80, 1024));
	setDefault(WEBSERVER_FORMAT,"%Y-%m-%d %H:%M: %[ip] tried getting %[file]");
	setDefault(LOG_WEBSERVER, false);
	setDefault(WEBSERVER_USER, "apexdc");
	setDefault(WEBSERVER_PASS, "apexdc");
	setDefault(AUTO_PRIORITY_DEFAULT, true);
	setDefault(TOOLBARIMAGE,"");
	setDefault(TOOLBARHOTIMAGE,"");
	setDefault(TIME_DEPENDENT_THROTTLE, false);
	setDefault(BANDWIDTH_LIMIT_START, 0);
	setDefault(BANDWIDTH_LIMIT_END, 0);
	setDefault(REMOVE_FORBIDDEN, true);
	setDefault(THROTTLE_ENABLE, false);
	setDefault(EXTRA_DOWNLOAD_SLOTS, 3);

	setDefault(BOLD_AUTHOR_MESS, true);
	setDefault(KICK_MSG_RECENT_01, "");
	setDefault(KICK_MSG_RECENT_02, "");
	setDefault(KICK_MSG_RECENT_03, "");
	setDefault(KICK_MSG_RECENT_04, "");
	setDefault(KICK_MSG_RECENT_05, "");
	setDefault(KICK_MSG_RECENT_06, "");
	setDefault(KICK_MSG_RECENT_07, "");
	setDefault(KICK_MSG_RECENT_08, "");
	setDefault(KICK_MSG_RECENT_09, "");
	setDefault(KICK_MSG_RECENT_10, "");
	setDefault(KICK_MSG_RECENT_11, "");
	setDefault(KICK_MSG_RECENT_12, "");
	setDefault(KICK_MSG_RECENT_13, "");
	setDefault(KICK_MSG_RECENT_14, "");
	setDefault(KICK_MSG_RECENT_15, "");
	setDefault(KICK_MSG_RECENT_16, "");
	setDefault(KICK_MSG_RECENT_17, "");
	setDefault(KICK_MSG_RECENT_18, "");
	setDefault(KICK_MSG_RECENT_19, "");
	setDefault(KICK_MSG_RECENT_20, "");
	setDefault(WINAMP_FORMAT, "/me playing: %[title] at %[bitrate] <Winamp %[version]>");
	setDefault(SHOW_INFOTIPS, true);
	setDefault(MINIMIZE_ON_STARTUP, false);
	setDefault(FREE_SLOTS_DEFAULT, false);
	setDefault(EXPAND_QUEUE, true);
	setDefault(TRANSFER_SPLIT_SIZE, 8000);

	setDefault(PERCENT_FAKE_SHARE_TOLERATED, 20);
	setDefault(CZCHARS_DISABLE, false);
	setDefault(REPORT_ALTERNATES, true);	

	setDefault(SOUNDS_DISABLED, false);
	setDefault(CHECK_NEW_USERS, false);
	setDefault(UPLOADQUEUEFRAME_SHOW_TREE, true);	
	setDefault(DONT_BEGIN_SEGMENT, true);
	setDefault(DONT_BEGIN_SEGMENT_SPEED, 512);

	setDefault(DISCONNECT_RAW, 0);
	setDefault(TIMEOUT_RAW, 0);
	setDefault(FAKESHARE_RAW, 0);
	setDefault(LISTLEN_MISMATCH, 0);
	setDefault(FILELIST_TOO_SMALL, 0);
	setDefault(FILELIST_UNAVAILABLE, 0);
	setDefault(DISPLAY_CHEATS_IN_MAIN_CHAT, true);	
	setDefault(SEARCH_TIME, 10);
	setDefault(SUPPRESS_MAIN_CHAT, false);
	setDefault(AUTO_SLOTS, 5);	
	
	// default sounds
	setDefault(BEGINFILE, Util::emptyString);
	setDefault(BEEPFILE, Util::emptyString);
	setDefault(FINISHFILE, Util::emptyString);
	setDefault(UPLOADFILE, Util::emptyString);
	setDefault(FAKERFILE, Util::emptyString);
	setDefault(CHATNAMEFILE, Util::emptyString);
	setDefault(SOUND_EXC, Util::emptyString);
	setDefault(SOUND_HUBCON, Util::emptyString);
	setDefault(SOUND_HUBDISCON, Util::emptyString);
	setDefault(SOUND_FAVUSER, Util::emptyString);
	setDefault(SOUND_TYPING_NOTIFY, Util::emptyString);

	setDefault(POPUP_HUB_CONNECTED, false);
	setDefault(POPUP_HUB_DISCONNECTED, false);
	setDefault(POPUP_FAVORITE_CONNECTED, true);
	setDefault(POPUP_CHEATING_USER, true);
	setDefault(POPUP_DOWNLOAD_START, true);
	setDefault(POPUP_DOWNLOAD_FAILED, false);
	setDefault(POPUP_DOWNLOAD_FINISHED, true);
	setDefault(POPUP_UPLOAD_FINISHED, false);
	setDefault(POPUP_PM, false);
	setDefault(POPUP_NEW_PM, true);
	setDefault(POPUP_TYPE, 0);
	setDefault(POPUP_AWAY, false);
	setDefault(POPUP_MINIMIZED, true);

	setDefault(AWAY, false);
	setDefault(SHUTDOWN_ACTION, 0);
	setDefault(MINIMUM_SEARCH_INTERVAL, 30);
	setDefault(PROGRESSBAR_ODC_STYLE, true);

	setDefault(MAX_AUTO_MATCH_SOURCES, 5);
	setDefault(MULTI_CHUNK, true);
	setDefault(USERLIST_DBLCLICK, 0);
	setDefault(TRANSFERLIST_DBLCLICK, 0);
	setDefault(CHAT_DBLCLICK, 0);	
	setDefault(HUBFRAME_VISIBLE, "1,1,0,1,0,1,0,0,0,0,0,0,0");
	setDefault(DIRECTORYLISTINGFRAME_VISIBLE, "1,1,0,1,1");	
	setDefault(FINISHED_VISIBLE, "1,1,1,1,1,1,1,1");
	setDefault(FINISHED_UL_VISIBLE, "1,1,1,1,1,1,1");
	setDefault(ACCEPTED_DISCONNECTS, 5);
	setDefault(ACCEPTED_TIMEOUTS, 10);
	setDefault(EMOTICONS_FILE, "Disabled");
	setDefault(GROUP_SEARCH_RESULTS, true);
	setDefault(TABS_POS, 1);
	setDefault(DOWNCONN_PER_SEC, 2);
	setDefault(FILTER_ENTER, false);
	setDefault(SHOW_QUICK_SEARCH, true);	
	setDefault(UC_SUBMENU, true);

	setDefault(DROP_MULTISOURCE_ONLY, true);
	setDefault(DISCONNECT_SPEED, 5);
	setDefault(DISCONNECT_FILE_SPEED, 15);
	setDefault(DISCONNECT_TIME, 40);
	setDefault(DISCONNECT_FILESIZE, 50);
    setDefault(REMOVE_SPEED, 2);
	setDefault(OVERLAP_CHUNKS, true);
	setDefault(KEEP_FINISHED_FILES, false);
	setDefault(SETTINGS_SAVE_INTERVAL, 10);

	// ApexDC++
	setDefault(SHOW_DESCRIPTION_LIMIT, true);
	setDefault(PROTECT_TRAY, false);
	setDefault(PROTECT_START, false);
	setDefault(PROTECT_CLOSE, false);
	setDefault(STRIP_TOPIC, false);
	setDefault(SKIPLIST_SHARE, Util::emptyString);
	setDefault(TB_IMAGE_SIZE, 20);
	setDefault(TB_IMAGE_SIZE_HOT, 20);
	setDefault(PM_PREVIEW, true);
	setDefault(HIGH_PRIO_FILES, "*.sfv;*.nfo;*sample*;*cover*;*.pls;*.m3u");
	setDefault(LOW_PRIO_FILES, Util::emptyString);
	setDefault(POPUP_TIME, 5);
	setDefault(AWAY_START, 0);
	setDefault(AWAY_END, 0);
	setDefault(AWAY_TIME_THROTTLE, false);
	setDefault(SECONDARY_AWAY_MESSAGE, Util::emptyString);
	setDefault(PROGRESSBAR_ODC_BUMPED, true);
	setDefault(TOP_SPEED, 100);
	setDefault(TOP_UP_SPEED, 50);
	setDefault(STEALTHY_STYLE, true);
	setDefault(IP_IN_CHAT, false);
	setDefault(COUNTRY_IN_CHAT, false);
	setDefault(SETTINGS_STATE, true);
	setDefault(PAGE, 0);
	setDefault(ENABLE_HUBTOPIC, false);
	setDefault(PROT_USERS, Util::emptyString);
	setDefault(PROT_FAVS, false);
	setDefault(MAX_MSG_LENGTH, 120);
	setDefault(OPEN_LOGS_INTERNAL, false);
	setDefault(GLOBAL_HUBFRAME_CONF, false);
	setDefault(USE_ANTIVIR, false);
	setDefault(ANTIVIR_PATH, Util::emptyString);
	setDefault(ANTIVIR_PARAMS, "%[file]");
	setDefault(STEALTHY_INDICATE_SPEEDS, false);
	setDefault(OLD_TRAY_BEHAV, false);
	setDefault(OLD_NAMING_STYLE, false);
	setDefault(OLD_ICONS_MODE, 0);
	setDefault(DELETE_CHECKED, false);
	setDefault(TOPMOST, false);
	setDefault(LOCK_TOOLBARS, true);
	setDefault(SEARCH_DETECT_TTH, true);
	setDefault(ZION_TABS, true);
	setDefault(NON_HUBS_FRONT, false);
	setDefault(GLOBAL_MINI_TABS, false);
	setDefault(BLEND_OFFLINE_SEARCH, true);
	setDefault(MAX_RESIZE_LINES, 1);
	setDefault(MAX_FILELISTS, 10);
	setDefault(CHECK_DELAY, 2000);
	setDefault(SLEEP_TIME, 500);
	setDefault(DELAYED_RAW_SENDING, true);
	setDefault(NAT_SORT, false);
	setDefault(USE_CUSTOM_LIST_BACKGROUND, false);
	setDefault(USE_FAV_NAMES, false);
	setDefault(OPEN_SYSTEM_LOG, false);
	setDefault(BOLD_SYSTEM_LOG, false);
	setDefault(DOT_HIDDEN_FILES, false);
	setDefault(HIDE_ANTIVIR, true);
	setDefault(RANDOM_SEGMENTS, false);
	setDefault(IP_SERVER, "http://checkip.dyndns.org/");

	setDefault(SEARCH_RESPONSES_ACTIVE, 10);
	setDefault(SEARCH_RESPONSES_PASSIVE, 5);

	setDefault(MAIN_WINDOW_STATE, SW_SHOWNORMAL);
	setDefault(MAIN_WINDOW_SIZE_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_SIZE_Y, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_Y, CW_USEDEFAULT);

	setDefault(MDI_MAXIMIZED, true);
	setDefault(FILE_OPEN_DIRECTORY, Util::emptyString);

	/* Theme settings */
	setDefault(TEXT_FONT, "MS Shell Dlg,-11,400,0");

	setDefault(BACKGROUND_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_COLOR, RGB(67, 98, 154));

	setDefault(TEXT_GENERAL_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_GENERAL_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_GENERAL_BOLD, false);
	setDefault(TEXT_GENERAL_ITALIC, false);

	setDefault(TEXT_MYOWN_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_MYOWN_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_MYOWN_BOLD, false);
	setDefault(TEXT_MYOWN_ITALIC, false);

	setDefault(TEXT_PRIVATE_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_PRIVATE_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_PRIVATE_BOLD, false);
	setDefault(TEXT_PRIVATE_ITALIC, true);

	setDefault(TEXT_SYSTEM_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_SYSTEM_FORE_COLOR, RGB(0, 128, 64));
	setDefault(TEXT_SYSTEM_BOLD, true);
	setDefault(TEXT_SYSTEM_ITALIC, false);

	setDefault(TEXT_SERVER_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_SERVER_FORE_COLOR, RGB(0, 128, 64));
	setDefault(TEXT_SERVER_BOLD, true);
	setDefault(TEXT_SERVER_ITALIC, false);

	setDefault(TEXT_TIMESTAMP_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_TIMESTAMP_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_TIMESTAMP_BOLD, false);
	setDefault(TEXT_TIMESTAMP_ITALIC, false);

	setDefault(TEXT_MYNICK_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_MYNICK_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_MYNICK_BOLD, true);
	setDefault(TEXT_MYNICK_ITALIC, false);

	setDefault(TEXT_FAV_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_FAV_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_FAV_BOLD, true);
	setDefault(TEXT_FAV_ITALIC, false);

	setDefault(TEXT_OP_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_OP_FORE_COLOR, RGB(0, 128, 64));
	setDefault(TEXT_OP_BOLD, true);
	setDefault(TEXT_OP_ITALIC, false);

	setDefault(TEXT_URL_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_URL_FORE_COLOR, RGB(0, 0, 255));
	setDefault(TEXT_URL_BOLD, false);
	setDefault(TEXT_URL_ITALIC, false);

	setDefault(ERROR_COLOR, RGB(255, 0, 0));
	setDefault(SEARCH_ALTERNATE_COLOUR, RGB(255,200,0));
	setDefault(COLOUR_DUPE, RGB(0, 174, 87));

	setDefault(MENUBAR_TWO_COLORS, true);
	setDefault(MENUBAR_LEFT_COLOR, RGB(0, 0, 128));
	setDefault(MENUBAR_RIGHT_COLOR, RGB(0, 64, 128));
	setDefault(MENUBAR_BUMPED, true);

	setDefault(NORMAL_COLOUR, RGB(67,98,154));
	setDefault(FAVORITE_COLOR, RGB(67,98,154));	
	setDefault(RESERVED_SLOT_COLOR, RGB(128,0,0));
	setDefault(IGNORED_COLOR, RGB(192,192,192));	
	setDefault(FIREBALL_COLOR, RGB(255,0,0));
 	setDefault(SERVER_COLOR, RGB(0,128,192));
	setDefault(OP_COLOR, RGB(0,128,64));
	setDefault(PROTECTED_COLOUR, RGB(60,181,111));
	setDefault(PASIVE_COLOR, RGB(67,98,154));
	setDefault(FULL_CHECKED_COLOUR, RGB(0, 128, 64));
	setDefault(BAD_CLIENT_COLOUR, RGB(192,0,0));
	setDefault(BAD_FILELIST_COLOUR, RGB(204,0,204));

	setDefault(PROGRESS_3DDEPTH, 4);
	setDefault(PROGRESS_OVERRIDE_COLORS, true);
	setDefault(PROGRESS_OVERRIDE_COLORS2, true);
	setDefault(PROGRESS_TEXT_COLOR_DOWN, RGB(255, 255, 255));
	setDefault(PROGRESS_TEXT_COLOR_UP, RGB(255, 255, 255));
	setDefault(UPLOAD_BAR_COLOR, RGB(255, 64, 0));
	setDefault(DOWNLOAD_BAR_COLOR, RGB(55, 170, 85));
	setDefault(PROGRESS_BACK_COLOR, RGB(95, 95, 95));
	setDefault(PROGRESS_COMPRESS_COLOR, RGB(222, 160, 0));
	setDefault(PROGRESS_SEGMENT_COLOR, RGB(49, 106, 197));
	setDefault(COLOR_RUNNING, RGB(0, 0, 100));
	setDefault(COLOR_DOWNLOADED, RGB(255, 255, 100));
	setDefault(COLOR_DONE, RGB(222, 160, 0));

	setDefault(POPUP_FONT, "MS Shell Dlg,-11,400,0");
	setDefault(POPUP_TITLE_FONT, "MS Shell Dlg,-11,400,0");
	setDefault(POPUP_BACKCOLOR, RGB(58, 122, 180));
	setDefault(POPUP_TEXTCOLOR, RGB(255, 255, 255));
	setDefault(POPUP_TITLE_TEXTCOLOR, RGB(255, 255, 255));
	
	setDefault(USE_DHT, false);
	setDefault(UPDATE_IP, false);
	setDefault(ALLOW_NAT_TRAVERSAL, true);
	setDefault(USE_EXPLORER_THEME, true);
	
#ifdef _WIN32
	OSVERSIONINFO ver;
	memzero(&ver, sizeof(OSVERSIONINFO));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((OSVERSIONINFO*)&ver);

	setDefault(USE_OLD_SHARING_UI, (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) ? false : true);
#endif

	setSearchTypeDefaults();
}

void SettingsManager::load(string const& aFileName)
{
	try {
		SimpleXML xml;

		xml.fromXML(File(aFileName, File::READ, File::OPEN).read());

		xml.resetCurrentChild();

		xml.stepIn();

		if(xml.findChild("Settings"))
		{
			xml.stepIn();

			int i;

			for(i=STR_FIRST; i<STR_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(StrSetting(i), xml.getChildData());
				xml.resetCurrentChild();
			}
			for(i=INT_FIRST; i<INT_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(IntSetting(i), Util::toInt(xml.getChildData()));
				xml.resetCurrentChild();
			}
			for(i=INT64_FIRST; i<INT64_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(Int64Setting(i), Util::toInt64(xml.getChildData()));
				xml.resetCurrentChild();
			}

			xml.stepOut();
		}

		xml.resetCurrentChild();
		if(xml.findChild("SearchTypes")) {
			try {
				searchTypes.clear();
				xml.stepIn();
				while(xml.findChild("SearchType")) {
					const string& extensions = xml.getChildData();
					if(extensions.empty()) {
						continue;
					}
					const string& name = xml.getChildAttrib("Id");
					if(name.empty()) {
						continue;
					}
					searchTypes[name] = StringTokenizer<string>(extensions, ';').getTokens();
				}
				xml.stepOut();
			} catch(const SimpleXMLException&) {
				setSearchTypeDefaults();
			}
		}

		if(SETTING(PRIVATE_ID).length() != 39 || CID(SETTING(PRIVATE_ID)).isZero()) {
			set(PRIVATE_ID, CID::generate().toBase32());
		}

		if(SETTING(CONFIG_VERSION).find(".") != string::npos) {
			double v = Util::toDouble(SETTING(CONFIG_VERSION));
			// if(v < 0.x) { // Fix old settings here }

			if(v <= 0.674) {
				// Formats changed, might as well remove these...
				unset(LOG_FORMAT_POST_DOWNLOAD);
				unset(LOG_FORMAT_POST_UPLOAD);
				unset(LOG_FORMAT_MAIN_CHAT);
				unset(LOG_FORMAT_PRIVATE_CHAT);
				unset(LOG_FORMAT_STATUS);
				unset(LOG_FORMAT_SYSTEM);
				unset(LOG_FILE_MAIN_CHAT);
				unset(LOG_FILE_STATUS);
				unset(LOG_FILE_PRIVATE_CHAT);
				unset(LOG_FILE_UPLOAD);
				unset(LOG_FILE_DOWNLOAD);
				unset(LOG_FILE_SYSTEM);
			}
		}

		auto v = Util::toInt64(SETTING(CONFIG_VERSION));
		if(v < 700) {
			unset(USE_DHT);
		} if(v < 725) {
			unset(USE_TLS);
			unset(PROFILES_URL);
		} if(v < 935 && SETTING(INCOMING_CONNECTIONS) != INCOMING_FIREWALL_PASSIVE) {
			set(AUTO_DETECT_CONNECTION, false); // Don't touch if it works
		} if (v <= 2227) {
			// add all the newly introduced default hublist servers automatically. 
			// change this to the COMPATIBLE_VERSIONID of the previous release each time a new default hublist server entry added.
			string lists = get(HUBLIST_SERVERS);

			StringTokenizer<string> t(getDefault(HUBLIST_SERVERS), ';');

			for(auto& i: t.getTokens()) {
				if(lists.find(i) == string::npos)
					lists += ";" + i;
			}

			set(HUBLIST_SERVERS, lists);
		}

		File::ensureDirectory(SETTING(TLS_TRUSTED_CERTIFICATES_PATH));

		fire(SettingsManagerListener::Load(), xml);

		xml.stepOut();

	} catch(const Exception&) {
		if(CID(SETTING(PRIVATE_ID)).isZero())
			set(PRIVATE_ID, CID::generate().toBase32());
	}

	if(SETTING(INCOMING_CONNECTIONS) == INCOMING_DIRECT) {
		set(TCP_PORT, (int)Util::rand(1025, 7999));
		set(TLS_PORT, SETTING(TCP_PORT) + 1);
		set(UDP_PORT, SETTING(TCP_PORT));
	}

	if(!isDefault(BIND_ADDRESS) && SETTING(BIND_ADDRESS) == "0.0.0.0")
		unset(BIND_ADDRESS);	

	if(SETTING(DHT_KEY).length() != 39 || CID(SETTING(DHT_KEY)).isZero())
		set(DHT_KEY, CID::generate().toBase32());
}

void SettingsManager::save(string const& aFileName) {

	SimpleXML xml;
	xml.addTag("DCPlusPlus");
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();

	int i;
	string type("type"), curType("string");

	for(i=STR_FIRST; i<STR_LAST; i++)
	{
		if(i == CONFIG_VERSION) {
			xml.addTag(settingTags[i], BOOST_STRINGIZE(COMPATIBLE_VERSIONID));
			xml.addChildAttrib(type, curType);
		} else if(isSet[i]) {
			xml.addTag(settingTags[i], get(StrSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}

	curType = "int";
	for(i=INT_FIRST; i<INT_LAST; i++)
	{
		if(isSet[i]) {
			xml.addTag(settingTags[i], get(IntSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	curType = "int64";
	for(i=INT64_FIRST; i<INT64_LAST; i++)
	{
		if(isSet[i])
		{
			xml.addTag(settingTags[i], get(Int64Setting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	xml.stepOut();
	
	xml.addTag("SearchTypes");
	xml.stepIn();
	for(SearchTypesIterC i = searchTypes.begin(); i != searchTypes.end(); ++i) {
		xml.addTag("SearchType", Util::toString(";", i->second));
		xml.addChildAttrib("Id", i->first);
	}
	xml.stepOut();

	fire(SettingsManagerListener::Save(), xml);

	try {
		File out(aFileName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		out.close();
		File::deleteFile(aFileName);
		File::renameFile(aFileName + ".tmp", aFileName);
	} catch(...) {
		// ...
	}
}

void SettingsManager::validateSearchTypeName(const string& name) const {
	if(name.empty() || (name.size() == 1 && name[0] >= '1' && name[0] <= '6')) {
		throw SearchTypeException("Invalid search type name"); // TODO: localize
	}
	for(int type = SearchManager::TYPE_ANY; type != SearchManager::TYPE_LAST; ++type) {
		if(SearchManager::getTypeStr(type) == name) {
			throw SearchTypeException("This search type already exists"); // TODO: localize
		}
	}
}

void SettingsManager::setSearchTypeDefaults() {
	searchTypes.clear();

	// for conveniency, the default search exts will be the same as the ones defined by SEGA.
	const auto& searchExts = AdcHub::getSearchExts();
	for(size_t i = 0, n = searchExts.size(); i < n; ++i)
		searchTypes[string(1, '1' + i)] = searchExts[i];

	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::addSearchType(const string& name, const StringList& extensions, bool validated) {
	if(!validated) {
		validateSearchTypeName(name);
	}

	if(searchTypes.find(name) != searchTypes.end()) {
		throw SearchTypeException("This search type already exists"); // TODO: localize
	}

	searchTypes[name] = extensions;
	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::delSearchType(const string& name) {
	validateSearchTypeName(name);
	searchTypes.erase(name);
	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::renameSearchType(const string& oldName, const string& newName) {
	validateSearchTypeName(newName);
	StringList exts = getSearchType(oldName)->second;
	addSearchType(newName, exts, true);
	searchTypes.erase(oldName);
}

void SettingsManager::modSearchType(const string& name, const StringList& extensions) {
	getSearchType(name)->second = extensions;
	fire(SettingsManagerListener::SearchTypesChanged());
}

const StringList& SettingsManager::getExtensions(const string& name) {
	return getSearchType(name)->second;
}

SettingsManager::SearchTypesIter SettingsManager::getSearchType(const string& name) {
	SearchTypesIter ret = searchTypes.find(name);
	if(ret == searchTypes.end()) {
		throw SearchTypeException("No such search type"); // TODO: localize
	}
	return ret;
}

bool SettingsManager::getType(const char* name, int& n, Types& type) const {
	for(n = 0; n < SETTINGS_LAST; ++n) {
		if(strcmp(settingTags[n].c_str(), name) == 0) {
			if(n < STR_LAST) {
				type = TYPE_STRING;
				return true;
			} else if(n < INT_LAST) {
				type = TYPE_INT;
				return true;
			} else {
				type = TYPE_INT64;
				return true;
			}
		}
	}
	return false;
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
