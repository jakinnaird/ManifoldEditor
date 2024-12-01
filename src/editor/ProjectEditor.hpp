/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "Editor.hpp"
#include "ProjectExplorer.hpp"

#include <wx/aui/aui.h>
#include <wx/filename.h>

#include <memory>

class ProjectEditor : public Editor
{
private:
	wxAuiManager m_AuiMgr;
	wxAuiNotebook* m_Pages;

	wxFileName m_FileName;

	ProjectExplorer* m_Explorer;

public:
	ProjectEditor(MainWindow* parent, wxMenu* editMenu, 
		BrowserWindow* browserWindow, const wxFileName& fileName);
	~ProjectEditor(void);

	// open a new editor window for the supplied file
	void OpenFile(const wxFileName& fileName);

	void Load(const wxFileName& filePath);
	bool HasChanged(void);

	void OnUndo(void);
	void OnRedo(void);
	bool OnSave(void);
	bool OnSaveAs(void);
	void OnCut(void);
	void OnCopy(void);
	void OnPaste(void);
	void OnDelete(void);

	void OnToolAction(wxCommandEvent& event);

private:
	void OnPageClose(wxAuiNotebookEvent& event);
	void OnBuildProject(wxCommandEvent& event);
};

class EditorPage : public wxPanel
{
protected:
	wxMenu* m_EditMenu;

public:
	EditorPage(wxWindow* parent, wxMenu* editMenu)
		: wxPanel(parent), m_EditMenu(editMenu) {}
	virtual ~EditorPage(void) {}

	virtual bool HasChanged(void) = 0;

	virtual void Save(void) = 0;

	virtual void OnUndo(void) = 0;
	virtual void OnRedo(void) = 0;

	virtual void OnCut(void) = 0;
	virtual void OnCopy(void) = 0;
	virtual void OnPaste(void) = 0;
};
