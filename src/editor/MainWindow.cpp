/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Common.hpp"
#include "FSHandler.hpp"
#include "MainWindow.hpp"
#include "MapEditor.hpp"
#include "Preferences.hpp"
#include "ProjectEditor.hpp"
#include "Serialize.hpp"

#include <wx/aboutdlg.h>
#include <wx/artprov.h>
#include <wx/busyinfo.h>
#include <wx/confbase.h>
#include <wx/filedlg.h>
#include <wx/filesys.h>
#include <wx/iconbndl.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/textctrl.h>

#include <stdexcept>

#if defined(__WXMSW__)
#include "resource.h"
#endif

enum STATUSBAR_SECTIONS
{
    SBS_MAIN,
    SBS_FPS,

    SBS_MAXCOUNT
};

MainWindow::MainWindow(void)
    : wxFrame(nullptr, wxID_ANY, APP_NAME, wxDefaultPosition,
        wxSize(1024, 768))
{
#if defined(__WXMSW__)
    SetIcon(wxICON(IDI_ICON1));
#endif

    m_AuiMgr.SetManagedWindow(this);

    m_AudioSystem.reset(new AudioSystem());

    m_Browser = new BrowserWindow(this);
    m_Browser->SetAudioSystem(m_AudioSystem);
    m_PackageManager = new PackageManager(this);

    m_ActiveEditor = nullptr;

    wxMenu* menuNewFile = new wxMenu;
    menuNewFile->Append(MENU_NEW_MAP, _("&Map"));
    menuNewFile->Append(MENU_NEW_PROJECT, _("&Project"));

    wxMenu* menuOpenFile = new wxMenu;
    menuOpenFile->Append(MENU_OPEN_MAP, _("M&ap"));
    menuOpenFile->Append(MENU_OPEN_PROJECT, _("P&roject"));

    wxMenu* menuFile = new wxMenu;
    //menuFile->Append(wxID_NEW);
    menuFile->AppendSubMenu(menuNewFile, _("&New"), _("Create new content"));
    //menuFile->Append(wxID_OPEN);
    menuFile->AppendSubMenu(menuOpenFile, _("&Open"), _("Open content"));
    menuFile->Append(wxID_SAVE);
    menuFile->Append(wxID_SAVEAS);
    menuFile->Append(wxID_CLOSE);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_PREFERENCES);
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    m_EditMenu = new wxMenu;
    m_EditMenu->Append(wxID_UNDO);
    m_EditMenu->Append(wxID_REDO);
    m_EditMenu->AppendSeparator();
    m_EditMenu->Append(wxID_CUT);
    m_EditMenu->Append(wxID_COPY);
    m_EditMenu->Append(wxID_PASTE);
    m_EditMenu->Append(wxID_DELETE);
    //menuEdit->AppendSeparator();
    //menuEdit->Append(MENU_ALIGNTOP, _("Align top"), nullptr, _("Align selection tops"));
    //menuEdit->Append(MENU_ALIGNMIDDLE, _("Align middle"), nullptr, _("Align selection middles"));
    //menuEdit->Append(MENU_ALIGNBOTTOM, _("Align bottom"), nullptr, _("Align selection bottoms"));

    wxMenu* menuTools = new wxMenu;
    menuTools->Append(TOOL_PACKAGEMANAGER, _("Package Manager"), nullptr, _("Open the package manager"));
    menuTools->Append(TOOL_BROWSER, _("Entity Browser"), nullptr, _("Open the entity browser"));
    menuTools->Append(TOOL_ACTORBROWSER, _("Show Actor Browser"), nullptr, _("Open the actor browser"));
    menuTools->Append(TOOL_TEXTUREBROWSER, _("Show Texture Browser"), nullptr, _("Open the texture browser"));
    menuTools->Append(TOOL_SOUNDBROWSER, _("Show Sound Browser"), nullptr, _("Open the sound browser"));
    menuTools->Append(TOOL_MESHBROWSER, _("Show Mesh Browser"), nullptr, _("Open the mesh browser"));

    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(m_EditMenu, "&Edit");
    menuBar->Append(menuTools, "&Tools");
    menuBar->Append(menuHelp, "&Help");

    SetMenuBar(menuBar);

    wxTextCtrl* logBox = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    wxLog::SetActiveTarget(new wxLogTextCtrl(logBox));

    m_AuiMgr.AddPane(logBox, wxAuiPaneInfo().Layer(1).Bottom()
        .Dockable().Caption("Logs").CloseButton(false).MinSize(250, 80));

    wxFileSystem fs;

    wxAuiToolBar* fileTools = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxAUI_TB_HORIZONTAL | wxAUI_TB_HORZ_LAYOUT);
    fileTools->AddTool(wxID_OPEN, _("Open"), wxArtProvider::GetBitmap(wxART_FILE_OPEN),
        _("Open..."));
    fileTools->AddTool(wxID_SAVE, _("Save"), wxArtProvider::GetBitmap(wxART_FILE_SAVE),
        _("Save..."));
    fileTools->AddTool(wxID_SAVEAS, _("Save As"), wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS),
        _("Save as..."));
    fileTools->AddSeparator();
    fileTools->AddTool(wxID_UNDO, _("Undo"), wxArtProvider::GetBitmap(wxART_UNDO),
        _("Undo"));
    fileTools->AddTool(wxID_REDO, _("Redo"), wxArtProvider::GetBitmap(wxART_REDO),
        _("Redo"));
    fileTools->AddSeparator();
    fileTools->AddTool(wxID_CUT, _("Cut"), wxArtProvider::GetBitmap(wxART_CUT),
        _("Cut"));
    fileTools->AddTool(wxID_COPY, _("Copy"), wxArtProvider::GetBitmap(wxART_COPY),
        _("Copy"));
    fileTools->AddTool(wxID_PASTE, _("Paste"), wxArtProvider::GetBitmap(wxART_PASTE),
        _("Paste"));
    fileTools->Realize();
    m_AuiMgr.AddPane(fileTools, wxAuiPaneInfo().ToolbarPane().Caption(_("File")).CloseButton(false)
        .Top());

    CreateStatusBar();
    wxStatusBar* statusBar = GetStatusBar();

    int widths[SBS_MAXCOUNT] = {
        -1,     // main section (SBS_MAIN)
        60     // FPS (SBS_FPS)
    };

    statusBar->SetFieldsCount(SBS_MAXCOUNT, widths);

    SetStatusText("Manifold Editor");

    Bind(wxEVT_CLOSE_WINDOW, &MainWindow::OnClose, this);

    Bind(ME_CONFIGCHANGED, &MainWindow::OnConfigChanged, this);

    Bind(wxEVT_MENU, &MainWindow::OnFileNewMap, this, MENU_NEW_MAP);
    Bind(wxEVT_MENU, &MainWindow::OnFileNewProject, this, MENU_NEW_PROJECT);
    Bind(wxEVT_MENU, &MainWindow::OnFileOpenMap, this, MENU_OPEN_MAP);
    Bind(wxEVT_MENU, &MainWindow::OnFileOpenProject, this, MENU_OPEN_PROJECT);
    Bind(wxEVT_MENU, &MainWindow::OnFileOpen, this, wxID_OPEN);
    Bind(wxEVT_MENU, &MainWindow::OnFileSave, this, wxID_SAVE);
    Bind(wxEVT_MENU, &MainWindow::OnFileSaveAs, this, wxID_SAVEAS);
    Bind(wxEVT_MENU, &MainWindow::OnFileClose, this, wxID_CLOSE);
    Bind(wxEVT_MENU, &MainWindow::OnFilePreferences, this, wxID_PREFERENCES);
    Bind(wxEVT_MENU, &MainWindow::OnFileExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainWindow::OnEditUndo, this, wxID_UNDO);
    Bind(wxEVT_MENU, &MainWindow::OnEditRedo, this, wxID_REDO);
    Bind(wxEVT_MENU, &MainWindow::OnEditCut, this, wxID_CUT);
    Bind(wxEVT_MENU, &MainWindow::OnEditCopy, this, wxID_COPY);
    Bind(wxEVT_MENU, &MainWindow::OnEditPaste, this, wxID_PASTE);
    Bind(wxEVT_MENU, &MainWindow::OnEditDelete, this, wxID_DELETE);
    Bind(wxEVT_MENU, &MainWindow::OnHelpAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MainWindow::OnToolsEntityBrowser, this, TOOL_BROWSER);
    Bind(wxEVT_MENU, &MainWindow::OnToolsPackageManager, this, TOOL_PACKAGEMANAGER);
    Bind(wxEVT_MENU, &MainWindow::OnToolsActorBrowser, this, TOOL_ACTORBROWSER);
    Bind(wxEVT_MENU, &MainWindow::OnToolsTextureBrowser, this, TOOL_TEXTUREBROWSER);
    Bind(wxEVT_MENU, &MainWindow::OnToolsSoundBrowser, this, TOOL_SOUNDBROWSER);
    Bind(wxEVT_MENU, &MainWindow::OnToolsMeshBrowser, this, TOOL_MESHBROWSER);

    m_AuiMgr.Update();

    m_LastFPS = 0;
}

MainWindow::~MainWindow(void)
{
    m_AuiMgr.UnInit();

    m_Browser->Destroy();
    m_PackageManager->Destroy();

    m_AudioSystem.reset();
}

void MainWindow::LoadFile(const wxString& filePath)
{
    // test for special file paths, this can only happen on start up
    if (filePath.compare(wxT("*.mmp")) == 0) // this is a new empty map editor
    {
        m_ActiveEditor = new MapEditor(this, m_EditMenu, m_Browser, wxFileName());
    }
    else if (filePath.compare(wxT("*.mep")) == 0) // new empty project editor
    {
        m_ActiveEditor = new ProjectEditor(this, m_EditMenu, m_Browser, wxFileName());
    }
    else
    {
        wxFileName fileName(filePath);

        // set the working directory to be where the file is
        wxSetWorkingDirectory(fileName.GetPath());

        // it could be a project
        if (fileName.GetExt().CompareTo(wxT("mep")) == 0)
        {
            if (m_ActiveEditor)
                m_ActiveEditor->Load(fileName);
            else
                m_ActiveEditor = new ProjectEditor(this, m_EditMenu, m_Browser, fileName);
            
        }
        else // could be a map
        {
            // load the serializer
            std::shared_ptr<Serializer> serializer = ISerializerFactory::GetLoad(fileName);

            // verify the file is a map
            if (serializer->Verify() == Serializer::CONTENT_MAP)
            {
                serializer.reset();
                if (m_ActiveEditor)
                    m_ActiveEditor->Load(fileName);
                else
                    m_ActiveEditor = new MapEditor(this, m_EditMenu, m_Browser, fileName);
            }
            else
            {
                // if we are loaded with a valid file name set, even if it doesn't exist
                // play nicely with saving
                if (fileName.GetFullName().StartsWith(wxT("*.")))
                    fileName = wxFileName();

                // default to a new map editor
                if (m_ActiveEditor)
                    m_ActiveEditor->Load(/*wxFileName()*/fileName);
                else
                    m_ActiveEditor = new MapEditor(this, m_EditMenu, m_Browser, /*wxFileName()*/fileName);
            }
        }
    }

    m_AuiMgr.AddPane(m_ActiveEditor, wxAuiPaneInfo().CenterPane().DestroyOnClose());
    SetCaption(m_ActiveEditor->GetTitle());
    m_AuiMgr.Update();
}

void MainWindow::SetCaption(const wxString& fileName)
{
    wxString title(wxT(APP_NAME));
    if (!fileName.empty())
        title.append(wxString::Format(wxT(" [%s]"), fileName));

    SetTitle(title);
}

void MainWindow::UpdateFrameTime(int fps)
{
    if (m_LastFPS != fps)
        SetStatusText(wxString::Format(wxT("%d FPS"), fps), SBS_FPS);

    m_LastFPS = fps;
}

void MainWindow::OnToolAction(wxCommandEvent& event)
{
    if (m_ActiveEditor)
        m_ActiveEditor->OnToolAction(event);
}

void MainWindow::OnClose(wxCloseEvent& event)
{
    if (event.CanVeto())
    {
        if (m_ActiveEditor && m_ActiveEditor->HasChanged())
        {
            wxMessageDialog check(this,
                _("Do you wish to save your changes?"),
                _("Unsaved changes"), wxYES_NO | wxCANCEL);
            int result = check.ShowModal();
            if (result == wxID_CANCEL)
            {
                event.Veto();
                return; // go no further
            }

            if (result == wxID_YES)
                m_ActiveEditor->OnSave(true);
        }
    }

    event.Skip(); // continue the process
}

void MainWindow::OnConfigChanged(wxCommandEvent& event)
{
    wxLogMessage(wxT("MainWindow::OnConfigChanged"));
    //event.Skip();
}

void MainWindow::OnFileNewMap(wxCommandEvent& event)
{
    // get the executable path
    wxString cmd = wxStandardPaths::Get().GetExecutablePath();
    cmd.append(wxT(" *.mmp")); // ensure we create a new map editor
    wxExecute(cmd, wxEXEC_ASYNC);
}

void MainWindow::OnFileNewProject(wxCommandEvent& event)
{
    // are we already a project editor and do we have an active project?
    if (m_ActiveEditor && m_ActiveEditor->GetType() == Editor::PROJECT_EDITOR)
    {
        if (!dynamic_cast<ProjectEditor*>(m_ActiveEditor)->GetFileName().IsOk())
        {
            wxFileDialog saveDialog(this,
                _("New Project..."), wxEmptyString,
                wxEmptyString, wxT("Manifold Project (*.mep)|*.mep"),
                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (saveDialog.ShowModal() == wxID_CANCEL)
                return; // not saving today

            wxFileName fileName(saveDialog.GetPath());
            m_ActiveEditor->Load(fileName);
            return;
        }
    }
 
    // launch a new Project Editor
    // get the executable path
    wxString cmd = wxStandardPaths::Get().GetExecutablePath();
    cmd.append(wxT(" *.mep")); // ensure we create a new package editor
    wxExecute(cmd, wxEXEC_ASYNC);
}

void MainWindow::OnFileOpenMap(wxCommandEvent& event)
{
    wxString mapPath = wxConfigBase::Get()->Read(wxT("/Paths/MapPath"));

    if (m_ActiveEditor && 
        m_ActiveEditor->HasChanged())
    {
        wxMessageDialog check(this,
            _("Do you wish to save your changes?"),
            _("Unsaved changes"), wxYES_NO | wxCANCEL);
        int result = check.ShowModal();
        if (result == wxID_CANCEL)
            return; // go no further

        if (result == wxID_YES)
            m_ActiveEditor->OnSave(true);
    }

    wxFileDialog openDialog(this,
        _("Open map"), mapPath, wxEmptyString,
        ISerializerFactory::BuildFilter(),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openDialog.ShowModal() == wxID_CANCEL)
        return; // not opening today

    if (m_ActiveEditor && m_ActiveEditor->GetType() == Editor::MAP_EDITOR)
    {
        LoadFile(openDialog.GetPath());
    }
    else
    {
        // not a map editor, we need to open one
        // get the executable path
        wxString cmd = wxStandardPaths::Get().GetExecutablePath();
        cmd.append(wxString::Format(wxT(" \"%s\""), openDialog.GetPath())); // ensure we create a new map editor
        wxExecute(cmd, wxEXEC_ASYNC);
    }
}

void MainWindow::OnFileOpenProject(wxCommandEvent& event)
{
    if (m_ActiveEditor &&
        m_ActiveEditor->HasChanged())
    {
        wxMessageDialog check(this,
            _("Do you wish to save your changes?"),
            _("Unsaved changes"), wxYES_NO | wxCANCEL);
        int result = check.ShowModal();
        if (result == wxID_CANCEL)
            return; // go no further

        if (result == wxID_YES)
            m_ActiveEditor->OnSave(true);
    }

    wxFileDialog openDialog(this,
        _("Open project"), wxEmptyString, wxEmptyString,
        _("Manifold Editor Project (*.mep)|*.mep"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openDialog.ShowModal() == wxID_CANCEL)
        return; // not opening today

    if (m_ActiveEditor && m_ActiveEditor->GetType() == Editor::PROJECT_EDITOR)
    {
        LoadFile(openDialog.GetPath());
    }
    else
    {
        // not a project editor, we need to open one
        // get the executable path
        wxString cmd = wxStandardPaths::Get().GetExecutablePath();
        cmd.append(wxString::Format(wxT(" \"%s\""), openDialog.GetPath())); // ensure we create a new map editor
        wxExecute(cmd, wxEXEC_ASYNC);
    }
}

void MainWindow::OnFileOpen(wxCommandEvent& event)
{
    if (m_ActiveEditor && m_ActiveEditor->HasChanged())
    {
        wxMessageDialog check(this,
            _("Do you wish to save your changes?"),
            _("Unsaved changes"), wxYES_NO | wxCANCEL);
        int result = check.ShowModal();
        if (result == wxID_CANCEL)
            return; // go no further

        if (result == wxID_YES)
            m_ActiveEditor->OnSave(true);
    }

    wxString fileFilter(_("Manifold Editor Project (*.mep)|*.mep"));
    fileFilter.Append(wxT("|"));
    fileFilter.Append(ISerializerFactory::BuildFilter());

    wxFileDialog openDialog(this,
        _("Open content"), wxEmptyString, wxEmptyString,
        fileFilter,
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openDialog.ShowModal() == wxID_CANCEL)
        return; // not opening today

    LoadFile(openDialog.GetPath());
}

void MainWindow::OnFileSave(wxCommandEvent& event)
{
    if (m_ActiveEditor && m_ActiveEditor->OnSave(true))
    {
        SetCaption(m_ActiveEditor->GetTitle());
    }
}

void MainWindow::OnFileSaveAs(wxCommandEvent& event)
{
    if (m_ActiveEditor && m_ActiveEditor->OnSaveAs())
    {
        SetCaption(m_ActiveEditor->GetTitle());
    }
}

void MainWindow::OnFileClose(wxCommandEvent& event)
{
    if (m_ActiveEditor && m_ActiveEditor->HasChanged())
    {
        wxMessageDialog check(this,
            _("Do you wish to save your changes?"),
            _("Unsaved changes"), wxYES_NO | wxCANCEL);
        int result = check.ShowModal();
        if (result == wxID_CANCEL)
            return; // go no further

        if (result == wxID_YES)
            m_ActiveEditor->OnSave(true);
    }

    // what kind of default are we and recreate it
    if (m_ActiveEditor && m_ActiveEditor->GetType() == Editor::MAP_EDITOR)
        LoadFile(wxT("*.mmp"));
    else if (m_ActiveEditor && m_ActiveEditor->GetType() == Editor::PROJECT_EDITOR)
        LoadFile(wxT("*.mep"));
}

void MainWindow::OnFilePreferences(wxCommandEvent& event)
{
    PreferencesWindow prefs(this);
    if (prefs.ShowModal() == wxID_OK)
        prefs.ApplyChanges();
}

void MainWindow::OnFileExit(wxCommandEvent& event)
{
    Close(false);
}

void MainWindow::OnEditUndo(wxCommandEvent& event)
{
    if (m_ActiveEditor)
        m_ActiveEditor->OnUndo();
}

void MainWindow::OnEditRedo(wxCommandEvent& event)
{
    if (m_ActiveEditor)
        m_ActiveEditor->OnRedo();
}

void MainWindow::OnEditCut(wxCommandEvent& event)
{
    if (m_ActiveEditor)
        m_ActiveEditor->OnCut();
}

void MainWindow::OnEditCopy(wxCommandEvent& event)
{
    if (m_ActiveEditor)
        m_ActiveEditor->OnCopy();
}

void MainWindow::OnEditPaste(wxCommandEvent& event)
{
    if (m_ActiveEditor)
        m_ActiveEditor->OnPaste();
}

void MainWindow::OnEditDelete(wxCommandEvent& event)
{
    if (m_ActiveEditor)
        m_ActiveEditor->OnDelete();
}

void MainWindow::OnHelpAbout(wxCommandEvent& event)
{
    wxAboutDialogInfo info;
    info.SetName(APP_NAME);
    info.SetVersion(APP_VERSION);
    info.SetDescription("Manifold Editor content creation tool");
    info.SetCopyright("(c) 2023-2025");
    info.AddDeveloper("James Kinnaird");

    wxAboutBox(info, this);
}

void MainWindow::OnToolsEntityBrowser(wxCommandEvent& event)
{
    if (!m_Browser->IsVisible())
        m_Browser->Show();

    m_Browser->Raise();
}

void MainWindow::OnToolsActorBrowser(wxCommandEvent& event)
{
    OnToolsEntityBrowser(event);
    m_Browser->SwitchTo(BrowserWindow::PAGE_ACTORS);
}

void MainWindow::OnToolsTextureBrowser(wxCommandEvent& event)
{
    OnToolsEntityBrowser(event);
    m_Browser->SwitchTo(BrowserWindow::PAGE_TEXTURES);
}

void MainWindow::OnToolsSoundBrowser(wxCommandEvent& event)
{
    OnToolsEntityBrowser(event);
    m_Browser->SwitchTo(BrowserWindow::PAGE_SOUNDS);
}

void MainWindow::OnToolsMeshBrowser(wxCommandEvent& event)
{
    OnToolsEntityBrowser(event);
    m_Browser->SwitchTo(BrowserWindow::PAGE_MESHES);
}

void MainWindow::OnToolsPackageManager(wxCommandEvent& event)
{
    if (!m_PackageManager->IsVisible())
        m_PackageManager->Show();

    m_PackageManager->Raise();
}
