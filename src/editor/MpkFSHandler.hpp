/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/filefn.h>
#include <wx/filesys.h>
#include <wx/fs_filter.h>
#include <wx/wfstream.h>

// seamlessly handle .mpk files (which are just zip archives)
class MpkFSHandler : public wxFilterFSHandler
{
private:
	wxPathList m_SearchPaths;

public:
	MpkFSHandler(void);
	~MpkFSHandler(void);

	void AddSearchPath(const wxString& path);

	bool CanOpen(const wxString& location) wxOVERRIDE;
	wxFSFile* OpenFile(wxFileSystem& fs, const wxString& location) wxOVERRIDE;

private:
	wxString FindValidPath(const wxString& location);
};
