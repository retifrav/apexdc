/*
 * Copyright (C) 2001-2014 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_SPLASHWINDOW_H
#define DCPLUSPLUS_WIN32_SPLASHWINDOW_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <string>

#include <client/typedefs.h>
#include <client/version.h>

#include "ResourceLoader.h"

using std::string;

template <typename Base>
class SimpleAsyncWindowImpl : public CWindowImpl<Base>
{
public:
	typedef CWindowImpl<Base> baseClass;
	typedef SimpleAsyncWindowImpl<Base> thisClass;

	typedef std::function<void ()> asyncF;

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
	END_MSG_MAP()

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		auto func_ptr = reinterpret_cast<asyncF*>(lParam);
		(*func_ptr)();
		delete func_ptr;

		return FALSE;
	}

	void callAsync(const asyncF& func) {
		PostMessage(WM_SPEAKER, NULL, (LPARAM)new asyncF(func));
	}
};

class SplashWindow : public SimpleAsyncWindowImpl<SplashWindow>
{
public:
	typedef SimpleAsyncWindowImpl<SplashWindow> baseClass;

	DECLARE_WND_CLASS(_T("ApexSplashWindow"))

	BEGIN_MSG_MAP(SplashWindow)
		MESSAGE_HANDLER(WM_PAINT, onPaint)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkgnd)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		if(GetUpdateRect(NULL)) {
			// Get some information
			RECT rc;
			CPaintDC dc(m_hWnd);
			GetWindowRect(&rc);

			OffsetRect(&rc, -rc.left, -rc.top);
			rc.top = rc.bottom - 15; 
			rc.right = rc.right - 5;
			dc.SetBkMode(TRANSPARENT);

			{			 
				HDC comp = CreateCompatibleDC(dc);
				HGDIOBJ old = SelectObject(comp, img);	

				dc.BitBlt(0, 0, 350, 120, comp, 0, 0, SRCCOPY);

				SelectObject(comp, old);
				DeleteDC(comp);
			}

			HFONT oldFont = dc.SelectFont(hFont);
			dc.SetTextColor(RGB(255,255,255));
			dc.DrawText(title.c_str(), title.size(), &rc, DT_RIGHT);

			if(!status.empty()) {
				tstring fullStatus = _T(" ") + status + _T(" ");
				if(progress)
					fullStatus += _T(" [") + Text::toT(Util::toString(progress * 100.)) + _T("%]");
				dc.DrawText(fullStatus.c_str(), fullStatus.size(), &rc, DT_LEFT);
			}

			dc.SelectFont(oldFont);
		}

		return FALSE;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if(closing)
			bHandled = FALSE;
		return FALSE;
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		img.LoadFromResource(IDR_SPLASH, _T("PNG"), _Module.get_m_hInst());
		CRect rc;

		rc.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
		rc.top = (rc.bottom / 2) - 80;
		rc.bottom = rc.top + img.GetHeight();

		rc.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rc.left = rc.right / 2 - 85;
		rc.right = rc.left + img.GetWidth();

		SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));

		LOGFONT logFont;
		GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
		lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
		logFont.lfHeight = 12;
		logFont.lfWeight = 700;
		hFont = CreateFontIndirect(&logFont);

		HideCaret();
		SetWindowPos(HWND_TOP, &rc, SWP_SHOWWINDOW);
		CenterWindow();

		BringWindowToTop();
		SetFocus();

		RedrawWindow();

		bHandled = FALSE;
		return TRUE;
	}

	LRESULT onEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = TRUE;
		return TRUE;
	}

	SplashWindow(const tstring& title, const tstring& status = Util::emptyStringT) : status(status), title(title) { }

	~SplashWindow() {
		if(!img.IsNull())
			img.Destroy();

		DeleteObject(hFont);
	}

	void OnFinalMessage(HWND /*hWnd*/) { delete this; }

	// SplashWindow::create and SplashWindow::destroy must be called from the same thread
	static SplashWindow* create(const tstring& title, const tstring& status = Util::emptyStringT) {
		auto splash = new SplashWindow(title, status);
		splash->Create(GetDesktopWindow(), rcDefault, _T(APPNAME), WS_POPUP | WS_VISIBLE | SS_USERITEM | WS_EX_TOOLWINDOW | WS_EX_TOPMOST);
		return splash;
	}

	// This is synchronous so that OnFinalMessage is quaranteed to get called
	void destroy() {
		closing = true;
		SendMessage(WM_CLOSE, 0, 0);
	}

	void show(bool show = true) {
		callAsync([this, show]() {
			status.clear();
			progress = 0;

			RedrawWindow();
			ShowWindow(show ? SW_SHOW : SW_HIDE);
		});
	}

	void update(const string& newStatus) {
		callAsync([this, newStatus]() {
			status = Text::toT(newStatus);
			progress = 0;
			RedrawWindow();
		});
	}

	void update(float newProgress) {
		callAsync([this, newProgress]() {
			if(progress == 0.00 || newProgress - progress >= 0.01) {
				progress = newProgress;
				RedrawWindow();
			}
		});
	}

private:
	tstring status, title;

	ExCImage img;

	float progress = 0;
	bool closing = false;

	HFONT hFont;
};

#endif