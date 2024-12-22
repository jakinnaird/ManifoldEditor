/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/filename.h>
#include <wx/menu.h>
#include <wx/panel.h>

class MainWindow;
class BrowserWindow;

class Editor : public wxPanel
{
public:
	enum EDITOR_TYPE
	{
		MAP_EDITOR,
		PROJECT_EDITOR
	};

protected:
	EDITOR_TYPE m_Type;
	wxString m_Title;
	wxMenu* m_EditMenu;

	BrowserWindow* m_Browser;

public:
	Editor(MainWindow* parent, wxMenu* editMenu, EDITOR_TYPE type,
		BrowserWindow* browserWindow);
	virtual ~Editor(void);

	EDITOR_TYPE GetType(void) { return m_Type; }
	const wxString& GetTitle(void) { return m_Title; }

	virtual void Load(const wxFileName& filePath) = 0;

	virtual void OnToolAction(wxCommandEvent& event) = 0;

	virtual bool HasChanged(void) = 0;

	virtual void OnUndo(void) = 0;
	virtual void OnRedo(void) = 0;
	virtual bool OnSave(bool allFiles) = 0;
	virtual bool OnSaveAs(void) = 0;
	virtual void OnCut(void) = 0;
	virtual void OnCopy(void) = 0;
	virtual void OnPaste(void) = 0;
	virtual void OnDelete(void) = 0;
};
