/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "BrowserWindow.hpp"
#include "Editor.hpp"
#include "MapEditor.hpp"
#include "PackageManager.hpp"

#include <wx/aui/aui.h>
#include <wx/cmdproc.h>
#include <wx/frame.h>

#include <memory>

class MainWindow : public wxFrame
{
private:
	wxAuiManager m_AuiMgr;
	wxMenu* m_EditMenu;

	BrowserWindow* m_Browser;
	PackageManager* m_PackageManager;

	Editor* m_ActiveEditor;

	int m_LastFPS;

public:
	MainWindow(void);
	~MainWindow(void);

	wxAuiManager& GetAuiMgr(void) { return m_AuiMgr; }

	void LoadFile(const wxString& filePath);

	void SetCaption(const wxString& fileName);
	void UpdateFrameTime(int fps);

	void OnToolAction(wxCommandEvent& event);

private:
	// event handlers
	void OnClose(wxCloseEvent& event);

	void OnConfigChanged(wxCommandEvent& event);

	void OnFileNewMap(wxCommandEvent& event);
	void OnFileNewProject(wxCommandEvent& event);
	void OnFileOpenMap(wxCommandEvent& event);
	void OnFileOpenProject(wxCommandEvent& event);
	void OnFileOpen(wxCommandEvent& event);
	void OnFileSave(wxCommandEvent& event);
	void OnFileSaveAs(wxCommandEvent& event);
	void OnFileClose(wxCommandEvent& event);
	void OnFilePreferences(wxCommandEvent& event);
	void OnFileExit(wxCommandEvent& event);
	void OnEditUndo(wxCommandEvent& event);
	void OnEditRedo(wxCommandEvent& event);
	void OnEditCut(wxCommandEvent& event);
	void OnEditCopy(wxCommandEvent& event);
	void OnEditPaste(wxCommandEvent& event);
	void OnEditDelete(wxCommandEvent& event);
	void OnHelpAbout(wxCommandEvent& event);

	void OnToolsEntityBrowser(wxCommandEvent& event);
	void OnToolsActorBrowser(wxCommandEvent& event);
	void OnToolsTextureBrowser(wxCommandEvent& event);
	void OnToolsSoundBrowser(wxCommandEvent& event);
	void OnToolsPackageManager(wxCommandEvent& event);
};
