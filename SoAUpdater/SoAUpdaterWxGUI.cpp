#include "stdafx.h"
#include "SoAUpdaterWxGUI.h"

#if !wxUSE_WEBVIEW_WEBKIT && !wxUSE_WEBVIEW_IE
#error "A wxWebView backend is required by this sample"
#endif

#include "wx/artprov.h"
#include "wx/cmdline.h"
#include "wx/notifmsg.h"
#include "wx/settings.h"
#include "wx/webview.h"
#include "wx/webviewarchivehandler.h"
#include "wx/webviewfshandler.h"
#include "wx/infobar.h"
#include "wx/filesys.h"
#include "wx/fs_arc.h"
#include "wx/fs_mem.h"


bool SoAUpdater::OnInit()
{
	SoAUpdaterFrame *frame = new SoAUpdaterFrame("Hello World", wxPoint(50, 50), wxSize(450, 340));
	frame->Show(true);
	return true;
}

SoAUpdaterFrame::SoAUpdaterFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
		"Help string shown in status bar for this menu item");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");

	SetMenuBar(menuBar);

	CreateStatusBar();
	SetStatusText("Welcome to wxWidgets!");
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	wxWebView *m_browser = wxWebView::New(this, wxID_ANY, "http://seedofandromeda.com/updater/");
	sizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));
	wxButton *launchButton = new wxButton(this, -1, "Play SoA");
	sizer->Add(launchButton, 0, 0, 0);
	SetSizer(sizer);
}

void SoAUpdaterFrame::OnExit(wxCommandEvent& event)
{
	Close(true);
}

void SoAUpdaterFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox("This is a wxWidgets' Hello world sample",
		"About Hello World", wxOK | wxICON_INFORMATION);
}

void SoAUpdaterFrame::OnHello(wxCommandEvent& event)
{
	wxLogMessage("Hello world from wxWidgets!");
}