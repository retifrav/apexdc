/* 
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#ifdef _DEBUG
/** 
	Memory leak detector
	You can remove following 3 lines if you don't want to detect memory leaks.
	Ignore STL pseudo-leaks, we can avoid them with _STLP_LEAKS_PEDANTIC, but it only slows down everything.
 */
#define VLD_MAX_DATA_DUMP 0
#define VLD_AGGREGATE_DUPLICATES
//#include <vld.h>
#endif

#include <client/DCPlusPlus.h>
#include <client/MerkleTree.h>
#include <client/MappingManager.h>

#include "SingleInstance.h"
#include "WinUtil.h"
#include "ExceptionDlg.h"
#include "SplashWindow.h"

#include "Mapper_NATPMP.h"
#include "Mapper_MiniUPnPc.h"
#include "Mapper_WinUPnP.h"

#include "ExtendedTrace.h"

#include "MainFrm.h"
#include "PluginApiWin.h"

#include <future>
#include <delayimp.h>

CAppModule _Module;

CriticalSection cs;
enum { DEBUG_BUFSIZE = 8192 };
static char guard[DEBUG_BUFSIZE];
static int recursion = 0;
static char tth[((24 * 8) / 5) + 2];
static bool firstException = true;

static char buf[DEBUG_BUFSIZE];

EXCEPTION_RECORD CurrExceptionRecord;
CONTEXT CurrContext;
int iLastExceptionDlgResult;

#ifndef _DEBUG

FARPROC WINAPI FailHook(unsigned /* dliNotify */, PDelayLoadInfo  pdli) {
	char buf[DEBUG_BUFSIZE];
	sprintf(buf, "ApexDC++ just encountered and unhandled exception and will terminate.\nPlease do not report this as a bug. The error was caused by library %s.", pdli->szDll);
	MessageBox(WinUtil::mainWnd, Text::toT(buf).c_str(), _T("ApexDC++ Has Crashed"), MB_OK | MB_ICONERROR);
	exit(-1);
}

#ifndef DELAYIMP_INSECURE_WRITABLE_HOOKS
const
#endif
PfnDliHook __pfnDliFailureHook2 = FailHook;

#endif

#include <client/SSLSocket.h>

string getExceptionName(DWORD code) {
	switch(code)
    { 
		case EXCEPTION_ACCESS_VIOLATION:
			return "Access violation"; break; 
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return "Array out of range"; break; 
		case EXCEPTION_BREAKPOINT:
			return "Breakpoint"; break; 
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return "Read or write error"; break; 
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			return "Floating-point error"; break; 
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return "Floating-point division by zero"; break; 
		case EXCEPTION_FLT_INEXACT_RESULT:
			return "Floating-point inexact result"; break; 
		case EXCEPTION_FLT_INVALID_OPERATION:
			return "Unknown floating-point error"; break; 
		case EXCEPTION_FLT_OVERFLOW:
			return "Floating-point overflow"; break; 
		case EXCEPTION_FLT_STACK_CHECK:
			return "Floating-point operation caused stack overflow"; break; 
		case EXCEPTION_FLT_UNDERFLOW:
			return "Floating-point underflow"; break; 
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return "Illegal instruction"; break; 
		case EXCEPTION_IN_PAGE_ERROR:
			return "Page error"; break; 
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return "Integer division by zero"; break; 
		case EXCEPTION_INT_OVERFLOW:
			return "Integer overflow"; break; 
		case EXCEPTION_INVALID_DISPOSITION:
			return "Invalid disposition"; break; 
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			return "Noncontinueable exception"; break; 
		case EXCEPTION_PRIV_INSTRUCTION:
			return "Invalid instruction"; break; 
		case EXCEPTION_SINGLE_STEP:
			return "Single step executed"; break; 
		case EXCEPTION_STACK_OVERFLOW:
			return "Stack overflow"; break; 
	}
	return "Unknown";
}

void ExceptionFunction() {
	if (iLastExceptionDlgResult == IDCANCEL) {
		ExitProcess(1);
	}
}

LONG __stdcall DCUnhandledExceptionFilter( LPEXCEPTION_POINTERS e )
{	
	Lock l(cs);

	if(recursion++ > 30)
		exit(-1);

#ifndef _DEBUG
	// The release version loads the dll and pdb:s here...
	EXTENDEDTRACEINITIALIZE( Util::getPath(Util::PATH_RESOURCES).c_str() );
#endif

	if(firstException) {
		File::deleteFile(Util::getFatalLogPath() + "exceptioninfo.txt");
		firstException = false;
	}

	if(File::getSize(Util::getPath(Util::PATH_RESOURCES) + PDB_NAME) == -1) {
		// No debug symbols, we're not interested...
		::MessageBox(WinUtil::mainWnd, _T("ApexDC++ has crashed, but in order for you to report the problem you need to have the program debug database (") _T(PDB_NAME) _T(") in your application directory."), _T("ApexDC++ has crashed"), MB_OK);
#ifndef _DEBUG
		exit(1);
#else
		return EXCEPTION_CONTINUE_SEARCH;
#endif
	}

	File f(Util::getFatalLogPath() + "exceptioninfo.txt", File::RW, File::OPEN | File::CREATE);
	f.setEndPos(0);

	// store position to show only one exception in dialog
	int64_t pos = f.getPos();

	DWORD exceptionCode = e->ExceptionRecord->ExceptionCode;
	sprintf(buf, "Code: %x (%s)\r\nVersion: %s (%s)\r\n", 
		exceptionCode, getExceptionName(exceptionCode).c_str(), VERSIONSTRING, __DATE__);

	f.write(buf, strlen(buf));

	OSVERSIONINFOEX ver;
	WinUtil::getVersionInfo(ver);

	sprintf(buf, "Major: %d\r\nMinor: %d\r\nBuild: %d\r\nSP: %d\r\nType: %d\r\n",
		(DWORD)ver.dwMajorVersion, (DWORD)ver.dwMinorVersion, (DWORD)ver.dwBuildNumber,
		(DWORD)ver.wServicePackMajor, (DWORD)ver.wProductType);

	f.write(buf, strlen(buf));

	time_t now;
	time(&now);
	strftime(buf, DEBUG_BUFSIZE, "Time: %Y-%m-%d %H:%M:%S\r\n", localtime(&now));
	f.write(buf, strlen(buf));

	f.write(LIT("TTH: "));
	f.write(tth, strlen(tth));
	f.write(LIT("\r\n\r\n"));

	STACKTRACE(f, e->ContextRecord);

	f.write(LIT("\r\n"));

	f.setPos(pos);
	WinUtil::exceptioninfo += Text::toT(f.read());

	f.close();

	if ((!SETTING(SOUND_EXC).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
		PlaySound(Text::toT(SETTING(SOUND_EXC)).c_str(), NULL, SND_FILENAME | SND_ASYNC);

	if(MainFrame::getMainFrame()) {
		NOTIFYICONDATA m_nid;
		m_nid.cbSize = sizeof(NOTIFYICONDATA);
		m_nid.hWnd = MainFrame::getMainFrame()->m_hWnd;
		m_nid.uID = 0;
		m_nid.uFlags = NIF_INFO;
		m_nid.uTimeout = 5000;
		m_nid.dwInfoFlags = NIIF_WARNING;
		_tcscpy(m_nid.szInfo, _T("exceptioninfo.txt was generated"));
		_tcscpy(m_nid.szInfoTitle, _T("ApexDC++ has crashed"));
		Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	}

	CExceptionDlg dlg;
	iLastExceptionDlgResult = dlg.DoModal(WinUtil::mainWnd);
	ExceptionFunction();

#ifndef _DEBUG
	EXTENDEDTRACEUNINITIALIZE();
	
	return EXCEPTION_CONTINUE_EXECUTION;
#else
	return EXCEPTION_CONTINUE_SEARCH;
#endif
}

static void sendCmdLine(HWND hOther, LPTSTR lpstrCmdLine)
{
	tstring cmdLine = lpstrCmdLine;
	LRESULT result;

	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = sizeof(TCHAR)*(cmdLine.length() + 1);
	cpd.lpData = (void *)cmdLine.c_str();
	result = SendMessage(hOther, WM_COPYDATA, NULL,	(LPARAM)&cpd);
}

BOOL CALLBACK searchOtherInstance(HWND hWnd, LPARAM lParam) {
	ULONG_PTR result;
	LRESULT ok = ::SendMessageTimeout(hWnd, WMU_WHERE_ARE_YOU, 0, 0,
		SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &result);
	if(ok == 0)
		return TRUE;
	if(result == WMU_WHERE_ARE_YOU) {
		// found it
		HWND *target = (HWND *)lParam;
		*target = hWnd;
		return FALSE;
	}
	return TRUE;
}

static void checkCommonControls() {
#define PACKVERSION(major,minor) MAKELONG(minor,major)

	HINSTANCE hinstDll;
	DWORD dwVersion = 0;
	
	hinstDll = LoadLibrary(_T("comctl32.dll"));
	
	if(hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
	
		pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");
		
		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;
			
			memzero(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);
			
			hr = (*pDllGetVersion)(&dvi);
			
			if(SUCCEEDED(hr))
			{
				dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
			}
		}
		
		FreeLibrary(hinstDll);
	}

	if(dwVersion < PACKVERSION(5,80)) {
		MessageBox(NULL, _T("Your version of windows common controls is too old for ApexDC++ to run correctly, and you will most probably experience problems with the user interface. You should download version 5.80 or higher from the DC++ homepage or from Microsoft directly."), _T("User Interface Warning"), MB_OK);
	}
}

static int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	checkCommonControls();

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	static auto splash = SplashWindow::create(_T(VERSIONSTRING) _T(" ") _T(CONFIGURATION_TYPE), _T("Starting ") _T(APPNAME) _T("..."));
	unique_ptr<MainFrame> wndMain;

	auto loader = std::async(std::launch::async, [&]() {
		dcpp::startup();

		PluginApiWin::init();

		MappingManager::getInstance()->addMapper<Mapper_NATPMP>();
		MappingManager::getInstance()->addMapper<Mapper_MiniUPnPc>();
		MappingManager::getInstance()->addMapper<Mapper_WinUPnP>();

		dcpp::load([](const string& text) { splash->update(text); }, [](float progress) { splash->update(progress); });

		if(ResourceManager::getInstance()->isRTL()) {
			SetProcessDefaultLayout(LAYOUT_RTL);
		}

		splash->update("Creating User Interface...");

		// This yields execution through SplashWindows message loop
		splash->callAsync([&] {
			wndMain.reset(new MainFrame());

			// register for message filtering and idle updates
			theLoop.AddMessageFilter(wndMain.get());
			theLoop.AddIdleHandler(wndMain.get());

			CRect rc = wndMain->rcDefault;

			if( (SETTING(MAIN_WINDOW_POS_X) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_POS_Y) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_SIZE_X) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_SIZE_Y) != CW_USEDEFAULT) ) {

				rc.left = SETTING(MAIN_WINDOW_POS_X);
				rc.top = SETTING(MAIN_WINDOW_POS_Y);
				rc.right = rc.left + SETTING(MAIN_WINDOW_SIZE_X);
				rc.bottom = rc.top + SETTING(MAIN_WINDOW_SIZE_Y);
				// Now, let's ensure we have sane values here...
				if( (rc.left < 0 ) || (rc.top < 0) || (rc.right - rc.left < 10) || ((rc.bottom - rc.top) < 10) ) {
					rc = wndMain->rcDefault;
				}
			}

			int rtl = ResourceManager::getInstance()->isRTL() ? WS_EX_RTLREADING : 0;
			if(wndMain->CreateEx(NULL, rc, 0, rtl | WS_EX_APPWINDOW | WS_EX_WINDOWEDGE) == NULL) {
				ATLTRACE(_T("Main window creation failed!\n"));
				return;
			}

			splash->show(false);

			if(BOOLSETTING(MINIMIZE_ON_STARTUP)) {
				wndMain->ShowWindow(SW_SHOWMINIMIZED);
			} else {
				wndMain->ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? SETTING(MAIN_WINDOW_STATE) : nCmdShow);
			}
		});
	});

	int nRet = theLoop.Run();
	_Module.RemoveMessageLoop();

	splash->destroy();

	ExCImage::ReleaseGDIPlus();

	dcpp::shutdown();

	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	SingleInstance dcapp(_T(INST_NAME));

	LPTSTR* argv = ++__targv;
	int argc = --__argc;
	bool multiple = false;

	for (;;) {
		if(argc <= 0) break;
		if(_tcscmp(*argv, _T("/uninstall")) == 0) {
			if(!dcapp.IsAnotherInstanceRunning()) {
				WinUtil::uninstallApp();
				return FALSE;
			} else {
				::MessageBox(NULL, _T("Please close all running instances and attempt againg."), _T(APPNAME) _T(" Uninstall"), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
				return FALSE;
			}
		} else if(_tcscmp(*argv, _T("/sign")) == 0 && --argc >= 2) {
			string xmlPath = Text::fromT(*++argv);
			string keyPath = Text::fromT(*++argv);
			bool genHeader = (argc > 2 && (_tcscmp(*++argv, _T("-pubout")) == 0));
			if(Util::fileExists(xmlPath) && Util::fileExists(keyPath))
				UpdateManager::signVersionFile(xmlPath, keyPath, genHeader);
			return FALSE;
		} else if(_tcscmp(*argv, _T("/update")) == 0) {
			if(--argc >= 2) {
				string sourcePath = Text::fromT(*++argv);
				string installPath = Text::fromT(*++argv);

				SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

				bool success = false;
				for(int i = 0; i < 5 && (success = UpdateManager::applyUpdate(sourcePath, installPath)) == false; ++i)
					Thread::sleep(1000);

				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
				SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

				if(argc > 2 && (_tcscmp(*++argv, _T("-restart")) == 0)) {
					LPTSTR cmdLine = success ? _T("/updated /silent") : _T("/silent");
					ShellExecute(NULL, NULL, Text::toT(installPath + Util::getFileName(WinUtil::getAppName())).c_str(), cmdLine, NULL, SW_SHOW);
				}
				return FALSE;
			} else {
				// This is compatibility code for updating from 1.4.x
				SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

				while(Util::fileExists(WinUtil::getAppName() + ".bak")) {
					Thread::sleep(5000);
					File::deleteFile(WinUtil::getAppName() + ".bak");
				}

				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
				SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
				multiple = true;
				break;
			}
		} else {
			if(_tcscmp(*argv, _T("/crash")) == 0) {
				// going for a timeout
				Thread::sleep(5000);
				multiple = true;
			}
			if(_tcscmp(*argv, _T("/silent")) == 0)
				multiple = true;
		}

		argv++;
		argc--;
	}

	if(dcapp.IsAnotherInstanceRunning()) {
		// Allow for more than one instance...
		if(_tcslen(lpstrCmdLine) == 0) {
			if (::MessageBox(NULL, _T("There is already an instance of ApexDC++ running.\nDo you want to launch another instance anyway?"), 
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
					multiple = true;
			}
		}

		if(multiple == false) {
			HWND hOther = NULL;
			EnumWindows(searchOtherInstance, (LPARAM)&hOther);

			if( hOther != NULL ) {
				// pop up
				::SetForegroundWindow(hOther);

				if( IsIconic(hOther)) {
					// restore
					::ShowWindow(hOther, SW_RESTORE);
				}
				sendCmdLine(hOther, lpstrCmdLine);
			}
			return FALSE;
		}
	}

	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
#ifdef _DEBUG
	EXTENDEDTRACEINITIALIZE(Util::getPath(Util::PATH_RESOURCES).c_str());
#endif
	LPTOP_LEVEL_EXCEPTION_FILTER pOldSEHFilter = NULL;
	pOldSEHFilter = SetUnhandledExceptionFilter(&DCUnhandledExceptionFilter);
	
	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES |
		ICC_TAB_CLASSES | ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES);	// add flags to support other controls
	
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	
	try {		
		File f(WinUtil::getAppName(), File::READ, File::OPEN);
		TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));
		size_t n = 0;
		size_t n2 = DEBUG_BUFSIZE;
		while((n = f.read(buf, n2)) > 0) {
			tth.update(buf, n);
			n2 = DEBUG_BUFSIZE;
		}
		tth.finalize();
		strncpy(::tth, tth.getRoot().toBase32().c_str(), sizeof(::tth));
		WinUtil::tth = Text::toT(::tth);
	} catch(const FileException&) {
		dcdebug("Failed reading exe\n");
	}	

	HINSTANCE hInstRich = ::LoadLibrary(ChatCtrl::GetLibraryName());	

	int nRet = Run(lpstrCmdLine, nCmdShow);
 
	if ( hInstRich ) {
		::FreeLibrary(hInstRich);
	}
	
	// Return back old VS SEH handler
	if (pOldSEHFilter != NULL)
		SetUnhandledExceptionFilter(pOldSEHFilter);

	_Module.Term();
	::CoUninitialize();

#ifdef _DEBUG
	EXTENDEDTRACEUNINITIALIZE();
#endif

	return nRet;
}

/**
 * @file
 * $Id$
 */
