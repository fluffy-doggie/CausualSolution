#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#	include <wx/wx.h>
#endif

#include <wx/setup.h>

#include "main_frame.h"

enum {
	ID_SAVE,
	ID_LOAD,
	ID_GENERATE,
	ID_EXIT,

	ID_PREFERENCE,
	ID_FONT,
	ID_RENDER,
	ID_EXPORT,

	ID_HELP,
};

MainFrame::MainFrame()
	:wxFrame(NULL, wxID_ANY, "Hello World")
{
	// create and append a menu bar
	auto menuBar = new wxMenuBar;

	auto fileMenu	 = new wxMenu;
	auto moreMenu	 = new wxMenu;
	auto settingMenu = new wxMenu;
	
	fileMenu->Append(wxID_SAVE	, wxT("保存(&S)"), "保存当前设置");
	fileMenu->Append(ID_LOAD	, wxT("加载(&L)"), "加载本地设置");
	fileMenu->Append(ID_GENERATE, wxT("生成(&G)"), "生成字体");
	fileMenu->Append(wxID_EXIT	, wxT("退出(&E)"), "退出应用");

	settingMenu->Append(wxID_PREFERENCES, wxT("偏好(&P)"), "偏好设置");
	settingMenu->Append(ID_FONT			, wxT("字体(&F)"), "字体设置");
	settingMenu->Append(ID_RENDER		, wxT("渲染(&R)"), "渲染设置");
	settingMenu->Append(ID_EXPORT		, wxT("导出(&E)"), "导出设置");

	
	moreMenu->Append(wxID_HELP, wxT("帮助(&H)"), "帮助");

	menuBar->Append(fileMenu	, wxT("文件(&F)"));
	menuBar->Append(settingMenu , wxT("设置(&S)"));
	menuBar->Append(moreMenu	, wxT("更多(&M)"));
	SetMenuBar(menuBar);

	auto toolbar = CreateToolBar();
	//toolbar->AddTool(wxID_EXIT, )
	//toolbar->AddTool()
	// create a status bar
	CreateStatusBar(1);
	SetStatusText(wxT("XFont v0.1.0 2018.11.27"));
}

// PushEventHandler
// PopEventHandler
// Connect
// Disconnect
// wxID_ANY wxID_LOWEST(4999) wxID_HIGHEST(5999) ...