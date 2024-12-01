/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/cmdproc.h>
#include <wx/dialog.h>
#include <wx/filename.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>

#include <map>

class PackageManager : public wxDialog
{
private:
	wxCommandProcessor m_Commands;
	wxStaticText* m_FileText;
	wxListView* m_FileList;

	typedef std::map<long, wxString> itempath_t;
	itempath_t m_ItemPaths;

public:
	PackageManager(wxWindow* parent);
	~PackageManager(void);

	bool Save(const wxString& destPath, const wxString& srcPath = wxEmptyString);

private:
	void OnCloseEvent(wxCloseEvent& event);

	void OnToolNew(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
	void OnToolSave(wxCommandEvent& event);
	void OnToolSaveAs(wxCommandEvent& event);
	void OnToolUndo(wxCommandEvent& event);
	void OnToolRedo(wxCommandEvent& event);
	void OnToolAdd(wxCommandEvent& event);
	void OnToolRemove(wxCommandEvent& event);
	void OnToolExtract(wxCommandEvent& event);
};
