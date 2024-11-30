/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "wx/filename.h"
#include "wx/panel.h"
#include "wx/treectrl.h"

class ProjectEditor;

class ProjectExplorer : public wxPanel
{
private:
	ProjectEditor* m_Editor;
	wxTreeCtrl* m_Explorer;
	wxTreeItemId m_Root;

	bool m_Changed;

public:
	ProjectExplorer(ProjectEditor* parent);
	~ProjectExplorer(void);

	void Save(const wxFileName& fileName);
	void Load(const wxFileName& fileName);

	void Clear(void);

	bool HasChanged(void);
	bool HasFilename(void);
	const wxFileName& GetFilename(void);

private:
	void OnNewMpkPackage(const wxFileName& fileName);
	void OnNewZipPackage(const wxFileName& fileName);

	void BuildPackage(const wxTreeItemId& package);
	void CleanPackage(const wxTreeItemId& package);

private:
	void OnItemRightClick(wxTreeEvent& event);
	void OnItemActivated(wxTreeEvent& event);

	void OnMenuNewPackage(wxCommandEvent& event);
	void OnMenuAddNewItem(wxCommandEvent& event);
	void OnMenuAddExistingItem(wxCommandEvent& event);
	void OnMenuAddFilter(wxCommandEvent& event);
	void OnMenuOpenFile(wxCommandEvent& event);
	void OnMenuRemove(wxCommandEvent& event);

public:
	void OnMenuBuildProject(wxCommandEvent& event);
	void OnMenuCleanProject(wxCommandEvent& event);
	void OnMenuBuildPackage(wxCommandEvent& event);
	void OnMenuCleanPackage(wxCommandEvent& event);
};
