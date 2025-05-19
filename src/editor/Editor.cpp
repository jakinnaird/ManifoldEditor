
#include "Editor.hpp"
#include "MainWindow.hpp"

Editor::Editor(MainWindow* parent, wxMenu* editMenu, EDITOR_TYPE type,
	BrowserWindow* browserWindow)
	: wxPanel(parent), m_EditMenu(editMenu), m_Type(type), m_Browser(browserWindow)
{
}

Editor::~Editor(void)
{
}
