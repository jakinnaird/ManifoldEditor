/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "FSHandler.hpp"

#include <wx/filename.h>
#include <wx/log.h>

FolderFSHandler::FolderFSHandler(void)
{
}

FolderFSHandler::~FolderFSHandler(void)
{
}

void FolderFSHandler::MountFolder(const wxString& path)
{
	m_Folders.Add(path);
}

bool FolderFSHandler::CanOpen(const wxString& location)
{
	wxString path = m_Folders.FindValidPath(location);
	return !path.empty();
}

wxFSFile* FolderFSHandler::OpenFile(wxFileSystem& fs, const wxString& location)
{
	wxFSFile* result = nullptr;

	wxString path = m_Folders.FindValidPath(location);
	wxFileInputStream* stream = new wxFileInputStream(path);
	if (!stream->IsOk())
		delete stream;
	else
		result = new wxFSFile(stream, path, wxEmptyString, wxEmptyString,
			wxDateTime::Now());

	return result;
}

wxImage ImageFromFS(wxFileSystem& fileSystem, const wxString& location, wxBitmapType type)
{
	wxFSFile* f = fileSystem.OpenFile(location);
	if (f)
	{
		wxImage img(*f->GetStream(), type);
		delete f;

		return img;
	}

	return wxImage();
}

wxBitmap BitmapFromFS(wxFileSystem& fileSystem, const wxString& location, wxBitmapType type)
{
	wxFSFile* f = fileSystem.OpenFile(location);
	if (f)
	{
		wxImage img(*f->GetStream(), type);
		delete f;

		return wxBitmap(img);
	}

	return wxBitmap();
}

irr::io::IReadFile* IrrFSHandler::createAndOpenFile(const irr::io::path& filename)
{
	wxFileSystem fileSystem;
	
	wxString filePath(filename.c_str());

	// check if the location is a zip file
	// e.g. demo.zip:models/sydney.md2 -> demo.zip#zip:models/sydney.md2
	if (filePath.Contains(wxT(".zip:")))
	{
		wxString zipFile = filePath.BeforeLast(wxT(':'));
		wxString fileName = filePath.AfterLast(wxT(':'));
		filePath = zipFile + wxT("#zip:") + fileName;
	}

	wxFSFile* f = fileSystem.OpenFile(filePath);
	if (f)
	{
		IrrReadFile* ret = new IrrReadFile(filename, f->DetachStream());
		delete f;
		return ret;
	}

	return nullptr;
}
