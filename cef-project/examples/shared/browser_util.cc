// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "examples/shared/browser_util.h"

#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

namespace shared {

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class WindowDelegate : public CefWindowDelegate {
 public:
  explicit WindowDelegate(CefRefPtr<CefBrowserView> browser_view)
      : browser_view_(browser_view) {}

  void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE {
    // Add the browser view and show the window.
    window->AddChildView(browser_view_);
    window->Show();

    // Give keyboard focus to the browser view.
    browser_view_->RequestFocus();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE {
    browser_view_ = NULL;
  }

  bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE {
    // Allow the window to close if the browser says it's OK.
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    if (browser)
      return browser->GetHost()->TryCloseBrowser();
    return true;
  }

  CefSize GetPreferredSize(CefRefPtr<CefView> view) OVERRIDE {
    // Preferred window size.
    return CefSize(800, 600);
  }

  CefSize GetMinimumSize(CefRefPtr<CefView> view) OVERRIDE {
    // Minimum window size.
    return CefSize(200, 100);
  }

 private:
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(WindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(WindowDelegate);
};

}  // namespace

void CreateBrowser(CefRefPtr<CefClient> client,
                   const CefString& startup_url,
                   const CefBrowserSettings& settings)
{
	CEF_REQUIRE_UI_THREAD();

	CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();
	const bool hasHwnd = command_line->HasSwitch("hwnd");
	CefWindowInfo window_info;
	
	if (hasHwnd) {
		const std::string& strHwnd = command_line->GetSwitchValue("hwnd");
		RECT wnd_rect = { 0, 0, 400, 400 };

		window_info.SetAsChild((HWND)(atoi(strHwnd.c_str())), wnd_rect);
	} else {
		window_info.SetAsPopup(NULL, "Volume Controller - Gary Hsieh");
		window_info.height = 400;
		window_info.width = 400;
	}

	CefBrowserHost::CreateBrowser(window_info, client, startup_url, settings, NULL);
}

}  // namespace shared
