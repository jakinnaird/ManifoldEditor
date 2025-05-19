/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Common.hpp"
#include "FSHandler.hpp"
#include "MainWindow.hpp"
#include "MapEditor.hpp"
#include "Serialize.hpp"

#include <wx/busyinfo.h>
#include <wx/confbase.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

MapEditor::MapEditor(MainWindow* parent, wxMenu* editMenu, 
    BrowserWindow* browserWindow, const wxFileName& mapName)
	: Editor(parent, editMenu, Editor::MAP_EDITOR, browserWindow), 
        m_FileName(mapName)
{
	m_AuiMgr.SetManagedWindow(this);
    m_Commands.Initialize();
    m_Commands.SetEditMenu(m_EditMenu);

	m_ExplorerPanel = new ExplorerPanel(this, m_Commands, m_Browser);
	m_PropertyPanel = new PropertyPanel(this, m_Commands);
	m_ViewPanel = new ViewPanel(this, m_Commands, m_Browser, m_ExplorerPanel, m_PropertyPanel);

	m_AuiMgr.AddPane(m_ViewPanel, wxAuiPaneInfo().CenterPane());
	m_AuiMgr.AddPane(m_ExplorerPanel, wxAuiPaneInfo().Right()
		.Caption(_("Explorer")).MinSize(250, 250));
	m_AuiMgr.AddPane(m_PropertyPanel, wxAuiPaneInfo().Right()
		.Caption(_("Properties")).MinSize(250, 250));

    wxAuiManager& mainAui = parent->GetAuiMgr();

    wxFileSystem fs;

    // add tool bars
    wxAuiToolBar* alignTools = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxAUI_TB_HORIZONTAL | wxAUI_TB_HORZ_LAYOUT);
    wxVector<wxBitmap> alignTop;
    alignTop.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-top32.png", wxBITMAP_TYPE_PNG));
    alignTop.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-top48.png", wxBITMAP_TYPE_PNG));
    alignTop.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-top64.png", wxBITMAP_TYPE_PNG));
    alignTools->AddTool(MENU_ALIGNTOP, _("Align top"), wxBitmapBundle::FromBitmaps(alignTop),
        _("Align selection tops"));
    wxVector<wxBitmap> alignMiddle;
    alignMiddle.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-middle32.png", wxBITMAP_TYPE_PNG));
    alignMiddle.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-middle48.png", wxBITMAP_TYPE_PNG));
    alignMiddle.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-middle64.png", wxBITMAP_TYPE_PNG));
    alignTools->AddTool(MENU_ALIGNMIDDLE, _("Align middle"), wxBitmapBundle::FromBitmaps(alignMiddle),
        _("Align selection middles"));
    wxVector<wxBitmap> alignBottom;
    alignBottom.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-bottom32.png", wxBITMAP_TYPE_PNG));
    alignBottom.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-bottom48.png", wxBITMAP_TYPE_PNG));
    alignBottom.push_back(BitmapFromFS(fs, "editor.mpk:icons/align-bottom64.png", wxBITMAP_TYPE_PNG));
    alignTools->AddTool(MENU_ALIGNBOTTOM, _("Align bottom"), wxBitmapBundle::FromBitmaps(alignBottom),
        _("Align selection bottoms"));
    alignTools->Realize();
    mainAui.AddPane(alignTools, wxAuiPaneInfo().ToolbarPane().Caption(_("Alignment")).CloseButton(false)
        .Top());

    wxAuiToolBar* geometryTools = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxAUI_TB_HORIZONTAL | wxAUI_TB_HORZ_LAYOUT);
    wxVector<wxBitmap> cubeTool;
    cubeTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/cube32.png", wxBITMAP_TYPE_PNG));
    cubeTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/cube48.png", wxBITMAP_TYPE_PNG));
    cubeTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/cube64.png", wxBITMAP_TYPE_PNG));
    geometryTools->AddTool(TOOL_CUBE, _("Cube brush"), wxBitmapBundle::FromBitmaps(cubeTool),
        _("Add cube brush"));

    wxVector<wxBitmap> cylinderTool;
    cylinderTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/cylinder32.png", wxBITMAP_TYPE_PNG));
    cylinderTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/cylinder48.png", wxBITMAP_TYPE_PNG));
    cylinderTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/cylinder64.png", wxBITMAP_TYPE_PNG));
    geometryTools->AddTool(TOOL_CYLINDER, _("Cylinder brush"), wxBitmapBundle::FromBitmaps(cylinderTool),
        _("Add cylinder brush"));

    wxVector<wxBitmap> sphereTool;
    sphereTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/sphere32.png", wxBITMAP_TYPE_PNG));
    sphereTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/sphere48.png", wxBITMAP_TYPE_PNG));
    sphereTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/sphere64.png", wxBITMAP_TYPE_PNG));
    geometryTools->AddTool(TOOL_SPHERE, _("Sphere brush"), wxBitmapBundle::FromBitmaps(sphereTool),
        _("Add sphere brush"));

    wxVector<wxBitmap> planeTool;
    planeTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/plane32.png", wxBITMAP_TYPE_PNG));
    planeTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/plane48.png", wxBITMAP_TYPE_PNG));
    planeTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/plane64.png", wxBITMAP_TYPE_PNG));
    geometryTools->AddTool(TOOL_PLANE, _("Plane brush"), wxBitmapBundle::FromBitmaps(planeTool),
        _("Add plane brush"));

    wxVector<wxBitmap> terrainTool;
    terrainTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/terrain32.png", wxBITMAP_TYPE_PNG));
    terrainTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/terrain48.png", wxBITMAP_TYPE_PNG));
    terrainTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/terrain64.png", wxBITMAP_TYPE_PNG));
    geometryTools->AddTool(TOOL_TERRAIN, _("Terrain brush"), wxBitmapBundle::FromBitmaps(terrainTool),
        _("Add terrain brush"));

    wxVector<wxBitmap> skyboxTool;
    skyboxTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/skybox32.png", wxBITMAP_TYPE_PNG));
    skyboxTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/skybox48.png", wxBITMAP_TYPE_PNG));
    skyboxTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/skybox64.png", wxBITMAP_TYPE_PNG));
    geometryTools->AddTool(TOOL_SKYBOX, _("Skybox brush"), wxBitmapBundle::FromBitmaps(skyboxTool),
        _("Add skybox brush"));

    geometryTools->Realize();
    mainAui.AddPane(geometryTools, wxAuiPaneInfo().ToolbarPane().Caption(_("Geometry")).CloseButton(false)
        .Top());

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

    wxVector<wxBitmap> meshTool;
    meshTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/tunnel32.png", wxBITMAP_TYPE_PNG));
    meshTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/tunnel48.png", wxBITMAP_TYPE_PNG));
    meshTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/tunnel64.png", wxBITMAP_TYPE_PNG));
    advancedTools->AddTool(TOOL_MESHBROWSER, _("Mesh Browser"), wxBitmapBundle::FromBitmaps(meshTool),
        _("Mesh Browser"));

    wxVector<wxBitmap> lightTool;
    lightTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/light32.png", wxBITMAP_TYPE_PNG));
    lightTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/light48.png", wxBITMAP_TYPE_PNG));
    lightTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/light64.png", wxBITMAP_TYPE_PNG));
    advancedTools->AddTool(TOOL_CALCLIGHTING, _("Recompute Lighting"), wxBitmapBundle::FromBitmaps(lightTool),
        _("Recompute Lighting"));

    wxVector<wxBitmap> playTool;
    playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play32.png", wxBITMAP_TYPE_PNG));
    playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play48.png", wxBITMAP_TYPE_PNG));
    playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play64.png", wxBITMAP_TYPE_PNG));
    advancedTools->AddTool(TOOL_PLAYMAP, _("Play Map"), wxBitmapBundle::FromBitmaps(playTool),
        _("Play Map"));

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

    Load(m_FileName);

    Bind(wxEVT_IDLE, &MapEditor::OnIdle, this);

    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, MENU_ALIGNTOP);
    Bind(wxEVT_MENU, &ViewPanel::OnMenuAlignTop, m_ViewPanel, MENU_ALIGNTOP);
    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, MENU_ALIGNMIDDLE);
    Bind(wxEVT_MENU, &ViewPanel::OnMenuAlignMiddle, m_ViewPanel, MENU_ALIGNMIDDLE);
    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, MENU_ALIGNBOTTOM);
    Bind(wxEVT_MENU, &ViewPanel::OnMenuAlignBottom, m_ViewPanel, MENU_ALIGNBOTTOM);

    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, TOOL_CUBE);
    Bind(wxEVT_MENU, &ViewPanel::OnToolCube, m_ViewPanel, TOOL_CUBE);
    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, TOOL_CYLINDER);
    Bind(wxEVT_MENU, &ViewPanel::OnToolCylinder, m_ViewPanel, TOOL_CYLINDER);
    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, TOOL_SPHERE);
    Bind(wxEVT_MENU, &ViewPanel::OnToolSphere, m_ViewPanel, TOOL_SPHERE);
    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, TOOL_PLANE);
    Bind(wxEVT_MENU, &ViewPanel::OnToolPlane, m_ViewPanel, TOOL_PLANE);
    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, TOOL_TERRAIN);
    Bind(wxEVT_MENU, &ViewPanel::OnToolTerrain, m_ViewPanel, TOOL_TERRAIN);
    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, TOOL_SKYBOX);
    Bind(wxEVT_MENU, &ViewPanel::OnToolSkybox, m_ViewPanel, TOOL_SKYBOX);

    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, TOOL_CALCLIGHTING);
    Bind(wxEVT_MENU, &MapEditor::OnToolsRecomputeLighting, this, TOOL_CALCLIGHTING);
    parent->Bind(wxEVT_MENU, &MainWindow::OnToolAction, parent, TOOL_PLAYMAP);
    Bind(wxEVT_MENU, &MapEditor::OnToolsPlayMap, this, TOOL_PLAYMAP);

    m_PlayMapProcess = nullptr;
}

MapEditor::~MapEditor(void)
{
    if (m_PlayMapProcess)
        m_PlayMapProcess->Detach();

    m_Map.reset();

    m_AuiMgr.UnInit();

    m_ViewPanel->Destroy();
    m_ExplorerPanel->Destroy();
    m_PropertyPanel->Destroy();
}

void MapEditor::PlayProcessTerminated(void)
{
    m_PlayMapProcess = nullptr;
}

int MapEditor::GetFPS(void)
{
    return m_ViewPanel->GetFPS();
}

void MapEditor::Load(const wxFileName& filePath)
{
    wxBusyInfo wait(wxBusyInfoFlags()
        .Parent(this)
        .Title(_("Opening map"))
        .Text(_("Please wait..."))
        .Foreground(*wxBLACK)
        .Background(*wxWHITE));

    if (filePath.IsOk())
    {
        m_Title = filePath.GetFullName();
        m_Map.reset(new Map(filePath));
    }
    else
    {
        m_Title.assign(_("untitled"));
        m_Map.reset(new Map);
    }

    m_ViewPanel->SetMap(m_Map);

    m_FileName = filePath;
}

bool MapEditor::HasChanged(void)
{
    return m_Commands.IsDirty();
}

void MapEditor::OnUndo(void)
{
    m_ViewPanel->ClearSelection();
    m_Commands.Undo();
    m_ViewPanel->Refresh(false);
}

void MapEditor::OnRedo(void)
{
    m_ViewPanel->ClearSelection();
    m_Commands.Redo();
    m_ViewPanel->Refresh(false);
}

bool MapEditor::OnSave(bool allFiles)
{
    if (!m_Map->HasFilename())
        return OnSaveAs();

    wxBusyInfo wait(wxBusyInfoFlags()
        .Parent(this)
        .Title(_("Saving map"))
        .Text(_("Please wait..."))
        .Foreground(*wxBLACK)
        .Background(*wxWHITE));

    m_Map->Save(wxFileName());
    m_Commands.MarkAsSaved();

    m_Title = m_Map->GetFileName().GetFullName();
    return true;
}

bool MapEditor::OnSaveAs(void)
{
    wxString mapPath = m_Map->GetFileName().GetPath();
    if (mapPath.empty())
        mapPath = wxConfigBase::Get()->Read(wxT("/Paths/MapPath"));

    wxFileDialog saveDialog(this,
        _("Save Map As..."), mapPath,
        m_Map->GetFileName().GetFullName(),
        ISerializerFactory::BuildFilter(),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveDialog.ShowModal() == wxID_CANCEL)
        return false; // not saving today

    wxFileName fileName(saveDialog.GetPath());
    
    wxBusyInfo wait(wxBusyInfoFlags()
        .Parent(this)
        .Title(_("Saving map"))
        .Text(_("Please wait..."))
        .Foreground(*wxBLACK)
        .Background(*wxWHITE));

    m_ViewPanel->ShowSelection(false);
    m_Map->Save(fileName);
    m_ViewPanel->ShowSelection(true);
    m_ExplorerPanel->SetMapName(m_Map->GetFileName().GetFullName());
    m_Commands.MarkAsSaved();

    m_Title = m_Map->GetFileName().GetFullName();
    return true;
}

void MapEditor::OnCut(void)
{
    wxCommandEvent event(wxEVT_MENU, wxID_CUT);
    m_ViewPanel->OnEditCut(event);
}

void MapEditor::OnCopy(void)
{
    wxCommandEvent event(wxEVT_MENU, wxID_COPY);
    m_ViewPanel->OnEditCopy(event);
}

void MapEditor::OnPaste(void)
{
    wxCommandEvent event(wxEVT_MENU, wxID_PASTE);
    m_ViewPanel->OnEditPaste(event);
}

void MapEditor::OnDelete(void)
{
    wxCommandEvent event(wxEVT_MENU, wxID_DELETE);
    m_ViewPanel->OnEditDelete(event);
}

void MapEditor::OnToolAction(wxCommandEvent& event)
{
    this->ProcessEvent(event);
}

void MapEditor::OnIdle(wxIdleEvent& event)
{
    if (m_PlayMapProcess)
    {
        m_PlayMapProcess->ProcessRedirect();
        event.RequestMore();
    }

    dynamic_cast<MainWindow*>(GetParent())->UpdateFrameTime(GetFPS());
}

void MapEditor::OnToolsRecomputeLighting(wxCommandEvent& event)
{
    if (m_Map)
        m_Map->RecomputeLighting(true);
}

void MapEditor::OnToolsPlayMap(wxCommandEvent& event)
{
    if (m_PlayMapProcess)
    {
        wxMessageBox(_("Instance already running"), _("Play Map"));
        return;
    }
    
    // the map needs to be saved
    if (m_Commands.IsDirty() || !m_Map->HasFilename())
    {
        wxMessageDialog check(this,
            _("You must save your map to continue. Save?"),
            _("Unsaved changes"), wxYES_NO | wxCANCEL);
        int result = check.ShowModal();
        if (result == wxID_CANCEL)
            return; // go no further
     
        wxCommandEvent cmdEvent;
        if (result == wxID_YES)
            OnSave(); // do the saving
    }
    
    // show the launcher dialog
    PlayLauncher launcher(this);
    if (launcher.ShowModal() == wxID_OK)
    {
        wxString gameExe = launcher.GetGameExe();
        wxString params = launcher.GetParams();

        m_PlayMapProcess = new PlayProcess(this);

        // build the mecc command line
        wxString cmd(gameExe);

        wxConfigBase::Get()->Write(wxT("/Editor/Launcher"), gameExe);

        // process the params
        if (params.Length() > 0)
        {
            wxConfigBase::Get()->Write(wxT("/Editor/LaunchParams"), params);

            // add the map path
            if (params.Contains(wxT("%mappath%")))
                params.Replace(wxT("%mappath%"), m_Map->GetFileName().GetFullPath());
            else
                params.Append(wxString::Format(wxT(" %s"), m_Map->GetFileName().GetFullPath()));

            cmd.Append(wxString::Format(wxT(" %s"), params));
        }
        else
            cmd.Append(wxString::Format(wxT(" %s"), m_Map->GetFileName().GetFullPath()));

        long pid = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_SHOW_CONSOLE, m_PlayMapProcess);
        if (pid > 0)
            m_PlayMapProcess->Activate();
        else
            SAFE_DELETE(m_PlayMapProcess);
    }
}
