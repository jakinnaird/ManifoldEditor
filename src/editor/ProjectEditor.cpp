/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Common.hpp"
#include "FSHandler.hpp"
#include "MainWindow.hpp"
#include "ProjectEditor.hpp"
#include "ScriptEditor.hpp"

#include <wx/accel.h>
#include <wx/busyinfo.h>
#include <wx/confbase.h>
#include <wx/filedlg.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

class IrrEventReceiver : public irr::IEventReceiver
{
public:
	bool OnEvent(const irr::SEvent& event)
	{
		switch (event.EventType)
		{
		case irr::EET_LOG_TEXT_EVENT:
		{
			switch (event.LogEvent.Level)
			{
			case irr::ELL_DEBUG:
				wxLogDebug(event.LogEvent.Text);
				break;
			case irr::ELL_INFORMATION:
				wxLogMessage(event.LogEvent.Text);
				break;
			case irr::ELL_WARNING:
				// an unset texture attribute has a value of '0' which throws a warning
				if (wxString(event.LogEvent.Text).Cmp("Could not open file of texture: 0") == 0)
					break;
				wxLogWarning(event.LogEvent.Text);
				break;
			case irr::ELL_ERROR:
				wxLogError(event.LogEvent.Text);
				break;
			case irr::ELL_NONE:
				wxLogMessage(event.LogEvent.Text);
				break;
			}
		} return true;
		}

		return false;
	}
};

ProjectEditor::ProjectEditor(MainWindow* parent, wxMenu* editMenu,
	BrowserWindow* browserWindow, const wxFileName& fileName)
	: Editor(parent, editMenu, PROJECT_EDITOR, browserWindow), m_FileName(fileName)
{
	m_AuiMgr.SetManagedWindow(this);

	m_Pages = new wxAuiNotebook(this);
	m_AuiMgr.AddPane(m_Pages, wxAuiPaneInfo().CenterPane());

	m_Explorer = new ProjectExplorer(this);
	m_AuiMgr.AddPane(m_Explorer, wxAuiPaneInfo().Right()
		.Caption(_("Explorer")).MinSize(250, 250));

	wxAuiManager& mainAui = parent->GetAuiMgr();
	wxFileSystem fs;

	wxAuiToolBar* advancedTools = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxAUI_TB_HORIZONTAL | wxAUI_TB_HORZ_LAYOUT);
	wxVector<wxBitmap> actorTool;
	actorTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/actor32.png", wxBITMAP_TYPE_PNG));
	actorTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/actor48.png", wxBITMAP_TYPE_PNG));
	actorTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/actor64.png", wxBITMAP_TYPE_PNG));
	advancedTools->AddTool(TOOL_ACTORBROWSER, _("Actor Browser"), wxBitmapBundle::FromBitmaps(actorTool),
		_("Actor Browser"));

	wxVector<wxBitmap> textureTool;
	textureTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/texture32.png", wxBITMAP_TYPE_PNG));
	textureTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/texture48.png", wxBITMAP_TYPE_PNG));
	textureTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/texture64.png", wxBITMAP_TYPE_PNG));
	advancedTools->AddTool(TOOL_TEXTUREBROWSER, _("Texture Browser"), wxBitmapBundle::FromBitmaps(textureTool),
		_("Texture Browser"));

	wxVector<wxBitmap> soundTool;
	soundTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/sound32.png", wxBITMAP_TYPE_PNG));
	soundTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/sound48.png", wxBITMAP_TYPE_PNG));
	soundTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/sound64.png", wxBITMAP_TYPE_PNG));
	advancedTools->AddTool(TOOL_SOUNDBROWSER, _("Sound Browser"), wxBitmapBundle::FromBitmaps(soundTool),
		_("Sound Browser"));

	//wxVector<wxBitmap> playTool;
	//playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play32.png", wxBITMAP_TYPE_PNG));
	//playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play48.png", wxBITMAP_TYPE_PNG));
	//playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play64.png", wxBITMAP_TYPE_PNG));
	//advancedTools->AddTool(TOOL_PLAYMAP, _("Play Map"), wxBitmapBundle::FromBitmaps(playTool),
	//	_("Play Map"));

	advancedTools->Realize();
	mainAui.AddPane(advancedTools, wxAuiPaneInfo().ToolbarPane().Caption(_("Tools")).CloseButton(false)
		.Top());

	wxAuiToolBar* utilityTools = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxAUI_TB_HORIZONTAL | wxAUI_TB_HORZ_LAYOUT);
	wxVector<wxBitmap> packageManagerTool;
	packageManagerTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/package32.png", wxBITMAP_TYPE_PNG));
	packageManagerTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/package48.png", wxBITMAP_TYPE_PNG));
	packageManagerTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/package64.png", wxBITMAP_TYPE_PNG));
	utilityTools->AddTool(TOOL_PACKAGEMANAGER, _("Package Manager"), wxBitmapBundle::FromBitmaps(packageManagerTool),
		_("Package Manager"));

	utilityTools->Realize();
	mainAui.AddPane(utilityTools, wxAuiPaneInfo().ToolbarPane().Caption(_("Utility")).CloseButton(false)
		.Top());

	m_AuiMgr.Update();

	irr::SIrrlichtCreationParameters params;
	params.DriverType = irr::video::EDT_NULL;
	params.EventReceiver = new IrrEventReceiver;
#if defined(_DEBUG)
	params.LoggingLevel = irr::ELL_DEBUG;
#endif

	m_RenderDevice = irr::createDeviceEx(params);

	// register the filesystem handler
	m_RenderDevice->getFileSystem()->setFileListSystem(irr::io::FILESYSTEM_VIRTUAL);
	if (!m_RenderDevice->getFileSystem()->addFileArchive(new IrrFSHandler))
	{
		wxLogError("Failed to mount base resources");
		return;
	}

	m_Browser->SetRenderDevice(m_RenderDevice);

	Load(m_FileName);

	wxAcceleratorEntry accelEntries[1];
	accelEntries[0].Set(wxACCEL_CTRL | wxACCEL_SHIFT, WXK_CONTROL_B, MENU_BUILDPROJECT);

	wxAcceleratorTable accelTable(1, accelEntries);
	parent->SetAcceleratorTable(accelTable);

	m_Pages->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &ProjectEditor::OnPageClose, this);
	parent->Bind(wxEVT_MENU, &ProjectEditor::OnBuildProject, this, MENU_BUILDPROJECT);
}

ProjectEditor::~ProjectEditor(void)
{
	m_AuiMgr.UnInit();
	GetParent()->SetAcceleratorTable(wxNullAcceleratorTable);
}

void ProjectEditor::OpenFile(const wxFileName& fileName)
{
	// try to find an existing instance
	for (size_t i = 0; i < m_Pages->GetPageCount(); ++i)
	{
		if (m_Pages->GetPageText(i) == fileName.GetFullName())
		{
			m_Pages->SetSelection(i);
			return;
		}
	}

	// new instance required
	if (fileName.GetExt().CompareTo(wxT("js"), wxString::ignoreCase) == 0 ||
		fileName.GetExt().CompareTo(wxT("xml"), wxString::ignoreCase) == 0)
	{
		ScriptEditor* editor = new ScriptEditor(m_Pages, m_EditMenu, fileName);
		m_Pages->AddPage(editor, fileName.GetFullName(), true, -1);
	}
}

void ProjectEditor::Load(const wxFileName& filePath)
{
	wxBusyInfo wait(wxBusyInfoFlags()
		.Parent(this)
		.Title(_("Opening project"))
		.Text(_("Please wait..."))
		.Foreground(*wxBLACK)
		.Background(*wxWHITE));

	m_Explorer->Load(filePath);

	m_Title = filePath.GetFullName();
	//if (filePath.IsOk())
	//{
	//	m_Title = filePath.GetFullName();
	//}
	//else
	//{
	//	m_Title.assign(_("untitled"));
	//}

	m_FileName = filePath;
}

bool ProjectEditor::HasChanged(void)
{
	//if (m_Explorer->HasChanged())
	//	return true;

	for (size_t i = 0; i < m_Pages->GetPageCount(); ++i)
	{
		EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetPage(i));
		if (page->HasChanged())
		{
			m_Pages->SetSelection(i);
			return true;
		}
	}

	return false;
}

void ProjectEditor::OnUndo(void)
{
	if (m_Pages->GetPageCount() > 0)
	{
		EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetCurrentPage());
		page->OnUndo();
	}
}

void ProjectEditor::OnRedo(void)
{
	if (m_Pages->GetPageCount() > 0)
	{
		EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetCurrentPage());
		page->OnRedo();
	}
}

bool ProjectEditor::OnSave(bool allFiles)
{
	//if (!m_Explorer->HasFilename())
	//	return OnSaveAs();

	//wxBusyInfo wait(wxBusyInfoFlags()
	//	.Parent(this)
	//	.Title(_("Saving project"))
	//	.Text(_("Please wait..."))
	//	.Foreground(*wxBLACK)
	//	.Background(*wxWHITE));

	//m_Explorer->Save(wxFileName());

	if (allFiles)
	{
		for (size_t i = 0; i < m_Pages->GetPageCount(); ++i)
		{
			EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetPage(i));
			if (page && page->HasChanged())
				page->Save();
		}
	}
	else
	{
		// process the active page
		EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetCurrentPage());
		if (page && page->HasChanged())
			page->Save();
	}

	//m_Title = m_Explorer->GetFilename().GetFullName();
	return true;
}

bool ProjectEditor::OnSaveAs(void)
{
	//wxString projectPath = m_Explorer->GetFilename().GetPath();
	//wxFileDialog saveDialog(this,
	//	_("Save Project As..."), projectPath,
	//	m_Explorer->GetFilename().GetFullName(),
	//	_("Manifold Editor Project (*.mep)|*.mep"),
	//	wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	//if (saveDialog.ShowModal() == wxID_CANCEL)
	//	return false; // not saving today

	//wxFileName fileName(saveDialog.GetPath());

	//wxBusyInfo wait(wxBusyInfoFlags()
	//	.Parent(this)
	//	.Title(_("Saving project"))
	//	.Text(_("Please wait..."))
	//	.Foreground(*wxBLACK)
	//	.Background(*wxWHITE));

	//m_Explorer->Save(fileName);

	//// process the active page
	//EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetCurrentPage());
	//if (page && page->HasChanged())
	//	page->Save();

	//m_Title = m_Explorer->GetFilename().GetFullName();
	return true;
}

void ProjectEditor::OnCut(void)
{
	EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetCurrentPage());
	if (page)
		page->OnCut();
}

void ProjectEditor::OnCopy(void)
{
	EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetCurrentPage());
	if (page)
		page->OnCopy();
}

void ProjectEditor::OnPaste(void)
{
	EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetCurrentPage());
	if (page)
		page->OnPaste();
}

void ProjectEditor::OnDelete(void)
{
}

void ProjectEditor::OnToolAction(wxCommandEvent& event)
{
}

void ProjectEditor::OnPageClose(wxAuiNotebookEvent& event)
{
	EditorPage* page = dynamic_cast<EditorPage*>(m_Pages->GetPage(event.GetSelection()));
	if (page && page->HasChanged())
	{
		wxMessageDialog check(this,
			_("Do you wish to save your changes?"),
			_("Unsaved changes"), wxYES_NO | wxCANCEL);
		int result = check.ShowModal();
		if (result == wxID_CANCEL)
			event.Veto();
		else if (result == wxID_YES)
			page->Save();
	}
}

void ProjectEditor::OnBuildProject(wxCommandEvent& event)
{
	m_Explorer->OnMenuBuildProject(event);
}
