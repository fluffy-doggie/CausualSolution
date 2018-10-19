#include <wx/wx.h>
#include "MyApp.h"
#include "MyFrame.h"

bool MyApp::OnInit()
{
	MyFrame *myFrame = new MyFrame("MyApp", wxDefaultPosition, wxDefaultSize);
	myFrame->Show();
	return true;
}