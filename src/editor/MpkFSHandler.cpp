/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "MpkFSHandler.hpp"

#include "wx/config.h"
#include "wx/fs_arc.h"
#include "wx/stdpaths.h"

MpkFSHandler::MpkFSHandler(void)
{
}

MpkFSHandler::~MpkFSHandler(void)
{
}

void MpkFSHandler::AddSearchPath(const wxString& path)
{
	m_SearchPaths.Add(path);
}

bool MpkFSHandler::CanOpen(const wxString& location)
{
	// providing the archive file can set the filename as the protocol
	// e.g. archive.mpk:/path/to/file
	wxFileName fn(location.substr(0,
		location.rfind(wxT(':'))));
	if (fn.GetExt().CompareTo(wxT("mpk"), wxString::ignoreCase) == 0)
		return true;

	if (fn.GetExt().CompareTo(wxT("mmp"), wxString::ignoreCase) == 0)
		return true;

	return false; // fall through
}

wxString MpkFSHandler::FindValidPath(const wxString& location)
{
	wxString path = location;

	wxFileName fn(path);

	// check if it's in the data directory
	wxFileName search(wxStandardPaths::Get().GetDataDir(), fn.GetFullName());
	if (search.FileExists())
	{
		path = search.GetFullPath();
	}
	else
	{
		// check if it's in the executable directory
		wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
		search.Assign(exePath.GetPath(), fn.GetFullName());
		if (search.FileExists())
		{
			path = search.GetFullPath();
		}
		else
		{
			bool found = false;

			// check all the configured paths
			wxConfigBase* config = wxConfigBase::Get();
			if (config) // we might not have a config file
			{
				wxString entry;
				long cookie;
				wxConfigPathChanger cpc(config, wxT("/Paths/"));
				if (config->GetFirstEntry(entry, cookie))
				{
					do
					{
						search.Assign(config->Read(entry), fn.GetFullName());
						if (search.FileExists())
						{
							path = search.GetFullPath();
							found = true;
							break;
						}
					} while (config->GetNextEntry(entry, cookie));
				}
			}

			// check the custom search paths
			if (!found)
			{
				wxString search = m_SearchPaths.FindValidPath(fn.GetFullName());
				if (!search.empty())
					path = search;
			}
		}
	}

	return path;
}

wxFSFile* MpkFSHandler::OpenFile(wxFileSystem& fs, const wxString& location)
{
	// providing the archive file can set the filename as the protocol
	// e.g. archive.mpk:/path/to/file
	if (location.empty())
		return nullptr;

	wxString loc = GetLeftLocation(location);
	if (loc.empty())
	{
		loc = GetProtocol(location);
		if (!loc.EndsWith(wxT(".mpk")) &&
			!loc.EndsWith(wxT(".mmp")))
		{
			size_t pos = location.rfind(wxT(':'));
			if (pos == wxString::npos)
				return nullptr;

			loc = location.substr(0, pos);
		}
	}

	// search for the package
	wxString path = FindValidPath(loc);
	if (path.empty())
		path.assign(loc); // just set it to the current dir, maybe it wasn't added as a search path

	wxString rightLoc = GetRightLocation(location);
	if (!rightLoc.StartsWith(wxT("#")))
		path += wxT("#zip:");

	// convert '\\' to '/'
	rightLoc.Replace(wxT("\\"), wxT("/"));
	path += rightLoc;

	wxArchiveFSHandler archiveFSHandler;
	return archiveFSHandler.OpenFile(fs, path);
}
