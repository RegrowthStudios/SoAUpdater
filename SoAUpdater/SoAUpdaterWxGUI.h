#pragma once
class SoAUpdater : public wxApp
{
public:
	virtual bool OnInit();
};

class SoAUpdaterFrame : public wxFrame
{
public:
	SoAUpdaterFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:
	void OnHello(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

enum
{
	ID_Hello = 1
};

wxBEGIN_EVENT_TABLE(SoAUpdaterFrame, wxFrame)
EVT_MENU(ID_Hello, SoAUpdaterFrame::OnHello)
EVT_MENU(wxID_EXIT, SoAUpdaterFrame::OnExit)
EVT_MENU(wxID_ABOUT, SoAUpdaterFrame::OnAbout)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(SoAUpdater);
