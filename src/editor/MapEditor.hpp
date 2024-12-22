/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "Editor.hpp"
#include "ExplorerPanel.hpp"
#include "Map.hpp"
#include "PlayProcess.hpp"
#include "PropertyPanel.hpp"
#include "ViewPanel.hpp"

#include <wx/aui/aui.h>
#include <wx/cmdproc.h>
#include <wx/filename.h>

#include <memory>

class MapEditor : public Editor
{
private:
	wxAuiManager m_AuiMgr;
	wxCommandProcessor m_Commands;

	ViewPanel* m_ViewPanel;
	ExplorerPanel* m_ExplorerPanel;
	PropertyPanel* m_PropertyPanel;

	wxFileName m_FileName;
	std::shared_ptr<Map> m_Map;
	PlayProcess* m_PlayMapProcess;

public:
	MapEditor(MainWindow* parent, wxMenu* editMenu, 
		BrowserWindow* browserWindow, const wxFileName& mapName);
	~MapEditor(void);

	ViewPanel* GetViewPanel(void) { return m_ViewPanel; }

	void PlayProcessTerminated(void);
	int GetFPS(void);

	void Load(const wxFileName& filePath);

	bool HasChanged(void);

	void OnUndo(void);
	void OnRedo(void);
	bool OnSave(bool allFiles = false);
	bool OnSaveAs(void);
	void OnCut(void);
	void OnCopy(void);
	void OnPaste(void);
	void OnDelete(void);

	void OnToolAction(wxCommandEvent& event);

private:
	void OnIdle(wxIdleEvent& event);

	void OnToolsRecomputeLighting(wxCommandEvent& event);
	void OnToolsPlayMap(wxCommandEvent& event);
};
