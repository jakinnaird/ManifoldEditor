/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Common.hpp"
#include "ProjectEditor.hpp"
#include "ProjectExplorer.hpp"
#include "Serialize.hpp"

#include <wx/filedlg.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/mimetype.h>
#include <wx/sizer.h>
#include <wx/stdpaths.h>
#include <wx/textdlg.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>
#include <wx/zipstrm.h>

#define XML_PROJECT_NAME	"ManifoldProject"
#define XML_PACKAGE_NAME	"Package"
#define XML_FILTER_NAME		"Filter"
#define XML_FILE_NAME		"File"
#define XML_MAP_NAME		"Map"

class TreeItemData : public wxTreeItemData
{
public:
	enum NODE_TYPE
	{
		NODE_PROJECT,
		NODE_PACKAGE,
		NODE_MAP,
		NODE_FILTER,
		NODE_FILE
	} m_Type;

	wxFileName m_FileName;
	wxString m_Filter;

public:
	TreeItemData(NODE_TYPE type) : m_Type(type) {}
	~TreeItemData(void)	{}
};

ProjectExplorer::ProjectExplorer(ProjectEditor* parent)
	: wxPanel((wxWindow*)parent),
	  m_Editor(parent)
{
	m_Explorer = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxTR_HAS_BUTTONS | wxTR_MULTIPLE);
	TreeItemData* data = new TreeItemData(TreeItemData::NODE_PROJECT);
	m_Root = m_Explorer->AddRoot(_("untitled"), -1, -1, data);
	m_Explorer->Expand(m_Root);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_Explorer, wxSizerFlags(1).Expand());
	this->SetSizerAndFit(sizer);

	m_Changed = false;

	Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &ProjectExplorer::OnItemRightClick, this);
	Bind(wxEVT_TREE_ITEM_ACTIVATED, &ProjectExplorer::OnItemActivated, this);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuNewPackage, this, MENU_NEWPACKAGE);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuNewMap, this, MENU_NEWMAP);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuAddNewItem, this, MENU_ADDNEWFILE);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuAddExistingItem, this, MENU_ADDEXISTINGFILE);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuAddFilter, this, MENU_ADDFILTER);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuBuildProject, this, MENU_BUILDPROJECT);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuCleanProject, this, MENU_CLEANPROJECT);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuBuildPackage, this, MENU_BUILDPACKAGE);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuCleanPackage, this, MENU_CLEANPACKAGE);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuOpenFile, this, MENU_OPENFILE);
	Bind(wxEVT_MENU, &ProjectExplorer::OnMenuRemove, this, MENU_REMOVE);
}

ProjectExplorer::~ProjectExplorer(void)
{
}

void ProjectExplorer::Save(const wxFileName& fileName)
{
	// pick the right output file name
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(m_Root));
	wxFileName outFileName(fileName);
	if (!outFileName.IsOk())
		outFileName = data->m_FileName;

	wxTempFileOutputStream tempFile(outFileName.GetFullPath());
	if (!tempFile.IsOk())
		return;

	wxXmlDocument doc;
	wxXmlNode* root = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, XML_PROJECT_NAME);
	doc.SetRoot(root);

	// walk the tree
	wxTreeItemIdValue rootCookie;
	wxTreeItemId package = m_Explorer->GetFirstChild(m_Root, rootCookie);
	while (package.IsOk())
	{
		TreeItemData* pkgData = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(package));
		if (pkgData->m_FileName.IsAbsolute())
			pkgData->m_FileName.MakeRelativeTo(outFileName.GetPath());

		wxXmlNode* pkgNode = new wxXmlNode(root, wxXML_ELEMENT_NODE, XML_PACKAGE_NAME);
		pkgNode->AddAttribute("Path", pkgData->m_FileName.GetFullPath());

		wxTreeItemIdValue filterCookie;
		wxTreeItemId filter = m_Explorer->GetFirstChild(package, filterCookie);
		while (filter.IsOk())
		{
			TreeItemData* filterData = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(filter));

			wxXmlNode* filterNode = new wxXmlNode(pkgNode, wxXML_ELEMENT_NODE, XML_FILTER_NAME);
			filterNode->AddAttribute("Name", m_Explorer->GetItemText(filter));
			filterNode->AddAttribute("FileTypes", filterData->m_Filter);

			wxTreeItemIdValue fileCookie;
			wxTreeItemId file = m_Explorer->GetFirstChild(filter, fileCookie);
			while (file.IsOk())
			{
				TreeItemData* fileData = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(file));
				if (fileData->m_FileName.IsAbsolute())
					fileData->m_FileName.MakeRelativeTo(outFileName.GetPath());

				wxXmlNode* fileNode = new wxXmlNode(filterNode, wxXML_ELEMENT_NODE, XML_FILE_NAME);
				fileNode->AddAttribute("Path", fileData->m_FileName.GetFullPath());

				file = m_Explorer->GetNextChild(filter, fileCookie);
			}

			filter = m_Explorer->GetNextChild(package, filterCookie);
		}

		package = m_Explorer->GetNextChild(m_Root, rootCookie);
	}

	if (doc.IsOk())
		m_Changed = !(doc.Save(tempFile) && tempFile.Commit());

	data->m_FileName = outFileName;
	m_Explorer->SetItemText(m_Root, outFileName.GetFullName());
}

void ProjectExplorer::Load(const wxFileName& fileName)
{
	if (!fileName.IsOk())
		return; // new project

	wxXmlDocument doc;
	if (!doc.Load(fileName.GetFullPath()) ||
		doc.GetRoot()->GetName() != XML_PROJECT_NAME)
	{
		wxLogWarning(_("Failed to open project %s"), fileName.GetFullPath());
		return;
	}

	wxXmlNode* child = doc.GetRoot()->GetChildren();
	while (child)
	{
		if (child->GetName() == XML_PACKAGE_NAME)
		{
			wxXmlNode* packageNode = child;
			TreeItemData* data = new TreeItemData(TreeItemData::NODE_PACKAGE);
			data->m_FileName = packageNode->GetAttribute("Path");
			wxTreeItemId packageId = m_Explorer->AppendItem(m_Root, data->m_FileName.GetFullName(),
				-1, -1, data);

			wxXmlNode* packageChild = packageNode->GetChildren();
			while (packageChild)
			{
				if (packageChild->GetName() == XML_FILTER_NAME)
				{
					wxXmlNode* filterNode = packageChild;
					TreeItemData* data = new TreeItemData(TreeItemData::NODE_FILTER);
					data->m_Filter = filterNode->GetAttribute("FileTypes");
					wxTreeItemId filterId = m_Explorer->AppendItem(packageId, filterNode->GetAttribute("Name"),
						-1, -1, data);

					wxXmlNode* fileNode = filterNode->GetChildren();
					while (fileNode)
					{
						TreeItemData* data = new TreeItemData(TreeItemData::NODE_FILE);
						data->m_FileName = fileNode->GetAttribute("Path");
						m_Explorer->AppendItem(filterId, data->m_FileName.GetFullName(), -1, -1, data);

						fileNode = fileNode->GetNext();
					}

					m_Explorer->SortChildren(filterId);
				}

				m_Explorer->SortChildren(packageId);
				packageChild = packageChild->GetNext();
	 		}
		}

		child = child->GetNext();
	}

	m_Explorer->SetItemText(m_Root, fileName.GetFullName());
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(m_Root));
	data->m_FileName = fileName;
	m_Explorer->SortChildren(m_Root);
	m_Explorer->Expand(m_Root);
}

void ProjectExplorer::Clear(void)
{
	m_Explorer->DeleteChildren(m_Root);
	m_Explorer->SetItemText(m_Root, _("untitled"));
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(m_Root));
	data->m_FileName.Clear();
}

bool ProjectExplorer::HasChanged(void)
{
	return m_Changed;
}

bool ProjectExplorer::HasFilename(void)
{
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(m_Root));
	if (data->m_FileName.IsOk())
		return true;

	return false;
}

const wxFileName& ProjectExplorer::GetFilename(void)
{
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(m_Root));
	return data->m_FileName;
}

void ProjectExplorer::OnNewMpkPackage(const wxFileName& fileName)
{
	TreeItemData* data = new TreeItemData(TreeItemData::NODE_PACKAGE);
	data->m_FileName = fileName;
	wxTreeItemId packageId = m_Explorer->AppendItem(
		m_Root, fileName.GetFullName(), -1, -1, data);

	m_Explorer->EnsureVisible(packageId);

	// create the base filters
	data = new TreeItemData(TreeItemData::NODE_FILTER);
	data->m_Filter.assign(wxT("TrueType Fonts|*.ttf"));
	wxTreeItemId item = m_Explorer->AppendItem(
		packageId, _("fonts"), -1, -1, data);

	data = new TreeItemData(TreeItemData::NODE_FILTER);
	data->m_Filter.assign(wxT("Model Files|*.md2;*.obj"));
	item = m_Explorer->AppendItem(
		packageId, _("models"), -1, -1, data);

	data = new TreeItemData(TreeItemData::NODE_FILTER);
	data->m_Filter.assign(wxT("JavaScript Files|*.js|XML Files|*.xml"));
	item = m_Explorer->AppendItem(
		packageId, _("scripts"), -1, -1, data);

	data = new TreeItemData(TreeItemData::NODE_FILTER);
	data->m_Filter.assign(wxT("Image Files|*.jpg;*.png;*.bmp;*.tiff;*.gif"));
	item = m_Explorer->AppendItem(
		packageId, _("textures"), -1, -1, data);

	m_Explorer->SortChildren(m_Root);
}

void ProjectExplorer::OnNewZipPackage(const wxFileName& fileName)
{
	TreeItemData* data = new TreeItemData(TreeItemData::NODE_PACKAGE);
	data->m_FileName = fileName;
	wxTreeItemId packageId = m_Explorer->AppendItem(
		m_Root, fileName.GetFullName(), -1, -1, data);

	m_Explorer->EnsureVisible(packageId);

	// create the base filters
	data = new TreeItemData(TreeItemData::NODE_FILTER);
	data->m_Filter.assign(wxT("All Files|*.*"));
	wxTreeItemId item = m_Explorer->AppendItem(
		packageId, _("content"), -1, -1, data);
}

void ProjectExplorer::BuildPackage(const wxTreeItemId& package)
{
	// move the working directory to the project file location
	TreeItemData* rootData = dynamic_cast<TreeItemData*>(
		m_Explorer->GetItemData(m_Root));

	wxString oldWorkingPath = wxGetCwd();
	wxSetWorkingDirectory(rootData->m_FileName.GetPath());

	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(package));
	wxFileName packageName(data->m_FileName);

	wxLogMessage(_("Building package %s"), packageName.GetFullPath());

	wxTempFileOutputStream tempFile(packageName.GetFullPath());
	if (!tempFile.IsOk())
		return;

	wxZipOutputStream outStream(tempFile);
	if (!outStream.IsOk())
		return;

	wxTreeItemIdValue filterCookie;
	wxTreeItemId filter = m_Explorer->GetFirstChild(package, filterCookie);
	while (filter.IsOk())
	{
		wxString filterName(m_Explorer->GetItemText(filter));

		wxTreeItemIdValue fileCookie;
		wxTreeItemId file = m_Explorer->GetFirstChild(filter, fileCookie);
		while (file.IsOk())
		{
			TreeItemData* fileData = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(file));
			wxString destPath(fileData->m_FileName.GetFullName());
			if (packageName.GetExt().CompareTo(wxT("zip"), wxString::ignoreCase) != 0)
			{
				// zip files don't put files in the content/ subfolder, so rebuild
				destPath = filterName;
				destPath.append(wxFileName::GetPathSeparator());
				destPath.append(fileData->m_FileName.GetFullName());
			}
			
			wxLogMessage(_("Adding %s -> %s"), fileData->m_FileName.GetFullPath(),
				destPath);

			wxFileInputStream srcFile(fileData->m_FileName.GetFullPath());
			if (srcFile.IsOk())
			{
				if (outStream.PutNextEntry(destPath))
					outStream.Write(srcFile);
			}

			file = m_Explorer->GetNextChild(filter, fileCookie);
		}

		filter = m_Explorer->GetNextChild(package, filterCookie);
	}

	outStream.Close();
	tempFile.Commit();

	wxSetWorkingDirectory(oldWorkingPath);
	wxLogMessage(_("Finished building package %s"), packageName.GetFullPath());
}

void ProjectExplorer::CleanPackage(const wxTreeItemId& package)
{
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(package));
	wxRemoveFile(data->m_FileName.GetFullPath());
}

void ProjectExplorer::OnItemRightClick(wxTreeEvent& event)
{
	wxMenu popupMenu;

	// build the popup
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(event.GetItem()));
	if (event.GetItem() == m_Root)
	{
		popupMenu.Append(MENU_BUILDPROJECT, _("Build project"));
		popupMenu.Append(MENU_CLEANPROJECT, _("Clean project"));

		popupMenu.AppendSeparator();
		popupMenu.Append(MENU_NEWPACKAGE, _("New package"));
		popupMenu.Append(MENU_NEWMAP, _("New map"));
	}
	else if (data->m_Type == TreeItemData::NODE_PACKAGE)
	{
		wxMenu* buildMenu = new wxMenu;
		buildMenu->Append(MENU_BUILDPACKAGE, wxString::Format(_("Build only %s"), 
			data->m_FileName.GetFullName()));
		buildMenu->Append(MENU_CLEANPACKAGE, wxString::Format(_("Clean only %s"), 
			data->m_FileName.GetFullName()));
		popupMenu.AppendSubMenu(buildMenu, _("Package Only"));

		wxMenu* addMenu = new wxMenu;
		addMenu->Append(MENU_ADDFILTER, _("New Filter"));

		popupMenu.AppendSeparator();
		popupMenu.AppendSubMenu(addMenu, _("Add"));
	}
	else if (data->m_Type == TreeItemData::NODE_FILTER)
	{
		wxMenu* addMenu = new wxMenu;
		addMenu->Append(MENU_ADDNEWFILE, _("New Item"));
		addMenu->Append(MENU_ADDEXISTINGFILE, _("Existing Item"));
		popupMenu.AppendSubMenu(addMenu, _("Add"));
	}
	else if (data->m_Type == TreeItemData::NODE_FILE ||
		data->m_Type == TreeItemData::NODE_MAP)
	{
		popupMenu.Append(MENU_OPENFILE, _("Open"));
	}

	if (event.GetItem() != m_Root)
	{
		popupMenu.AppendSeparator();
		popupMenu.Append(MENU_REMOVE, _("Remove"));
		//popupMenu.Append(MENU_RENAME, _("Rename"));
	}

	popupMenu.AppendSeparator();
	popupMenu.Append(MENU_PROPERTIES, _("Properties"));

	PopupMenu(&popupMenu);
}

void ProjectExplorer::OnItemActivated(wxTreeEvent& event)
{
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(event.GetItem()));
	if (data->m_Type == TreeItemData::NODE_FILE)
	{
		// open the file
		m_Editor->OpenFile(data->m_FileName);
	}
	else if (data->m_Type == TreeItemData::NODE_MAP)
	{
		// launch a new editor instance
		wxString cmd = wxStandardPaths::Get().GetExecutablePath();
		cmd.append(wxT(" \""));
		cmd.append(data->m_FileName.GetFullPath()); // ensure we create a new map editor
		cmd.append(wxT("\""));
		wxExecute(cmd, wxEXEC_ASYNC);
	}
	else
		event.Skip(); // allow expand/collapse functions
}

void ProjectExplorer::OnMenuNewPackage(wxCommandEvent& event)
{
	wxFileDialog newPackage(GetParent(),
		_("Add new package"), wxEmptyString,
		_("untitled.mpk"), 
		_("Manifold Archive Package (*.mpk)|*.mpk|Zip Archive (*.zip)|*.zip"),
		wxFD_SAVE);

	if (newPackage.ShowModal() == wxID_CANCEL)
		return;

	wxFileName fileName(newPackage.GetPath());
	if (fileName.GetExt().CompareTo(wxT("mpk"), wxString::ignoreCase) == 0)
		OnNewMpkPackage(fileName);
	else if (fileName.GetExt().CompareTo(wxT("zip"), wxString::ignoreCase) == 0)
		OnNewZipPackage(fileName);

	m_Changed = true;
}

void ProjectExplorer::OnMenuNewMap(wxCommandEvent& event)
{
	wxFileDialog newMap(GetParent(),
		_("Add new map"), wxEmptyString,
		wxEmptyString,
		ISerializerFactory::BuildFilter(),
		wxFD_SAVE);

	if (newMap.ShowModal() == wxID_CANCEL)
		return;

	wxFileName fileName(newMap.GetPath());
	TreeItemData* data = new TreeItemData(TreeItemData::NODE_MAP);
	data->m_FileName = fileName;
	wxTreeItemId mapId = m_Explorer->AppendItem(
		m_Root, fileName.GetFullName(), -1, -1, data);
	m_Explorer->EnsureVisible(mapId);

	m_Changed = true;
}

void ProjectExplorer::OnMenuAddNewItem(wxCommandEvent& event)
{
	wxTreeItemId filterItem = m_Explorer->GetFocusedItem();
	if (!filterItem.IsOk())
		return; // how did we even get here?

	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(filterItem));

	wxFileDialog newItem(GetParent(),
		_("Add new item"), wxEmptyString, wxEmptyString,
		data->m_Filter,	wxFD_SAVE);

	if (newItem.ShowModal() == wxID_CANCEL)
		return;

	TreeItemData* newData = new TreeItemData(TreeItemData::NODE_FILE);
	newData->m_FileName = newItem.GetPath();
	wxTreeItemId fileId = m_Explorer->AppendItem(filterItem, newData->m_FileName.GetFullName(),
		-1, -1, newData);
	m_Explorer->SortChildren(filterItem);
	m_Explorer->EnsureVisible(fileId);

	m_Changed = true;
}

void ProjectExplorer::OnMenuAddExistingItem(wxCommandEvent& event)
{
	wxTreeItemId filterItem = m_Explorer->GetFocusedItem();
	if (!filterItem.IsOk())
		return; // how did we even get here?

	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(filterItem));

	wxFileDialog newItem(GetParent(),
		_("Add existing item"), wxEmptyString, wxEmptyString,
		data->m_Filter, wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (newItem.ShowModal() == wxID_CANCEL)
		return;

	TreeItemData* newData = new TreeItemData(TreeItemData::NODE_FILE);
	newData->m_FileName = newItem.GetPath();
	wxTreeItemId fileId = m_Explorer->AppendItem(filterItem, newData->m_FileName.GetFullName(),
		-1, -1, newData);
	m_Explorer->SortChildren(filterItem);
	m_Explorer->EnsureVisible(fileId);

	m_Changed = true;
}

void ProjectExplorer::OnMenuAddFilter(wxCommandEvent& event)
{
	wxTreeItemId item = m_Explorer->GetFocusedItem();
	if (!item.IsOk())
		return; // how did we even get here?

	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(item));
	if (data->m_Type != TreeItemData::NODE_PACKAGE &&
		data->m_Type != TreeItemData::NODE_FILTER)
		return;

	wxTextEntryDialog dialog(this, _("New filter"),
		_("New filter"));
	if (dialog.ShowModal() == wxID_OK)
	{
		TreeItemData* filterData = new TreeItemData(TreeItemData::NODE_FILTER);
		filterData->m_Filter.assign(wxFileSelectorDefaultWildcardStr);
		wxTreeItemId filterItem = m_Explorer->AppendItem(item, dialog.GetValue(), -1, -1, 
			filterData);
		m_Explorer->SortChildren(item);
		m_Explorer->EnsureVisible(filterItem);

		m_Changed = true;
	}
}

void ProjectExplorer::OnMenuOpenFile(wxCommandEvent& event)
{
	wxTreeItemId item = m_Explorer->GetFocusedItem();
	if (!item.IsOk())
		return; // how did we even get here?

	// open the file
	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(item));
	if (data->m_Type == TreeItemData::NODE_FILE)
		m_Editor->OpenFile(data->m_FileName);
}

void ProjectExplorer::OnMenuRemove(wxCommandEvent& event)
{
	wxTreeItemId item = m_Explorer->GetFocusedItem();
	if (!item.IsOk())
		return; // how did we even get here?

	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(item));
	if (data->m_Type == TreeItemData::NODE_PROJECT)
		return;

	if (data->m_Type == TreeItemData::NODE_FILTER ||
		data->m_Type == TreeItemData::NODE_PACKAGE)
	{
		wxMessageDialog check(this,
			wxString::Format(_("Are you sure you want to remove %s?"),
				m_Explorer->GetItemText(item)),
			_("Confirm removal"), wxYES_NO | wxCANCEL);
		if (check.ShowModal() != wxID_YES)
			return;
	}

	m_Explorer->Delete(item);
	m_Changed = true;
}

void ProjectExplorer::OnMenuBuildProject(wxCommandEvent& event)
{
	wxLogMessage(_("Building project"));

	wxTreeItemIdValue packageCookie;
	wxTreeItemId package = m_Explorer->GetFirstChild(m_Root, packageCookie);
	while (package.IsOk())
	{
		TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(package));
		if (data->m_Type == TreeItemData::NODE_PACKAGE)
			BuildPackage(package);

		package = m_Explorer->GetNextChild(m_Root, packageCookie);
	}

	wxLogMessage(_("Build completed"));
}

void ProjectExplorer::OnMenuCleanProject(wxCommandEvent& event)
{
	wxLogMessage(_("Cleaning project"));

	wxTreeItemIdValue packageCookie;
	wxTreeItemId package = m_Explorer->GetFirstChild(m_Root, packageCookie);
	while (package.IsOk())
	{
		TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(package));
		if (data->m_Type == TreeItemData::NODE_PACKAGE)
			CleanPackage(package);

		package = m_Explorer->GetNextChild(m_Root, packageCookie);
	}

	wxLogMessage(_("Clean completed"));
}

void ProjectExplorer::OnMenuBuildPackage(wxCommandEvent& event)
{
	wxTreeItemId package = m_Explorer->GetFocusedItem();
	if (!package.IsOk())
		return;

	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(package));
	if (data->m_Type != TreeItemData::NODE_PACKAGE)
		return;

	BuildPackage(package);
}

void ProjectExplorer::OnMenuCleanPackage(wxCommandEvent& event)
{
	wxTreeItemId package = m_Explorer->GetFocusedItem();
	if (!package.IsOk())
		return;

	TreeItemData* data = dynamic_cast<TreeItemData*>(m_Explorer->GetItemData(package));
	if (data->m_Type != TreeItemData::NODE_PACKAGE)
		return;

	CleanPackage(package);
}
