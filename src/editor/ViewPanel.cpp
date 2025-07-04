/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "BrowserWindow.hpp"
#include "Commands.hpp"
#include "Common.hpp"
#include "Component.hpp"
#include "FSHandler.hpp"
// #include "MainWindow.hpp"
#include "MapEditor.hpp"
#include "ViewPanel.hpp"

#include "../extend/PathSceneNode.hpp"
#include "../extend/SceneNodeFactory.hpp"

#include <wx/dcclient.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/sizer.h>

#include "irrUString.h"
#include "CGUITTFont.h"
#include "../source/Irrlicht/CSceneNodeAnimatorCameraFPS.h"

#if defined(__WXGTK__)
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif

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

ViewPanel::ViewPanel(wxWindow* parent, wxCommandProcessor& cmdProc,
	BrowserWindow* browserWindow,
	ExplorerPanel* explorerPanel, PropertyPanel* propertyPanel)
	: wxPanel(parent),
	  m_RefreshTimer(this), m_Commands(cmdProc),
	  m_Browser(browserWindow), m_ExplorerPanel(explorerPanel), 
	  m_PropertyPanel(propertyPanel), m_Init(false), m_ActiveView(VIEW_3D), 
	  m_FreeLook(false), m_RenderDevice(nullptr),
	  m_EditorRoot(nullptr), m_MapRoot(nullptr),
	  m_Camera(nullptr), m_TranslatingSelection(false),
	  m_TerrainEditor(nullptr), m_TerrainToolbar(nullptr), m_TerrainEditingMode(false), m_ActiveTerrain(nullptr)
{
	m_ExplorerPanel->SetViewPanel(this);

	// set the edit menu
	wxMenu* editMenu = dynamic_cast<MapEditor*>(GetParent())->GetEditMenu();
	editMenu->AppendSeparator();
	editMenu->Append(MENU_TERRAINEDIT, _("Edit Terrain"), nullptr, _("Edit selected terrain"));
	editMenu->Enable(MENU_TERRAINEDIT, false);

	wxFileSystem fs;
	m_Cursor[CURSOR_MOVE] = new wxCursor(ImageFromFS(fs, "editor.mpk:icons/move.png", wxBITMAP_TYPE_PNG));
	m_Cursor[CURSOR_ROTATE] = new wxCursor(ImageFromFS(fs, "editor.mpk:icons/rotate.png", wxBITMAP_TYPE_PNG));

	m_View[0] = m_View[1] = m_View[2] = m_View[3] = nullptr;
	m_Ortho[0] = m_Ortho[1] = m_Ortho[2] = nullptr;
	m_3DCam = nullptr;
	m_Grid[0] = m_Grid[1] = m_Grid[2] = m_Grid[3] = nullptr;
	m_Label[0] = m_Label[1] = m_Label[2] = m_Label[3] = nullptr;

	Bind(wxEVT_TIMER, &ViewPanel::OnTimer, this);
	Bind(wxEVT_SIZE, &ViewPanel::OnResize, this);
	Bind(wxEVT_PAINT, &ViewPanel::OnPaint, this);
	Bind(wxEVT_MOTION, &ViewPanel::OnMouse, this);
	Bind(wxEVT_LEFT_DOWN, &ViewPanel::OnMouse, this);
	Bind(wxEVT_MIDDLE_DOWN, &ViewPanel::OnMouse, this);
	Bind(wxEVT_RIGHT_DOWN, &ViewPanel::OnMouse, this);
	Bind(wxEVT_LEFT_UP, &ViewPanel::OnMouse, this);
	Bind(wxEVT_MIDDLE_UP, &ViewPanel::OnMouse, this);
	Bind(wxEVT_RIGHT_UP, &ViewPanel::OnMouse, this);
	Bind(wxEVT_MOUSEWHEEL, &ViewPanel::OnMouse, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, &ViewPanel::OnMouseCaptureLost, this);
	Bind(wxEVT_MOUSE_CAPTURE_CHANGED, &ViewPanel::OnMouseCaptureChanged, this);
	Bind(wxEVT_KEY_DOWN, &ViewPanel::OnKey, this);
	Bind(wxEVT_KEY_UP, &ViewPanel::OnKey, this);

	Bind(wxEVT_MENU, &ViewPanel::OnEditCut, this, wxID_CUT);
	Bind(wxEVT_MENU, &ViewPanel::OnEditCopy, this, wxID_COPY);
	Bind(wxEVT_MENU, &ViewPanel::OnEditPaste, this, wxID_PASTE);
	Bind(wxEVT_MENU, &ViewPanel::OnEditDelete, this, wxID_DELETE);
	Bind(wxEVT_MENU, &ViewPanel::OnMenuTerrainEdit, this, MENU_TERRAINEDIT);

	Bind(wxEVT_MENU, &ViewPanel::OnToolPlayerStart, this, TOOL_PLAYERSTART);
	Bind(wxEVT_MENU, &ViewPanel::OnToolLight, this, TOOL_LIGHT);
	Bind(wxEVT_MENU, &ViewPanel::OnToolPathNode, this, TOOL_PATHNODE);
	Bind(wxEVT_MENU, &ViewPanel::OnToolActor, this, TOOL_ACTOR);
	Bind(wxEVT_MENU, &ViewPanel::OnToolMesh, this, TOOL_MESH);
	Bind(wxEVT_MENU, &ViewPanel::OnMenuFreeLook, this, MENU_FREELOOK);
	Bind(wxEVT_MENU, &ViewPanel::OnMenuSetTexture, this, MENU_SETTEXTURE);
}

ViewPanel::~ViewPanel(void)
{
	delete m_Cursor[CURSOR_MOVE];
	delete m_Cursor[CURSOR_ROTATE];

	m_RefreshTimer.Stop();

	if (m_TerrainEditor)
	{
		m_TerrainEditor->shutdown();
		delete m_TerrainEditor;
		m_TerrainEditor = nullptr;
	}

	if (m_TerrainToolbar)
	{
		m_TerrainToolbar->Destroy();
		m_TerrainToolbar = nullptr;
	}

	if (m_RenderDevice)
	{
		m_RenderDevice->getCursorControl()->setVisible(true);
		m_RenderDevice->closeDevice();
		m_RenderDevice->drop();
		m_RenderDevice = nullptr;
	}

	if (HasCapture())
		ReleaseMouse();
}

irr::io::IFileSystem* ViewPanel::GetFileSystem(void)
{
	if (!m_Init)
		return nullptr;

	return m_RenderDevice->getFileSystem();
}

int ViewPanel::GetFPS(void)
{
	if (m_RenderDevice)
		return m_RenderDevice->getVideoDriver()->getFPS();

	return 0;
}

void ViewPanel::SetMap(std::shared_ptr<Map>& map)
{
	m_PropertyPanel->Clear();
	m_ExplorerPanel->Clear();
	ClearSelection();

	m_Map = map;

	m_PropertyPanel->SetMap(m_Map);

	if (m_Init)
	{
		m_MapRoot->removeAll();

		// we can create all the entities
		m_Map->SetSceneMgr(m_RenderDevice->getSceneManager());
		m_Map->Load(m_MapRoot, m_ExplorerPanel);

		// build all the path node links
		BuildPathLinks();
	}
}

void ViewPanel::AddToSelection(irr::scene::ISceneNode* node, bool append)
{
	bool removed = false;

	// check if node is already in there, remove if yes
	for (selection_t::iterator i = m_Selection.begin();
		i != m_Selection.end(); ++i)
	{
		if (strcmp((*i)->getName(), node->getName()) == 0)
		{
			node->setDebugDataVisible(irr::scene::EDS_OFF);
			m_ExplorerPanel->UnselectItem(node->getName());
			m_Selection.erase(i); // remove from the selection
			removed = true;
			break;
		}
	}

	if (!append)
		ClearSelection();

	if (!removed)
	{
		m_ExplorerPanel->SelectItem(node->getName());

		node->setDebugDataVisible(irr::scene::EDS_BBOX);
		m_Selection.push_back(node);
		if (m_Selection.size() > 1)
			m_PropertyPanel->Clear();
		else
			m_PropertyPanel->SetSceneNode(node);
		
		// Check if selected node is an UpdatableTerrainSceneNode
		if (m_Selection.size() == 1)
		{
			wxMenu* editMenu = dynamic_cast<MapEditor*>(GetParent())->GetEditMenu();

			if (node->getType() == irr::scene::ESNT_TERRAIN)
			{
				UpdatableTerrainSceneNode* terrain = dynamic_cast<UpdatableTerrainSceneNode*>(node);
				if (terrain)
				{
					// Auto-bind terrain to editor if terrain editing mode is enabled
					if (m_TerrainEditor && m_TerrainEditor->isEnabled())
					{
						m_ActiveTerrain = terrain;
						m_TerrainEditor->setTerrain(terrain);
					}
					else
					{
						// Store for potential later use
						m_ActiveTerrain = terrain;
					}

					// update the edit menu
					editMenu->Enable(MENU_TERRAINEDIT, true);
				}
			}
			else
			{
				// Clear active terrain if non-terrain node is selected
				m_ActiveTerrain = nullptr;
				if (m_TerrainEditor)
				{
					m_TerrainEditor->setTerrain(nullptr);
				}

				editMenu->Enable(MENU_TERRAINEDIT, false);
			}
		}
	}

	UpdateSelectionBoundingBox();
}

void ViewPanel::UpdateSelectionBoundingBox(void)
{
	m_SelectionBox.reset(0, 0, 0);

	for (selection_t::iterator i = m_Selection.begin();
		i != m_Selection.end(); ++i)
	{
		m_SelectionBox.addInternalBox((*i)->getTransformedBoundingBox());
	}
}

void ViewPanel::ShowSelection(bool show)
{
	for (selection_t::iterator i = m_Selection.begin();
		i != m_Selection.end(); ++i)
	{
		(*i)->setDebugDataVisible(show ? irr::scene::EDS_BBOX : irr::scene::EDS_OFF);
	}
}

void ViewPanel::ClearSelection(void)
{
	m_ExplorerPanel->UnselectAll();
	m_PropertyPanel->Clear();
	m_SelectionBox.reset(0, 0, 0);

	for (selection_t::iterator i = m_Selection.begin();
		i != m_Selection.end(); i = m_Selection.erase(i))
	{
		(*i)->setDebugDataVisible(irr::scene::EDS_OFF);
	}
	
	// Clear active terrain when selection is cleared
	m_ActiveTerrain = nullptr;
	if (m_TerrainEditor)
	{
		m_TerrainEditor->setTerrain(nullptr);
	}

	wxMenu* editMenu = dynamic_cast<MapEditor*>(GetParent())->GetEditMenu();
	editMenu->Enable(MENU_TERRAINEDIT, false);
}

void ViewPanel::DeleteSelection(void)
{
	if (m_Selection.size())
	{
		DeleteNodeCommand::selection_t selection;
		for (selection_t::iterator node = m_Selection.begin();
			node != m_Selection.end(); ++node)
		{
			(*node)->setDebugDataVisible(irr::scene::EDS_OFF);
			selection.push_back(wxString((*node)->getName()));
		}

		m_Commands.Submit(new DeleteNodeCommand(m_ExplorerPanel,
			m_RenderDevice->getSceneManager(), m_MapRoot,
			m_Map, selection));

		m_ExplorerPanel->UnselectAll();
		m_PropertyPanel->Clear();
		m_SelectionBox.reset(0, 0, 0);
		m_Selection.clear();
	}
}

void ViewPanel::BeginFreeLook(void)
{
	const wxSize& size = GetSize() * GetContentScaleFactor();

	if (m_RenderDevice)
	{
		// warp the pointer to the center of the 3D view
		wxPoint origin = GetScreenPosition();
		irr::core::recti rect(origin.x + (size.x / 2),
			origin.y + (size.y / 2), origin.x + size.x, origin.y + size.y);
		m_RenderDevice->getCursorControl()->setReferenceRect(&rect);

		// add the FPS camera
		m_View[VIEW_3D]->addAnimator(m_3DCam);

		m_RenderDevice->getCursorControl()->setVisible(false);
	}

	CaptureMouse();
	m_FreeLook = true;
}

void ViewPanel::EndFreeLook(void)
{
	m_FreeLook = false;
	if (m_RenderDevice)
	{
		m_RenderDevice->getCursorControl()->setReferenceRect(nullptr);
		m_View[VIEW_3D]->removeAnimator(m_3DCam);
		m_RenderDevice->getCursorControl()->setVisible(true);
	}

	if (HasCapture())
		ReleaseMouse();
}

void ViewPanel::OnTimer(wxTimerEvent& event)
{
	// only refresh if we are visible
	if (IsShownOnScreen())
	{
		if (m_RenderDevice)
		{
			m_RenderDevice->getTimer()->tick();
			
			// Update terrain editor
			if (m_TerrainEditor && m_TerrainEditor->isEnabled())
			{
				irr::f32 deltaTime = m_RenderDevice->getTimer()->getTime() / 1000.0f;
				m_TerrainEditor->update(deltaTime);
			}
		}

		Refresh(false); // Generate paint event without erasing the background
	}
}

void ViewPanel::OnResize(wxSizeEvent& event)
{
	event.Skip();

	if (!IsShownOnScreen())
		return;

	const wxSize& size = event.GetSize() * GetContentScaleFactor();

	// ensure we're initialized
	if (!m_Init && size.x > 2 && size.y > 2)
	{
		irr::SIrrlichtCreationParameters params;
		params.DriverType = irr::video::EDT_OPENGL;
		params.DeviceType = irr::EIDT_BEST;
		params.EventReceiver = new IrrEventReceiver;
		params.Stencilbuffer = true;
		params.HandleSRGB = true;
		params.UsePerformanceTimer = true;
		params.Doublebuffer = true;
#if defined(_DEBUG)
		params.LoggingLevel = irr::ELL_DEBUG;
#endif

#if defined(__WXMSW__)	// Windows
		m_VideoData.OpenGLWin32.HWnd = GetHandle();
		params.WindowId = reinterpret_cast<void*>(m_VideoData.OpenGLWin32.HWnd);
		//m_VideoData.OpenGLWin32.HDc = this->GetHDC();
		//m_VideoData.OpenGLWin32.HRc = m_Context->GetGLRC();
#elif defined(__WXGTK__)
		// https://forums.wxwidgets.org/viewtopic.php?t=29850
		// https://stackoverflow.com/a/14788489
		GtkWidget* widget = GetHandle();
		gtk_widget_realize(widget);
		m_VideoData.OpenGLLinux.X11Window = gdk_x11_window_get_xid(gtk_widget_get_window(widget));;
		params.WindowId = reinterpret_cast<void*>(m_VideoData.OpenGLLinux.X11Window);
		//m_VideoData.OpenGLLinux.X11Context;
		//m_VideoData.OpenGLLinux.X11Display;
#elif defined(__WXOSX__)
#error TODO
#endif

		if (m_RenderDevice == nullptr)
			m_RenderDevice = irr::createDeviceEx(params);
		if (!m_RenderDevice)
		{
			wxLogError("Unable to create Irrlicht device");
			return;
		}

		// register the filesystem handler
		m_RenderDevice->getFileSystem()->setFileListSystem(irr::io::FILESYSTEM_VIRTUAL);
		if (!m_RenderDevice->getFileSystem()->addFileArchive(new IrrFSHandler))
		{
			wxLogError("Failed to mount base resources");
			return;
		}

		// register the scene node factory
		irr::scene::ISceneNodeFactory* factory = new SceneNodeFactory(m_RenderDevice->getSceneManager());
		m_RenderDevice->getSceneManager()->registerSceneNodeFactory(factory);
		factory->drop();

		// register the component factory
		irr::scene::ISceneNodeAnimatorFactory* animatorFactory = new ComponentFactory(m_RenderDevice->getSceneManager());
		m_RenderDevice->getSceneManager()->registerSceneNodeAnimatorFactory(animatorFactory);
		animatorFactory->drop();

		// create default font
		irr::io::path defaultFontUri("editor.mpk:fonts/Gen-Light5.ttf");
		irr::gui::CGUITTFont* defaultFont = irr::gui::CGUITTFont::create(
			m_RenderDevice->getGUIEnvironment(), defaultFontUri, 28);
		if (defaultFont)
		{
			m_RenderDevice->getGUIEnvironment()->addFont(defaultFontUri, defaultFont);
			m_RenderDevice->getGUIEnvironment()->getSkin()->setFont(defaultFont);
		}
		else
			wxLogWarning("Failed to load default font, using built-in as default");

		m_EditorRoot = m_RenderDevice->getSceneManager()->addEmptySceneNode(nullptr,
			NID_NOSAVE);
		m_MapRoot = m_RenderDevice->getSceneManager()->addEmptySceneNode(nullptr,
			NID_NOSAVE);
		m_Camera = m_RenderDevice->getSceneManager()->addBillboardSceneNode(m_EditorRoot,
			irr::core::dimension2df(5, 5), irr::core::vector3df(0, 0, 0), NID_NOSAVE);
		m_Camera->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		m_Camera->setMaterialFlag(irr::video::EMF_ZWRITE_ENABLE, false);
		m_Camera->setMaterialType(irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);
		m_Camera->setMaterialTexture(0, m_RenderDevice->getVideoDriver()->getTexture(
			"editor.mpk:icons/camera.png"));

		m_View[VIEW_FRONT] = m_RenderDevice->getSceneManager()->addCameraSceneNode(m_EditorRoot,
			irr::core::vector3df(0, 0, 1000), irr::core::vector3df(0, 0, 0), NID_NOSAVE);
		m_Ortho[VIEW_FRONT] = new irr::scene::CSceneNodeAnimatorCameraOrtho(
			m_RenderDevice->getCursorControl(), irr::core::dimension2du(size.x, size.y),
			irr::scene::CSceneNodeAnimatorCameraOrtho::EOO_XY);
		m_View[VIEW_FRONT]->addAnimator(m_Ortho[VIEW_FRONT]);
		m_Label[VIEW_FRONT] = m_RenderDevice->getGUIEnvironment()->addStaticText(
			_("FRONT").wc_str(), irr::core::recti(10, 10, 100, 30), false);
		m_Label[VIEW_FRONT]->setOverrideColor(irr::video::SColor(255, 0, 0, 255));
		m_Label[VIEW_FRONT]->setVisible(false);

		m_View[VIEW_TOP] = m_RenderDevice->getSceneManager()->addCameraSceneNode(m_EditorRoot,
			irr::core::vector3df(0, 1000, 0), irr::core::vector3df(0, 0, 0), NID_NOSAVE);
		m_View[VIEW_TOP]->setUpVector(irr::core::vector3df(0, 0, -1));
		m_Ortho[VIEW_TOP] = new irr::scene::CSceneNodeAnimatorCameraOrtho(
			m_RenderDevice->getCursorControl(), irr::core::dimension2du(size.x, size.y),
			irr::scene::CSceneNodeAnimatorCameraOrtho::EOO_XZ);
		m_View[VIEW_TOP]->addAnimator(m_Ortho[VIEW_TOP]);
		m_Label[VIEW_TOP] = m_RenderDevice->getGUIEnvironment()->addStaticText(
			_("TOP").wc_str(), irr::core::recti(10, 10, 75, 30), false);
		m_Label[VIEW_TOP]->setOverrideColor(irr::video::SColor(255, 0, 0, 255));
		m_Label[VIEW_TOP]->setVisible(false);

		m_View[VIEW_RIGHT] = m_RenderDevice->getSceneManager()->addCameraSceneNode(m_EditorRoot,
			irr::core::vector3df(1000, 0, 0), irr::core::vector3df(0, 0, 0), NID_NOSAVE);
		m_Ortho[VIEW_RIGHT] = new irr::scene::CSceneNodeAnimatorCameraOrtho(
			m_RenderDevice->getCursorControl(), irr::core::dimension2du(size.x, size.y),
			irr::scene::CSceneNodeAnimatorCameraOrtho::EOO_YZ);
		m_View[VIEW_RIGHT]->addAnimator(m_Ortho[VIEW_RIGHT]);
		m_Label[VIEW_RIGHT] = m_RenderDevice->getGUIEnvironment()->addStaticText(
			_("RIGHT").wc_str(), irr::core::recti(10, 10, 100, 30), false);
		m_Label[VIEW_RIGHT]->setOverrideColor(irr::video::SColor(255, 0, 0, 255));
		m_Label[VIEW_RIGHT]->setVisible(false);

		m_View[VIEW_3D] = m_RenderDevice->getSceneManager()->addCameraSceneNode(m_EditorRoot,
			irr::core::vector3df(1000, 100, 0), irr::core::vector3df(0, 0, 0), NID_NOSAVE);
		m_3DCam = static_cast<irr::scene::ISceneNodeAnimatorCameraFPS*>(m_RenderDevice->
			getSceneManager()->getDefaultSceneNodeAnimatorFactory()->
			createSceneNodeAnimator(irr::scene::ESNAT_CAMERA_FPS, nullptr));
		m_View[VIEW_3D]->setPosition(irr::core::vector3df(0, 100, 100));
		m_View[VIEW_3D]->setTarget(irr::core::vector3df(0, 0, 0));
		m_Label[VIEW_3D] = m_RenderDevice->getGUIEnvironment()->addStaticText(
			_("3D").wc_str(), irr::core::recti(10, 10, 100, 30), false);
		m_Label[VIEW_3D]->setOverrideColor(irr::video::SColor(255, 0, 0, 255));
		m_Label[VIEW_3D]->setVisible(false);

		m_Grid[VIEW_FRONT] = new CGridSceneNode(m_EditorRoot, m_RenderDevice->getSceneManager(),
			NID_NOSAVE);
		m_Grid[VIEW_FRONT]->setGridsSize(irr::core::dimension2df(2500.0f, 2500.0f));
		m_Grid[VIEW_FRONT]->getGrid(0).setSpacing(10.0f);
		m_Grid[VIEW_FRONT]->setVisible(false);
		m_Grid[VIEW_FRONT]->setRotation(irr::core::vector3df(90, 0, 0));
		
		m_Grid[VIEW_TOP] = new CGridSceneNode(m_EditorRoot, m_RenderDevice->getSceneManager(),
			NID_NOSAVE);
		m_Grid[VIEW_TOP]->setGridsSize(irr::core::dimension2df(2500.0f, 2500.0f));
		m_Grid[VIEW_TOP]->getGrid(0).setSpacing(10.0f);
		m_Grid[VIEW_TOP]->setVisible(false);
		
		m_Grid[VIEW_RIGHT] = new CGridSceneNode(m_EditorRoot, m_RenderDevice->getSceneManager(),
			NID_NOSAVE);
		m_Grid[VIEW_RIGHT]->setGridsSize(irr::core::dimension2df(2500.0f, 2500.0f));
		m_Grid[VIEW_RIGHT]->getGrid(0).setSpacing(10.0f);
		m_Grid[VIEW_RIGHT]->setVisible(false);
		m_Grid[VIEW_RIGHT]->setRotation(irr::core::vector3df(0, 0, 90));
		
		m_Grid[VIEW_3D] = new CGridSceneNode(m_EditorRoot, m_RenderDevice->getSceneManager(),
			NID_NOSAVE);
		m_Grid[VIEW_3D]->setGridsSize(irr::core::dimension2df(2500.0f, 2500.0f));
		m_Grid[VIEW_3D]->getGrid(0).setSpacing(10.0f);
		m_Grid[VIEW_3D]->setVisible(false);

		if (m_Map)
		{
			m_ExplorerPanel->Clear();
			m_PropertyPanel->Clear();

			m_Map->SetSceneMgr(m_RenderDevice->getSceneManager());

			// we need to load the entities
			m_Map->Load(m_MapRoot, m_ExplorerPanel);

			// build the path links
			BuildPathLinks();
		}

		m_ExplorerPanel->SetSceneManager(m_RenderDevice->getSceneManager());

		// Add the render device to the browser window
		m_Browser->SetRenderDevice(m_RenderDevice);

		// Initialize terrain editor
		m_TerrainEditor = new TerrainEditor(
			m_RenderDevice->getVideoDriver(),
			m_RenderDevice->getSceneManager(),
			m_RenderDevice->getTimer()
		);
		m_TerrainEditor->initialize();
		m_TerrainEditor->setActiveCamera(m_View[VIEW_TOP]); // Default to TOP view
		m_TerrainEditor->setEnabled(false); // Start disabled

		// Initialize terrain toolbar
		m_TerrainToolbar = new TerrainToolbar(this, this);
		if (m_TerrainToolbar)
		{
			m_TerrainToolbar->SetTerrainEditor(m_TerrainEditor);
			// Set up bidirectional reference so terrain editor can update toolbar
			m_TerrainEditor->setToolbar(m_TerrainToolbar);
			m_TerrainToolbar->Hide(); // Start hidden
		}

		m_RefreshTimer.Start(40); // start refreshing the display, 25FPS
		m_Init = true;
		PostSizeEvent();
	}

	// resize the render pipeline
	if (m_RenderDevice)
		m_RenderDevice->getVideoDriver()->OnResize(irr::core::dimension2du(size.x, size.y));
}

void ViewPanel::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	const wxSize& size = GetSize() * GetContentScaleFactor();

	if (m_RenderDevice)
	{
		// clear the entire viewport
		m_RenderDevice->getVideoDriver()->setViewPort(irr::core::recti(
			0, 0, size.x, size.y));
		m_RenderDevice->getVideoDriver()->beginScene(true, true, irr::video::SColor(255, 170, 170, 170),
			m_VideoData);

		// update the camera billboard position
		m_Camera->setPosition(m_View[VIEW_3D]->getPosition());

		// turn off lighting for orthographic views
		for (irr::core::list<irr::scene::ISceneNode*>::ConstIterator child = m_MapRoot->getChildren().begin();
			child != m_MapRoot->getChildren().end(); ++child)
		{
			(*child)->getMaterial(1).setFlag(irr::video::EMF_LIGHTING, false);
			(*child)->getMaterial(1).setFlag(irr::video::EMF_WIREFRAME, true);
			(*child)->getMaterial(1).setFlag(irr::video::EMF_GOURAUD_SHADING, false);
		}

		// draw top-left view (FRONT)
		m_Grid[VIEW_FRONT]->setVisible(true);
		m_Label[VIEW_FRONT]->setVisible(true);
		m_RenderDevice->getVideoDriver()->setViewPort(irr::core::recti(
			0, 0, size.x / 2, size.y / 2));
		m_Ortho[VIEW_FRONT]->resize(irr::core::dimension2di(size.x / 2, size.y / 2));
		m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_FRONT]);
		m_RenderDevice->getGUIEnvironment()->drawAll();
		m_RenderDevice->getSceneManager()->drawAll();
		m_Grid[VIEW_FRONT]->setVisible(false);
		m_Label[VIEW_FRONT]->setVisible(false);

		// draw top-right view (TOP)
		m_Grid[VIEW_TOP]->setVisible(true);
		m_Label[VIEW_TOP]->setVisible(true);
		m_RenderDevice->getVideoDriver()->setViewPort(irr::core::recti(
			size.x / 2, 0, size.x, size.y / 2));
		m_Ortho[VIEW_TOP]->resize(irr::core::dimension2di(size.x / 2, size.y / 2));
		m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_TOP]);
		m_RenderDevice->getGUIEnvironment()->drawAll();
		m_RenderDevice->getSceneManager()->drawAll();
		
		// Render terrain editor brush preview (only in TOP view)
		if (m_TerrainEditor && m_TerrainEditor->isEnabled() && m_ActiveView == VIEW_TOP)
		{
			m_TerrainEditor->render();
		}
		
		m_Grid[VIEW_TOP]->setVisible(false);
		m_Label[VIEW_TOP]->setVisible(false);

		// draw bottom-left view (RIGHT)
		m_Grid[VIEW_RIGHT]->setVisible(true);
		m_Label[VIEW_RIGHT]->setVisible(true);
		m_RenderDevice->getVideoDriver()->setViewPort(irr::core::recti(
			0, size.y / 2, size.x / 2, size.y));
		m_Ortho[VIEW_RIGHT]->resize(irr::core::dimension2di(size.x / 2, size.y / 2));
		m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_RIGHT]);
		m_RenderDevice->getGUIEnvironment()->drawAll();
		m_RenderDevice->getSceneManager()->drawAll();
		m_Grid[VIEW_RIGHT]->setVisible(false);
		m_Label[VIEW_RIGHT]->setVisible(false);


		// turn on lighting
		bool lighting = m_Map->IsLighting();
		for (irr::core::list<irr::scene::ISceneNode*>::ConstIterator child = m_MapRoot->getChildren().begin();
			child != m_MapRoot->getChildren().end(); ++child)
		{
			(*child)->getMaterial(1).setFlag(irr::video::EMF_LIGHTING, lighting);
			(*child)->getMaterial(1).setFlag(irr::video::EMF_WIREFRAME, false);
			(*child)->getMaterial(1).setFlag(irr::video::EMF_GOURAUD_SHADING, true);
		}

		// draw bottom-right view (3D)
		m_Camera->setVisible(false);
		m_Grid[VIEW_3D]->setVisible(true);
		m_Label[VIEW_3D]->setVisible(true);
		m_RenderDevice->getVideoDriver()->setViewPort(irr::core::recti(
			size.x / 2, size.y / 2, size.x, size.y));
		m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_3D]);
		m_RenderDevice->getGUIEnvironment()->drawAll();
		m_RenderDevice->getSceneManager()->drawAll();
		
		// Note: Terrain editor brush preview will be rendered in TOP view instead
		
		m_Grid[VIEW_3D]->setVisible(false);
		m_Label[VIEW_3D]->setVisible(false);
		m_Camera->setVisible(true);


		// draw the dividing lines
		m_RenderDevice->getVideoDriver()->setViewPort(irr::core::recti(
			0, 0, size.x, size.y));
		m_RenderDevice->getVideoDriver()->draw2DLine(
			irr::core::vector2di(0, size.y / 2), irr::core::vector2di(size.x, size.y / 2));
		m_RenderDevice->getVideoDriver()->draw2DLine(
			irr::core::vector2di(size.x / 2, 0), irr::core::vector2di(size.x / 2, size.y));
		m_RenderDevice->getVideoDriver()->endScene();
	}
}

void ViewPanel::OnMouse(wxMouseEvent& event)
{
	irr::SEvent irrEvent;
	irrEvent.EventType = irr::EET_MOUSE_INPUT_EVENT;

	if (m_RenderDevice == nullptr)
	{
		event.Skip();
		return;
	}

	if (!HasCapture() || !m_FreeLook)
	{
		m_RenderDevice->getCursorControl()->setReferenceRect(nullptr);
		m_RenderDevice->getCursorControl()->setVisible(true);
	}

	// determine which viewport the mouse is in
	irr::core::vector2di cursor(event.GetX(), event.GetY());
	const wxSize& size = GetSize() * GetContentScaleFactor();

	if (!m_FreeLook)
	{
		if (cursor.X < size.x / 2) // left side
		{
			if (cursor.Y < size.y / 2)
			{
				// top-left
				m_ActiveView = VIEW_FRONT;
				m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_FRONT]);
			}
			else
			{
				// bottom-left
				m_ActiveView = VIEW_RIGHT;
				m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_RIGHT]);
			}
		}
		else // right side
		{
			if (cursor.Y < size.y / 2)
			{
				// top-right
				m_ActiveView = VIEW_TOP;
				m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_TOP]);
			}
			else
			{
				// bottom-right
				m_ActiveView = VIEW_3D;
				m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_3D]);
			}
		}

		// transform the cursor position
		switch (m_ActiveView)
		{
		case VIEW_FRONT: // top-left
		{
			cursor.X = (cursor.X / (irr::f32)(size.GetWidth() / 2)) * size.GetWidth();
			cursor.Y = (cursor.Y / (irr::f32)(size.GetHeight() / 2)) * size.GetHeight();
		} break;
		case VIEW_TOP: // top-right
		{
			cursor.X = ((cursor.X - (size.GetWidth() / 2.0f)) / (irr::f32)(size.GetWidth() / 2)) * size.GetWidth();
			cursor.Y = (cursor.Y / (irr::f32)(size.GetHeight() / 2)) * size.GetHeight();
		} break;
		case VIEW_RIGHT: // bottom-left
		{
			cursor.X = (cursor.X / (irr::f32)(size.GetWidth() / 2)) * size.GetWidth();
			cursor.Y = ((cursor.Y - (size.GetHeight() / 2.0f)) / (irr::f32)(size.GetHeight() / 2)) * size.GetHeight();
		} break;
		case VIEW_3D: // bottom-right
		{
			cursor.X = ((cursor.X - (size.GetWidth() / 2.0f)) / (irr::f32)(size.GetWidth() / 2)) * size.GetWidth();
			cursor.Y = ((cursor.Y - (size.GetHeight() / 2.0f)) / (irr::f32)(size.GetHeight() / 2)) * size.GetHeight();
		} break;
		}
	}

	irrEvent.MouseInput.X = cursor.X;
	irrEvent.MouseInput.Y = cursor.Y;

	// Handle terrain editing if enabled and we're in TOP view
	if (m_TerrainEditor && m_TerrainEditor->isEnabled() && 
		m_ActiveView == VIEW_TOP && m_ActiveTerrain)
	{
		// Ensure terrain editor has the correct camera reference
		m_TerrainEditor->setActiveCamera(m_View[VIEW_TOP]);
		
		// Set the correct viewport for the TOP view so getRayFromScreenCoordinates works correctly
		const wxSize& size = GetSize() * GetContentScaleFactor();
		m_RenderDevice->getVideoDriver()->setViewPort(irr::core::recti(
			size.x / 2, 0, size.x, size.y / 2));
		m_RenderDevice->getSceneManager()->setActiveCamera(m_View[VIEW_TOP]);
		
		// Calculate coordinates relative to the TOP viewport (top-right quarter)
		irr::s32 viewportMouseX = ((event.GetX() - (size.GetWidth() / 2.0f)) / (irr::f32)(size.GetWidth() / 2)) * (size.GetWidth() / 2);
		irr::s32 viewportMouseY = (event.GetY() / (irr::f32)(size.GetHeight() / 2)) * (size.GetHeight() / 2);
		
		// Create a transformed mouse event with viewport-relative coordinates
		wxMouseEvent transformedEvent = event;
		transformedEvent.m_x = viewportMouseX;
		transformedEvent.m_y = viewportMouseY;
		
		bool terrainHandled = m_TerrainEditor->onMouseEvent(transformedEvent);
		
		// Restore full viewport
		m_RenderDevice->getVideoDriver()->setViewPort(irr::core::recti(
			0, 0, size.x, size.y));
		
		if (terrainHandled)
		{
			// Terrain editor handled the event, skip normal processing
			event.Skip();
			return;
		}
	}

	// generate a ray for the mouse cursor
	irr::scene::ISceneCollisionManager* colMgr =
		m_RenderDevice->getSceneManager()->getSceneCollisionManager();
	irr::core::line3df mouseRay = colMgr->getRayFromScreenCoordinates(
		cursor, m_View[m_ActiveView]);

	do
	{
		wxEventType type = event.GetEventType();
		if (type == wxEVT_MOTION)
		{
			irrEvent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;

			if (event.Dragging() && m_ActiveView != VIEW_3D)
			{
				// we're in an ortho view and dragging the mouse
				if (event.LeftIsDown())
				{
					wxPoint delta(cursor.X - m_LastMousePos.x,
						cursor.Y - m_LastMousePos.y);

					if (m_TranslatingSelection)
					{
						// convert the delta to the active view
						irr::core::vector3df translate = m_Ortho[m_ActiveView]->transformPoint(
							delta.x, delta.y);

						// update the translate command
						TranslateNodeCommand* cmd = dynamic_cast<TranslateNodeCommand*>(
							m_Commands.GetCurrentCommand());
						if (cmd)
							cmd->Update(translate);

						m_PropertyPanel->Refresh();
					}
				}
			}
		}
		else if (type == wxEVT_LEFT_DOWN)
		{
			irrEvent.MouseInput.Event = irr::EMIE_LMOUSE_PRESSED_DOWN;

			if (m_Selection.size() > 0
				&& m_SelectionBox.intersectsWithLine(mouseRay))
			{
				if (!event.ControlDown() && !m_TranslatingSelection) // translating
				{
					m_TranslatingSelection = true;

					// convert the selection
					TranslateNodeCommand::selection_t selection;
					for (selection_t::iterator node = m_Selection.begin();
						node != m_Selection.end(); ++node)
					{
						selection.push_back(wxString((*node)->getName()));
					}

					m_Commands.Store(new TranslateNodeCommand(m_RenderDevice->getSceneManager(),
						selection, m_SelectionBox.MinEdge));

					SetCursor(*m_Cursor[CURSOR_MOVE]);
				}
			}
		}
		else if (type == wxEVT_MIDDLE_DOWN)
		{
			irrEvent.MouseInput.Event = irr::EMIE_MMOUSE_PRESSED_DOWN;
		}
		else if (type == wxEVT_RIGHT_DOWN)
		{
			irrEvent.MouseInput.Event = irr::EMIE_RMOUSE_PRESSED_DOWN;
		}
		else if (type == wxEVT_LEFT_UP)
		{
			irrEvent.MouseInput.Event = irr::EMIE_LMOUSE_LEFT_UP;

			if (m_TranslatingSelection)
			{
				m_TranslatingSelection = false;

				// update the selection aabb
				UpdateSelectionBoundingBox();

				// update the property panel, if appropriate
				if (m_Selection.size() == 1)
					m_PropertyPanel->Refresh();

				SetCursor(wxNullCursor);
			}

			// check if other mouse buttons are down - we are likely moving the camera
			if (!event.ButtonIsDown(wxMOUSE_BTN_MIDDLE) &&
				!event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
			{
				// try to pick an object
				irr::core::vector3df intersection;
				irr::core::triangle3df hitTriangle;
				irr::scene::ISceneNode* selection = colMgr->getSceneNodeAndCollisionPointFromRay(
					mouseRay, intersection, hitTriangle, NID_PICKABLE, m_MapRoot);
				if (selection)
				{
					// are we multi-selecting?
					AddToSelection(selection, event.ShiftDown());
				}
			}
		}
		else if (type == wxEVT_MIDDLE_UP)
		{
			irrEvent.MouseInput.Event = irr::EMIE_MMOUSE_LEFT_UP;
			
			// lock/unlock free look
			if (m_FreeLook)
				EndFreeLook();
			else
			{
				// only activate free look in the 3D view window
				if (m_ActiveView == VIEW_3D)
					BeginFreeLook();
			}
		}
		else if (type == wxEVT_RIGHT_UP)
		{
			irrEvent.MouseInput.Event = irr::EMIE_RMOUSE_LEFT_UP;

			// popup menu
			wxMenu popupMenu;
			popupMenu.Append(wxID_CUT);
			popupMenu.Append(wxID_COPY);
			popupMenu.Append(wxID_PASTE);
			popupMenu.Append(wxID_DELETE);
			popupMenu.AppendSeparator();

			popupMenu.Append(TOOL_PLAYERSTART, _("Add player start"));
			popupMenu.Append(TOOL_LIGHT, _("Add light"));
			popupMenu.Append(TOOL_PATHNODE, _("Add path node"));

			const wxString& actor = m_Browser->GetActor();
			if (!actor.empty())
				popupMenu.Append(TOOL_ACTOR, wxString::Format(_("Add actor: %s"), actor));

			const wxString& mesh = m_Browser->GetMesh();
			if (!mesh.empty())
			{
				wxFileName meshName(mesh);
				popupMenu.Append(TOOL_MESH, wxString::Format(_("Add mesh: %s"), meshName.GetName()));
			}

			if (m_ActiveView == VIEW_3D)
			{
				popupMenu.AppendSeparator();
				popupMenu.Append(MENU_FREELOOK, !m_FreeLook ? _("Begin free look") : _("End free look"));
				popupMenu.AppendSeparator();
			}

			const wxString& texture = m_Browser->GetTexture();
			if (!texture.empty())
				popupMenu.Append(MENU_SETTEXTURE, wxString::Format(_("Apply texture: %s"),
					texture));

			PopupMenu(&popupMenu);
		}
		else if (type == wxEVT_MOUSEWHEEL)
		{
			irrEvent.MouseInput.Event = irr::EMIE_MOUSE_WHEEL;
			if (event.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
				irrEvent.MouseInput.Wheel = event.GetWheelRotation() / event.GetWheelDelta();
		}

		m_RenderDevice->postEventFromUser(irrEvent);
	} while (false);

	m_LastMousePos.x = cursor.X;
	m_LastMousePos.y = cursor.Y;

	event.Skip();
}

void ViewPanel::OnKey(wxKeyEvent& event)
{
	// Handle terrain editing keys first if enabled
	if (m_TerrainEditor && m_TerrainEditor->isEnabled())
	{
		if (m_TerrainEditor->onKeyEvent(event))
		{
			// Terrain editor handled the event
			event.Skip();
			return;
		}
	}

	if (event.GetEventType() == wxEVT_KEY_DOWN)
	{
	}
	else if (event.GetEventType() == wxEVT_KEY_UP)
	{
		// switch (event.GetKeyCode())
		switch (event.GetUnicodeKey())
		{
		case WXK_ESCAPE:
			if (m_FreeLook)
				EndFreeLook();
			else
			{
				ClearSelection();
				// Exit terrain editing mode on Escape
				if (m_TerrainEditingMode)
					SetTerrainEditingMode(false);
			}
			break;
		case WXK_DELETE:
			DeleteSelection();
			break;
		// case 'T':
		// case 't':
		case wxT('T'):
		case wxT('t'):
			// Toggle terrain editing mode with 'T' key
			if (m_ActiveTerrain) // Only allow if terrain is available
			{
				SetTerrainEditingMode(!m_TerrainEditingMode);
				wxLogMessage(m_TerrainEditingMode ? 
					_("Terrain editing mode enabled - use TOP view (top-right) to edit") : 
					_("Terrain editing mode disabled"));
			}
			else
			{
				wxLogMessage(_("Select a terrain node to enable terrain editing"));
			}
			break;
		}
	}

	if (m_FreeLook)
	{
		// submit the key to the FPS camera
		irr::SEvent irrEvent;
		irrEvent.EventType = irr::EET_KEY_INPUT_EVENT;
		irrEvent.KeyInput.Char = 0;
		irrEvent.KeyInput.PressedDown = (event.GetEventType() == wxEVT_KEY_DOWN);
		irrEvent.KeyInput.Control = false;
		irrEvent.KeyInput.Shift = false;

		switch (event.GetKeyCode())
		{
		case 87: // 'w'
			irrEvent.KeyInput.Key = irr::KEY_UP;
			break;
		case 68: // 'd'
			irrEvent.KeyInput.Key = irr::KEY_RIGHT;
			break;
		case 83: // 's'
			irrEvent.KeyInput.Key = irr::KEY_DOWN;
			break;
		case 65: // 'a'
			irrEvent.KeyInput.Key = irr::KEY_LEFT;
			break;
		default:
			irrEvent.KeyInput.Key = irr::KEY_NONE;
			break;
		}

		if (m_RenderDevice)
			m_RenderDevice->postEventFromUser(irrEvent);
	}

	event.Skip(); // keep this moving
}

void ViewPanel::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	if (m_FreeLook)
		EndFreeLook();
}

void ViewPanel::OnMouseCaptureChanged(wxMouseCaptureChangedEvent& event)
{
	if (m_FreeLook)
		EndFreeLook();
}

void ViewPanel::BuildPathLinks(void)
{
	if (m_RenderDevice == nullptr)
		return;

	irr::core::array<irr::scene::ISceneNode*> nodes;
	m_RenderDevice->getSceneManager()->getSceneNodesFromType(
		(irr::scene::ESCENE_NODE_TYPE)ESNT_PATHNODE,
		nodes, nullptr);

	irr::u32 count = nodes.size();
	for (irr::u32 i = 0; i < count; ++i)
	{
		PathSceneNode* pathNode = dynamic_cast<PathSceneNode*>(nodes[i]);
		pathNode->drawLink(true);
	}
}

void ViewPanel::OnToolCube(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();

	m_Commands.Submit(new AddNodeCommand(TOOL_CUBE, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot,
		m_Map, location, m_Map->NextName("cube")));
}

void ViewPanel::OnToolCylinder(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();

	m_Commands.Submit(new AddNodeCommand(TOOL_CYLINDER, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot,
		m_Map, location, m_Map->NextName("cylinder")));
}

void ViewPanel::OnToolSphere(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();

	m_Commands.Submit(new AddNodeCommand(TOOL_SPHERE, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot, 
		m_Map, location, m_Map->NextName("sphere")));
}

void ViewPanel::OnToolPlane(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();

	m_Commands.Submit(new AddNodeCommand(TOOL_PLANE, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot, 
		m_Map, location, m_Map->NextName("plane")));
}

void ViewPanel::OnToolTerrain(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();
	location.Y = m_Grid[VIEW_3D]->getPosition().Y + 0.5f;

	m_Commands.Submit(new AddNodeCommand(TOOL_TERRAIN, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot, 
		m_Map, location, m_Map->NextName("terrain")));
}

void ViewPanel::OnToolSkybox(wxCommandEvent& event)
{
	m_Commands.Submit(new AddNodeCommand(TOOL_SKYBOX, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot, m_Map,
		irr::core::vector3df(0, 0, 0), m_Map->NextName("skybox")));
}

void ViewPanel::OnToolPlayerStart(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();
	m_Commands.Submit(new AddNodeCommand(TOOL_PLAYERSTART, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot, m_Map,
		location, m_Map->NextName(wxT("playerstart"))));
}

void ViewPanel::OnToolLight(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();
	m_Commands.Submit(new AddNodeCommand(TOOL_LIGHT, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot, m_Map,
		location, m_Map->NextName(wxT("light"))));
}

void ViewPanel::OnToolPathNode(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();
	m_Commands.Submit(new AddNodeCommand(TOOL_PATHNODE, m_ExplorerPanel,
		m_RenderDevice->getSceneManager(), m_MapRoot, m_Map,
		location, m_Map->NextName(wxT("pathnode"))));
}

void ViewPanel::OnToolActor(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();
	m_Commands.Submit(new AddNodeCommand(TOOL_ACTOR, 
		m_ExplorerPanel, m_RenderDevice->getSceneManager(), m_MapRoot,
		m_Map, location, m_Browser->GetActor()));
}

void ViewPanel::OnToolMesh(wxCommandEvent& event)
{
	// get the 3D camera and create the item directly in front of it
	irr::core::vector3df pos = m_View[VIEW_3D]->getAbsolutePosition();
	irr::core::vector3df target = m_View[VIEW_3D]->getTarget();
	irr::core::line3df ray(pos, target);

	irr::core::vector3df location = ray.getMiddle();

	m_Commands.Submit(new AddNodeCommand(TOOL_MESH, 
		m_ExplorerPanel, m_RenderDevice->getSceneManager(), m_MapRoot,
		m_Map, location, m_Browser->GetMeshDefinition()));
}

void ViewPanel::OnEditCut(wxCommandEvent& event)
{
	wxLogMessage(_("Not implemented"));
}

void ViewPanel::OnEditCopy(wxCommandEvent& event)
{
	wxLogMessage(_("Not implemented"));
}

void ViewPanel::OnEditPaste(wxCommandEvent& event)
{
	wxLogMessage(_("Not implemented"));
}

void ViewPanel::OnEditDelete(wxCommandEvent& event)
{
	DeleteSelection();
}

void ViewPanel::OnMenuTerrainEdit(wxCommandEvent& event)
{
	SetTerrainEditingMode(!m_TerrainEditingMode);
}

void ViewPanel::OnMenuAlignTop(wxCommandEvent& event)
{
	if (m_Selection.size() > 1)
	{
		AlignNodeCommand::selection_t selection;
		for (selection_t::iterator node = m_Selection.begin();
			node != m_Selection.end(); ++node)
		{
			selection.push_back(wxString((*node)->getName()));
		}

		m_Commands.Submit(new AlignNodeCommand(m_RenderDevice->getSceneManager(),
			selection, AlignNodeCommand::ALIGN_TOP));
	}
}

void ViewPanel::OnMenuAlignMiddle(wxCommandEvent& event)
{
	if (m_Selection.size() > 1)
	{
		AlignNodeCommand::selection_t selection;
		for (selection_t::iterator node = m_Selection.begin();
			node != m_Selection.end(); ++node)
		{
			selection.push_back(wxString((*node)->getName()));
		}

		m_Commands.Submit(new AlignNodeCommand(m_RenderDevice->getSceneManager(),
			selection, AlignNodeCommand::ALIGN_MIDDLE));
	}
}

void ViewPanel::OnMenuAlignBottom(wxCommandEvent& event)
{
	if (m_Selection.size() > 1)
	{
		AlignNodeCommand::selection_t selection;
		for (selection_t::iterator node = m_Selection.begin();
			node != m_Selection.end(); ++node)
		{
			selection.push_back(wxString((*node)->getName()));
		}

		m_Commands.Submit(new AlignNodeCommand(m_RenderDevice->getSceneManager(),
			selection, AlignNodeCommand::ALIGN_BOTTOM));
	}
}

void ViewPanel::OnMenuFreeLook(wxCommandEvent& event)
{
	if (!m_FreeLook)
		BeginFreeLook();
	else
		EndFreeLook();
}

void ViewPanel::OnMenuSetTexture(wxCommandEvent& event)
{
	ChangeTextureCommand::selection_t selection;
	for (selection_t::iterator node = m_Selection.begin();
		node != m_Selection.end(); ++node)
	{
		selection.push_back(wxString((*node)->getName()));
	}

	m_Commands.Submit(new ChangeTextureCommand(m_RenderDevice->getSceneManager(),
		selection, 1, 1, m_Browser->GetTexture()));
}

void ViewPanel::SetTerrainEditingMode(bool enabled)
{
	m_TerrainEditingMode = enabled;
	
	if (m_TerrainEditor)
	{
		m_TerrainEditor->setEnabled(enabled);
		
		// Update camera reference when mode changes
		if (enabled)
		{
			m_TerrainEditor->setActiveCamera(m_View[VIEW_TOP]); // Always use TOP view
			
			// Auto-detect terrain if one is selected
			if (m_Selection.size() == 1)
			{
				UpdatableTerrainSceneNode* terrain = dynamic_cast<UpdatableTerrainSceneNode*>(m_Selection.front());
				if (terrain)
				{
					m_ActiveTerrain = terrain;
					m_TerrainEditor->setTerrain(terrain);
				}
			}
		}
		else
		{
			// Clear active terrain when exiting terrain mode
			m_ActiveTerrain = nullptr;
			m_TerrainEditor->setTerrain(nullptr);
		}
	}

	// Update the toolbar if it exists
	if (m_TerrainToolbar)
	{
		m_TerrainToolbar->UpdateFromTerrainEditor();
		
		if (enabled)
			ShowTerrainToolbar();
		else
			HideTerrainToolbar();
	}
}

void ViewPanel::ShowTerrainToolbar()
{
	if (m_TerrainToolbar)
	{
		m_TerrainToolbar->Show();
		m_TerrainToolbar->Raise();
		
		// Force a refresh to make sure the window appears
		m_TerrainToolbar->Update();
	}
}

void ViewPanel::HideTerrainToolbar()
{
	if (m_TerrainToolbar)
		m_TerrainToolbar->Hide();
}

bool ViewPanel::IsTerrainToolbarVisible() const
{
	return m_TerrainToolbar && m_TerrainToolbar->IsShown();
}
