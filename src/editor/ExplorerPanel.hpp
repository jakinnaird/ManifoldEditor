/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "BrowserWindow.hpp"
#include "ViewPanel.hpp"

#include "wx/cmdproc.h"
#include "wx/panel.h"
#include "wx/treectrl.h"

#include "irrlicht.h"

class ExplorerPanel : public wxPanel
{
private:
	wxCommandProcessor& m_Commands;
	BrowserWindow* m_Browser;
	ViewPanel* m_ViewPanel;

	wxTreeCtrl* m_Explorer;
	wxTreeItemId m_Root;
	wxTreeItemId m_GeometryRoot;
	wxTreeItemId m_ActorRoot;

	irr::scene::ISceneManager* m_SceneMgr;

	bool m_Changing;

public:
	ExplorerPanel(wxWindow* parent, wxCommandProcessor& cmdProc,
		BrowserWindow* browser);
	~ExplorerPanel(void);

	void SetViewPanel(ViewPanel* viewPanel);

	void SetSceneManager(irr::scene::ISceneManager* sceneMgr);
	void SetMapName(const wxString& name);

	void Clear(void);

	void SelectItem(const wxString& name);
	void UnselectItem(const wxString& name);
	void UnselectAll(void);

	void AddGeometry(const wxString& name);
	void RemoveGeometry(const wxString& name);
	bool IsGeometry(const wxString& name);

	void AddActor(const wxString& name);
	void RemoveActor(const wxString& name);
	bool IsActor(const wxString& name);

private:
	wxTreeItemId FindItem(const wxString& name, wxTreeItemId& start);

	void OnSelectionChanged(wxTreeEvent& event);
	void OnItemRightClick(wxTreeEvent& event);
};
