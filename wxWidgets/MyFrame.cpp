#include <wx/wx.h>

#include "MyFrame.h"

MyFrame::MyFrame(
	const wxString & title, 
	const wxPoint & pos, 
	const wxSize & size)
: wxFrame(NULL, wxID_ANY, title, pos, size)
{
}