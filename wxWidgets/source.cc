#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#	include <wx/wx.h>
#endif

#include <wx/setup.h>

enum
{
	ID_Hello = 1
};

class MyPanel :public wxPanel {
public:
	MyPanel(wxFrame *parent) : wxPanel(parent) {
		Bind(wxEVT_PAINT, &MyPanel::OnPaint, this);

	};

	void OnPaint(wxPaintEvent &event) {
		wxPaintDC dc(this);

		dc.DrawPoint(wxPoint(10, 10));
		dc.DrawLine(wxPoint(15, 15), wxPoint(75, 75));
		dc.DrawRectangle(wxPoint(80, 35), wxSize(50, 45));

		const wxPoint points[] = {
			wxPoint(120, 120),
			wxPoint(120, 160),
			wxPoint(140, 160),
		};
		dc.DrawPolygon(3, points);

		dc.DrawCircle(wxPoint(280, 100), 80);

		dc.SetTextForeground(wxColor(255, 0, 255));
		dc.SetFont(wxFontInfo(12).Bold(2).FaceName(wxT("MS yahei")));
		dc.DrawText(wxT("²âÊÔÎÄ×Ö"), wxPoint(200, 160));
	}
};

class MyFrame : public wxFrame
{
public:
	MyFrame()
		:wxFrame(NULL, wxID_ANY, "Hello World")
	{
		//SetIcon(wxIcon());

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

		CreateStatusBar(2);
		SetStatusText("Welcome to wxWidgets!");

		Bind(wxEVT_MENU, &MyFrame::OnHello, this, ID_Hello);
		Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
		Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);

		//MyPanel *panel = new MyPanel(this);
	}

	void OnMotion(wxMouseEvent &event)
	{
		if (event.Dragging())
		{
			wxClientDC dc(this);
			wxPen pen(*wxRED, 1);
			dc.SetPen(pen);
			dc.DrawPoint(event.GetPosition());
			dc.SetPen(wxNullPen);
		}
	}

	void OnHello(wxCommandEvent &event)
	{
		wxLogMessage("Hello world from wxWidgets!");
	}

	void OnExit(wxCommandEvent &event)
	{
		Close(true);
	}

	void OnAbout(wxCommandEvent &event)
	{
		wxString message;
		message.Printf(wxT("Hello and welcome to %s"), wxVERSION_STRING);
		wxMessageBox(message, wxT("About Minimal"), wxOK | wxICON_INFORMATION, this);
		//wxMessageBox("This is a wxWidgets Hello World example",
			//"About Hello World", wxOK | wxICON_INFORMATION);
	}

	void OnErase(wxEraseEvent &event)
	{
		//wxDC *dc = event.GetDC();
		//if (!dc) {
		//	wxClientDC cdc(this);
		//	dc = &cdc;
		//}
		//wxSize size = GetClientSize();

		//wxBitmap bitmap("friends.png", wxBITMAP_TYPE_PNG);
		//dc->Clear();
		//dc->DrawBitmap(bitmap, wxPoint(0, 0), true);
	}

private:
	DECLARE_EVENT_TABLE();

};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_MOTION(MyFrame::OnMotion)
	EVT_ERASE_BACKGROUND(MyFrame::OnErase)
END_EVENT_TABLE()

class MyApp : public wxApp
{
public:
	virtual bool OnInit()
	{
		wxImage::AddHandler(new wxPNGHandler());

		MyFrame *frame = new MyFrame();
		frame->Show(true);
		return true;
	}
};

wxIMPLEMENT_APP(MyApp);