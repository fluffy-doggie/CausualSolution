#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#	include <wx/wx.h>
#endif

#include <wx/setup.h>

#include "application.h"
#include "main_frame.h"

bool Application::OnInit() 
{
	auto frame = new MainFrame;
	frame->Show(true);

	return true;
}

wxIMPLEMENT_APP(Application);