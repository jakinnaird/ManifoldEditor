/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/bitmap.h>
#include <wx/filesys.h>
#include <wx/fs_filter.h>
#include <wx/wfstream.h>

#include "irrlicht.h"

class FolderFSHandler : public wxFileSystemHandler
{
private:
	wxPathList m_Folders;

public:
	FolderFSHandler(void);
	~FolderFSHandler(void);

	void MountFolder(const wxString& path);

	bool CanOpen(const wxString& location);
	wxFSFile* OpenFile(wxFileSystem& fs, const wxString& location);
};

wxImage ImageFromFS(wxFileSystem& fileSystem, const wxString& location, wxBitmapType type = wxBITMAP_TYPE_ANY);
wxBitmap BitmapFromFS(wxFileSystem& fileSystem, const wxString& location, wxBitmapType type = wxBITMAP_TYPE_ANY);

class IrrFSHandler : public irr::io::IFileArchive
{
public:
	class IrrReadFile : public irr::io::IReadFile
	{
	private:
		irr::io::path m_Filename;
		wxInputStream* m_Stream;

	public:
		IrrReadFile(const irr::io::path& filename, wxInputStream* stream)
			: m_Filename(filename), m_Stream(stream) {}
		~IrrReadFile(void) { if (m_Stream) delete m_Stream; }

		const irr::io::path& getFileName(void) const { return m_Filename; }
		long getPos(void) const { return m_Stream->TellI(); }
		long getSize(void) const { return m_Stream->GetSize(); }
		irr::s32 read(void* buffer, irr::u32 sizeToRead)
		{
			m_Stream->Read(buffer, sizeToRead);
			return m_Stream->LastRead();
		}

		bool seek(long finalPos, bool relativeMovement = false)
		{
			wxSeekMode mode = relativeMovement ? wxFromCurrent : wxFromStart;
			return (m_Stream->SeekI(finalPos, mode) != wxInvalidOffset);
		}
	};

public:
	IrrFSHandler(void) {}
	~IrrFSHandler(void) {}

	irr::io::IReadFile* createAndOpenFile(const irr::io::path& filename);

	irr::io::IReadFile* createAndOpenFile(irr::u32 index)
	{
		return nullptr; // not implemented
	}

	const irr::io::IFileList* getFileList(void) const
	{
		return nullptr; // not implemented
	}

	irr::io::E_FILE_ARCHIVE_TYPE getType(void) const
	{
		return irr::io::EFAT_FOLDER;
	}
};

