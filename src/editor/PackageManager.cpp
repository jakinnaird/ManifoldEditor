/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "PackageManager.hpp"

#include "wx/artprov.h"
#include "wx/busyinfo.h"
#include "wx/filedlg.h"
#include "wx/filepicker.h"
#include "wx/log.h"
#include "wx/mimetype.h"
#include "wx/msgdlg.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "wx/toolbar.h"
#include "wx/wfstream.h"
#include "wx/zipstrm.h"

enum
{
	COL_PATH = 0,
	COL_DATE,
	COL_TYPE,
	COL_SIZE
};

class AddFileCommand : public wxCommand
{
private:
	wxListView* m_ListView;
	std::map<long, wxString>& m_ItemPaths;

	wxString m_FilePath;	// the file name saved in the archive
	wxString m_FileSource;	// the full path to the source file being added

public:
	AddFileCommand(wxListView* listView, std::map<long, wxString>& itemPaths,
		const wxString& filePath, const wxString& fileSource)
		: m_ListView(listView), m_ItemPaths(itemPaths),
		m_FilePath(filePath), m_FileSource(fileSource)
	{
	}

	~AddFileCommand(void)
	{
	}

	bool CanUndo(void) const { return true; }
	
	bool Do(void)
	{
		wxFileInputStream stream(m_FileSource);
		if (stream.IsOk())
		{
			wxFileName fn(m_FileSource);

			long index = m_ListView->InsertItem(m_ListView->GetItemCount(), m_FilePath);

			long pathIndex = m_ItemPaths.size();
			m_ItemPaths.emplace(pathIndex, m_FileSource);
			m_ListView->SetItemData(index, pathIndex);

			m_ListView->SetItem(index, COL_DATE, wxDateTime::Now().FormatISOCombined(' '));

			size_t size = stream.GetSize();
			wxFileType* mimeType = wxTheMimeTypesManager->GetFileTypeFromExtension(fn.GetExt());
			wxString type;
			if (mimeType && mimeType->GetMimeType(&type))
			{
				m_ListView->SetItem(index, COL_TYPE, type);
				delete mimeType;
			}
			else
				m_ListView->SetItem(index, COL_TYPE, _("Unknown"));

			m_ListView->SetItem(index, COL_SIZE, wxString::Format(wxT("%" wxLongLongFmtSpec "u"), size));

			return true;
		}

		return false;
	}

	wxString GetName(void) const
	{
		return wxString::Format(_("Add file %s"), m_FilePath);
	}

	bool Undo(void)
	{
		long index = m_ListView->FindItem(-1, m_FilePath);
		if (index != wxNOT_FOUND)
			return m_ListView->DeleteItem(index);

		return false;
	}
};

class RemoveFileCommand : public wxCommand
{
private:
	wxListView* m_ListView;

	long m_Index;

	wxString m_FilePath;
	wxString m_FileType;
	wxString m_ModifiedDate;
	wxString m_Size;

public:
	RemoveFileCommand(wxListView* listView, long index, 
		const wxString& filePath, const wxString& fileType, 
		const wxString& modifiedDate, const wxString& size)
		: m_ListView(listView), m_Index(index), m_FilePath(filePath),
		m_FileType(fileType), m_ModifiedDate(modifiedDate),
		m_Size(size)
	{
	}

	~RemoveFileCommand(void)
	{
	}

	bool CanUndo(void) const { return true; }

	bool Do(void)
	{
		return m_ListView->DeleteItem(m_Index);
	}

	wxString GetName(void) const
	{
		return wxString::Format(_("Remove file %s"), m_FilePath);
	}

	bool Undo(void)
	{
		long index = m_ListView->InsertItem(m_Index, m_FilePath);
		m_ListView->SetItem(index, COL_DATE, m_ModifiedDate);
		m_ListView->SetItem(index, COL_TYPE, m_FileType);
		m_ListView->SetItem(index, COL_SIZE, m_Size);
		return true;
	}
};

class FilePicker : public wxDialog
{
private:
	wxFilePickerCtrl* m_Source;
	wxTextCtrl* m_Path;

public:
	FilePicker(wxWindow* parent)
		: wxDialog(parent, wxID_ANY, _("Select file"))
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->SetMinSize(320, 80);

		m_Source = new wxFilePickerCtrl(this, wxID_ANY, wxEmptyString,
			wxString::FromAscii(wxFileSelectorPromptStr),
			wxString::FromAscii(wxFileSelectorDefaultWildcardStr),
			wxDefaultPosition, wxDefaultSize, 
			wxFLP_OPEN | wxFLP_FILE_MUST_EXIST | wxFLP_USE_TEXTCTRL);
		sizer->Add(new wxStaticText(this, wxID_ANY, _("File to add")));
		sizer->Add(m_Source, wxSizerFlags().Expand().Border(wxALL));

		m_Path = new wxTextCtrl(this, wxID_ANY);
		sizer->Add(new wxStaticText(this, wxID_ANY, _("Path in package")));
		sizer->Add(m_Path, wxSizerFlags().Expand().Border(wxALL));

		wxSizer* buttons = CreateButtonSizer(wxOK | wxCANCEL);
		sizer->Add(buttons, wxSizerFlags().Expand().Border(wxALL));

		SetSizerAndFit(sizer);

		Bind(wxEVT_FILEPICKER_CHANGED, &FilePicker::OnFilePicked, this);
	}

	~FilePicker(void)
	{
	}

	wxString GetSource(void) const { return m_Source->GetPath(); }
	wxString GetPath(void) const { return m_Path->GetValue(); }

private:
	void OnFilePicked(wxFileDirPickerEvent& event)
	{
		// try to determine the MIME type and so the base path
		wxString path;

		wxFileName fileName(event.GetPath());
		wxFileType* mimeType = wxTheMimeTypesManager->GetFileTypeFromExtension(fileName.GetExt());
		wxString type;
		if (mimeType)
		{
			if (mimeType->GetMimeType(&type))
			{
				if (type.StartsWith(wxT("image/")))
					path.assign(wxT("textures"));
				else if (type.StartsWith(wxT("map/")))
					path.assign(wxT("maps"));
				else if (type.StartsWith(wxT("model/")))
					path.assign(wxT("models"));
				else if (type.StartsWith(wxT("text/")))
					path.assign(wxT("scripts"));
				else if (type.StartsWith(wxT("shader/")))
					path.assign(wxT("shaders"));
				else if (type.StartsWith(wxT("audio/")))
					path.assign(wxT("sounds"));
				else if (type.StartsWith(wxT("lang/")))
					path.assign(wxT("lang"));
			}

			delete mimeType;
		}

		if (path.empty())
			path.assign(wxT("etc")); // unknown

		if (!path.empty())
			path.append(wxFILE_SEP_PATH);

		path.append(fileName.GetFullName());
		m_Path->SetValue(path);
	}
};

PackageManager::PackageManager(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, _("Package Manager"), wxDefaultPosition,
		wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->SetMinSize(640, 480);

	wxToolBar* tools = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_HORIZONTAL);
	tools->AddTool(wxID_NEW, _("New"), wxArtProvider::GetBitmap(wxART_NEW),
		_("New package"));
	tools->AddTool(wxID_OPEN, _("Open"), wxArtProvider::GetBitmap(wxART_FILE_OPEN),
		_("Open package"));
	tools->AddTool(wxID_SAVE, _("Save"), wxArtProvider::GetBitmap(wxART_FILE_SAVE),
		_("Save package"));
	tools->AddTool(wxID_SAVEAS, _("Save As"), wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS),
		_("Save package as"));
	tools->AddSeparator();
	tools->AddTool(wxID_UNDO, _("Undo"), wxArtProvider::GetBitmap(wxART_UNDO),
		_("Undo"));
	tools->AddTool(wxID_REDO, _("Redo"), wxArtProvider::GetBitmap(wxART_REDO),
		_("Redo"));
	tools->AddSeparator();
	tools->AddTool(wxID_ADD, _("Add"), wxArtProvider::GetBitmap(wxART_PLUS),
		_("Add file to package"));
	tools->AddTool(wxID_REMOVE, _("Remove"), wxArtProvider::GetBitmap(wxART_MINUS),
		_("Remove file from package"));
	tools->AddTool(wxID_HARDDISK, _("Extract"), wxArtProvider::GetBitmap(wxART_HARDDISK),
		_("Extract file from package"));
	tools->Realize();

	m_FileText = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxDefaultSize, wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_MIDDLE);

	m_FileList = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_VRULES);
	m_FileList->AppendColumn(_("Path"));
	m_FileList->AppendColumn(_("Date modified"));
	m_FileList->AppendColumn(_("Type"));
	m_FileList->AppendColumn(_("Size"));

	sizer->Add(tools, wxSizerFlags(1).Expand());
	sizer->Add(m_FileText, wxSizerFlags().Expand());
	sizer->Add(m_FileList, wxSizerFlags(9).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_CLOSE_WINDOW, &PackageManager::OnCloseEvent, this);
	Bind(wxEVT_MENU, &PackageManager::OnToolNew, this, wxID_NEW);
	Bind(wxEVT_MENU, &PackageManager::OnToolOpen, this, wxID_OPEN);
	Bind(wxEVT_MENU, &PackageManager::OnToolSave, this, wxID_SAVE);
	Bind(wxEVT_MENU, &PackageManager::OnToolSaveAs, this, wxID_SAVEAS);
	Bind(wxEVT_MENU, &PackageManager::OnToolAdd, this, wxID_ADD);
	Bind(wxEVT_MENU, &PackageManager::OnToolRemove, this, wxID_REMOVE);
	Bind(wxEVT_MENU, &PackageManager::OnToolExtract, this, wxID_HARDDISK);
	Bind(wxEVT_MENU, &PackageManager::OnToolUndo, this, wxID_UNDO);
	Bind(wxEVT_MENU, &PackageManager::OnToolRedo, this, wxID_REDO);
}

PackageManager::~PackageManager(void)
{
}

bool PackageManager::Save(const wxString& destPath, const wxString& srcPath)
{
	wxBusyInfo wait(wxBusyInfoFlags()
		.Parent(this)
		.Title(_("Saving package"))
		.Text(_("Please wait..."))
		.Foreground(*wxBLACK)
		.Background(*wxWHITE));

	wxTempFileOutputStream dest(destPath);
	if (!dest.IsOk())
	{
		wxLogWarning(_("Failed to save to %s"), destPath);
		return false;
	}

	wxZipOutputStream outputStream(dest);

	wxString sourceFilename(srcPath);
	if (!sourceFilename.empty())
	{
		wxFileInputStream source(sourceFilename);
		if (source.IsOk())
		{
			wxZipInputStream inputStream(source);
			wxZipEntry* entry = inputStream.GetNextEntry();
			while (entry)
			{
				// make sure we didn't remove this entry from the list
				if (m_FileList->FindItem(-1, entry->GetName()) != wxNOT_FOUND)
					outputStream.CopyEntry(entry, inputStream);

				entry->UnRef();
				entry = inputStream.GetNextEntry();
			}
		}
	}

	if (m_ItemPaths.size() > 0) // we have additions
	{
		for (long index = 0; index < m_FileList->GetItemCount(); ++index)
		{
			long itemPath = m_FileList->GetItemData(index);
			itempath_t::iterator item = m_ItemPaths.find(itemPath);
			if (item != m_ItemPaths.end())
			{
				wxString name = m_FileList->GetItemText(index, COL_PATH);
				wxString path = item->second;

				wxFileInputStream stream(path);
				if (stream.IsOk())
				{
					if (outputStream.PutNextEntry(name))
						outputStream.Write(stream);
				}

				m_FileList->SetItemData(index, -1);
			}
		}

		m_ItemPaths.clear();
	}

	if (!outputStream.Close() || !dest.Commit())
		return false;

	return true;
}

void PackageManager::OnCloseEvent(wxCloseEvent& event)
{
	if (event.CanVeto())
	{
		this->Show(false); // hide ourselves
		event.Veto();
	}
}

void PackageManager::OnToolNew(wxCommandEvent& event)
{
	if (m_Commands.IsDirty())
	{
		wxMessageDialog check(this,
			_("Do you wish to save your changes?"),
			_("Unsaved changes"), wxYES_NO | wxCANCEL);
		int result = check.ShowModal();
		if (result == wxID_CANCEL)
			return; // go no further

		if (result == wxID_YES)
			OnToolSaveAs(event); // do the saving
	}

	// clear out the file list
	m_FileList->DeleteAllItems();
	m_ItemPaths.clear();

	// reset the file name
	m_FileText->SetLabel(wxEmptyString);
}

void PackageManager::OnToolOpen(wxCommandEvent& event)
{
	if (m_Commands.IsDirty())
	{
		wxMessageDialog check(this,
			_("Do you wish to save your changes?"),
			_("Unsaved changes"), wxYES_NO | wxCANCEL);
		int result = check.ShowModal();
		if (result == wxID_CANCEL)
			return; // go no further

		if (result == wxID_YES)
			OnToolSaveAs(event); // do the saving
	}

	wxFileDialog openDialog(this,
		_("Open package"), wxEmptyString, wxEmptyString,
		_("Manifold Engine (*.mpk)|*.mpk|Zip Archive (*.zip)|*.zip"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openDialog.ShowModal() == wxID_CANCEL)
		return; // not opening today

	wxBusyInfo wait(wxBusyInfoFlags()
		.Parent(this)
		.Title(_("Opening package"))
		.Text(_("Please wait..."))
		.Foreground(*wxBLACK)
		.Background(*wxWHITE));

	wxFileName fileName(openDialog.GetPath());

	wxFileInputStream inStream(fileName.GetFullPath());
	if (inStream.IsOk())
	{
		wxZipInputStream zipStream(inStream);
		if (!zipStream.IsOk())
		{
			wxLogWarning(_("Unsupported archive: %s"), fileName.GetFullPath());
			return;
		}

		// clear out the file list
		m_FileList->DeleteAllItems();

		wxZipEntry* entry = zipStream.GetNextEntry();
		while (entry)
		{
			wxFileName fn(entry->GetName());

			long index = m_FileList->InsertItem(m_FileList->GetItemCount(), entry->GetName());
			
			m_FileList->SetItemData(index, -1); // we don't want to try to add something

			m_FileList->SetItem(index, COL_DATE, entry->GetDateTime().FormatISOCombined(' '));

			wxFileType* mimeType = wxTheMimeTypesManager->GetFileTypeFromExtension(fn.GetExt());
			wxString type;
			if (mimeType)
			{
				if (mimeType->GetMimeType(&type))
					m_FileList->SetItem(index, COL_TYPE, type);
				else
					m_FileList->SetItem(index, COL_TYPE, _("Unknown"));

				delete mimeType;
			}
			else
				m_FileList->SetItem(index, COL_TYPE, _("Unknown"));

			m_FileList->SetItem(index, COL_SIZE, wxString::Format(wxT("%" wxLongLongFmtSpec "u"), entry->GetSize()));

			entry->UnRef();
			entry = zipStream.GetNextEntry();
		}

		// resize the columns to fit the contents
		m_FileList->SetColumnWidth(COL_PATH, wxLIST_AUTOSIZE);
		m_FileList->SetColumnWidth(COL_DATE, wxLIST_AUTOSIZE);
		m_FileList->SetColumnWidth(COL_TYPE, wxLIST_AUTOSIZE);
		m_FileList->SetColumnWidth(COL_SIZE, wxLIST_AUTOSIZE);

		m_FileText->SetLabel(fileName.GetFullPath());
	}
	else
		wxLogWarning(_("Failed to open archive %s"), fileName.GetFullPath());
}

void PackageManager::OnToolSave(wxCommandEvent& event)
{
	wxFileName fileName(m_FileText->GetLabelText());
	if (!fileName.IsOk())
	{
		OnToolSaveAs(event);
		return;
	}

	if (Save(fileName.GetFullPath(), m_FileText->GetLabelText()))
		m_Commands.MarkAsSaved();
}

void PackageManager::OnToolSaveAs(wxCommandEvent& event)
{
	wxFileDialog saveDialog(this,
		_("Save Package As..."), wxEmptyString, m_FileText->GetLabelText(),
		_("Manifold Engine (*.mpk)|*.mpk|Zip Archive (*.zip)|*.zip"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveDialog.ShowModal() == wxID_CANCEL)
		return; // not saving today

	wxBusyInfo wait(wxBusyInfoFlags()
		.Parent(this)
		.Title(_("Saving package"))
		.Text(_("Please wait..."))
		.Foreground(*wxBLACK)
		.Background(*wxWHITE));

	wxFileName fileName(saveDialog.GetPath());

	if (Save(fileName.GetFullPath(), m_FileText->GetLabelText()))
	{
		m_Commands.MarkAsSaved();
		m_FileText->SetLabel(fileName.GetFullPath());
	}
}

void PackageManager::OnToolUndo(wxCommandEvent& event)
{
	m_Commands.Undo();
}

void PackageManager::OnToolRedo(wxCommandEvent& event)
{
	m_Commands.Redo();
}

void PackageManager::OnToolAdd(wxCommandEvent& event)
{
	FilePicker dialog(this);
	if (dialog.ShowModal() == wxID_OK)
	{
		m_Commands.Submit(new AddFileCommand(m_FileList, m_ItemPaths,
			dialog.GetPath(), dialog.GetSource()));
	}
}

void PackageManager::OnToolRemove(wxCommandEvent& event)
{
	// check if something is selected
	long index = m_FileList->GetFirstSelected();
	if (index == wxNOT_FOUND)
		return;

	wxString name = m_FileList->GetItemText(index, COL_PATH);
	wxString date = m_FileList->GetItemText(index, COL_DATE);
	wxString type = m_FileList->GetItemText(index, COL_TYPE);
	wxString size = m_FileList->GetItemText(index, COL_SIZE);

	m_Commands.Submit(new RemoveFileCommand(m_FileList, index,
		name, type, date, size));
}

void PackageManager::OnToolExtract(wxCommandEvent& event)
{
	// check if something is selected
	long index = m_FileList->GetFirstSelected();
	if (index == wxNOT_FOUND)
		return;

	wxFileName itemPath = m_FileList->GetItemText(index, COL_PATH);
	wxFileDialog saveDialog(this,
		_("Extract item..."), wxEmptyString, itemPath.GetFullName(),
		wxFileSelectorDefaultWildcardStr,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveDialog.ShowModal() == wxID_CANCEL)
		return; // not saving today

	wxBusyInfo wait(wxBusyInfoFlags()
		.Parent(this)
		.Title(_("Extracting file"))
		.Text(_("Please wait..."))
		.Foreground(*wxBLACK)
		.Background(*wxWHITE));


	wxString packagePath(m_FileText->GetLabelText());
	wxFileInputStream inStream(packagePath);
	if (inStream.IsOk())
	{
		wxZipInputStream zipStream(inStream);
		if (!zipStream.IsOk())
		{
			wxLogWarning(_("Unable to open package: %s"), packagePath);
			return;
		}

		wxZipEntry* entry = zipStream.GetNextEntry();
		while (entry)
		{
			if (itemPath == entry->GetName())
			{
				wxFileOutputStream outFile(saveDialog.GetPath());
				if (outFile.IsOk())
					outFile.Write(zipStream);

				return;
			}

			entry->UnRef();
			entry = zipStream.GetNextEntry();
		}
	}
}
