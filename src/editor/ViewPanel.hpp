/**
 * @file ViewPanel.hpp
 * @brief 3D view panel implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include "BrowserWindow.hpp"
#include "ExplorerPanel.hpp"
#include "Map.hpp"
#include "PropertyPanel.hpp"

#include <wx/cmdproc.h>
#include <wx/cursor.h>
#include <wx/panel.h>
#include <wx/timer.h>

#include "irrlicht.h"
#include "CGridSceneNode.h"
#include "CSceneNodeAnimatorCameraOrtho.h"

#include <list>
#include <memory>

/**
 * @class ViewPanel
 * @brief Panel class for 3D view and scene manipulation
 * 
 * The ViewPanel class provides a 3D view of the map and handles scene
 * manipulation, object selection, and camera controls. It supports multiple
 * view types including front, top, right, and 3D views.
 */
class ViewPanel : public wxPanel
{
	friend class PropertyPanel;

private:
	/**
	 * @enum VIEW
	 * @brief Enumeration of supported view types
	 */
	enum VIEW
	{
		VIEW_FRONT,    ///< Front view (top-left)
		VIEW_TOP,      ///< Top view (top-right)
		VIEW_RIGHT,    ///< Right view (bottom-left)
		VIEW_3D        ///< 3D view (bottom-right)
	};

	/**
	 * @enum CURSORS
	 * @brief Enumeration of cursor types
	 */
	enum CURSORS
	{
		CURSOR_MOVE,   ///< Move cursor
		CURSOR_ROTATE, ///< Rotate cursor
		NUM_CURSORS    ///< Number of cursor types
	};

private:
	wxTimer m_RefreshTimer;                         ///< Timer for view refresh
	wxCommandProcessor& m_Commands;                 ///< Command processor for undo/redo
	BrowserWindow* m_Browser;                       ///< Browser window for resources
	ExplorerPanel* m_ExplorerPanel;                 ///< Explorer panel for scene hierarchy
	PropertyPanel* m_PropertyPanel;                 ///< Property panel for object properties
	wxCursor* m_Cursor[NUM_CURSORS];               ///< Array of cursors

	irr::IrrlichtDevice* m_RenderDevice;           ///< Irrlicht rendering device
	irr::video::SExposedVideoData m_VideoData;     ///< Video data for rendering

	bool m_Init;                                    ///< Initialization flag
	VIEW m_ActiveView;                             ///< Currently active view
	bool m_FreeLook;                               ///< Free look mode flag

	irr::scene::ISceneNode* m_EditorRoot;          ///< Root node for editor objects
	irr::scene::ISceneNode* m_MapRoot;             ///< Root node for map objects
	irr::scene::IBillboardSceneNode* m_Camera;     ///< Camera billboard

	irr::scene::ICameraSceneNode* m_View[4];       ///< Camera nodes for each view
	irr::scene::CSceneNodeAnimatorCameraOrtho* m_Ortho[3]; ///< Orthographic camera animators
	irr::scene::ISceneNodeAnimatorCameraFPS* m_3DCam;      ///< FPS camera animator
	CGridSceneNode* m_Grid[4];                     ///< Grid nodes for each view
	irr::gui::IGUIStaticText* m_Label[4];          ///< View labels

	std::shared_ptr<Map> m_Map;                    ///< The current map
	
	typedef std::list<irr::scene::ISceneNode*> selection_t; ///< Type for node selection
	selection_t m_Selection;                       ///< Currently selected nodes
	irr::core::aabbox3df m_SelectionBox;          ///< Bounding box of selection

	wxPoint m_LastMousePos;                        ///< Last mouse position
	bool m_TranslatingSelection;                   ///< Selection translation flag

public:
	/**
	 * @brief Constructor for the ViewPanel class
	 * @param parent Pointer to the parent window
	 * @param cmdProc Reference to the command processor
	 * @param browserWindow Pointer to the browser window
	 * @param explorerPanel Pointer to the explorer panel
	 * @param propertyPanel Pointer to the property panel
	 */
	ViewPanel(wxWindow* parent, wxCommandProcessor& cmdProc,
		BrowserWindow* browserWindow,
		ExplorerPanel* explorerPanel, PropertyPanel* propertyPanel);
	~ViewPanel(void);

	/**
	 * @brief Get the file system
	 * @return Pointer to the Irrlicht file system
	 */
	irr::io::IFileSystem* GetFileSystem(void);

	/**
	 * @brief Get the current FPS
	 * @return Current frames per second
	 */
	int GetFPS(void);

	/**
	 * @brief Set the current map
	 * @param map Shared pointer to the map
	 */
	void SetMap(std::shared_ptr<Map>& map);

	/**
	 * @brief Add a node to the selection
	 * @param node Pointer to the node to add
	 * @param append Whether to append to existing selection
	 */
	void AddToSelection(irr::scene::ISceneNode* node, bool append);

	/**
	 * @brief Update the selection bounding box
	 */
	void UpdateSelectionBoundingBox(void);

	/**
	 * @brief Show or hide the selection
	 * @param show Whether to show the selection
	 */
	void ShowSelection(bool show); // used when saving the map

	/**
	 * @brief Clear the current selection
	 */
	void ClearSelection(void);

	/**
	 * @brief Delete the selected nodes
	 */
	void DeleteSelection(void);

	/**
	 * @brief Begin free look mode
	 */
	void BeginFreeLook(void);

	/**
	 * @brief End free look mode
	 */
	void EndFreeLook(void);

private:
	/**
	 * @brief Handle timer events
	 * @param event The timer event
	 */
	void OnTimer(wxTimerEvent& event);

	/**
	 * @brief Handle resize events
	 * @param event The size event
	 */
	void OnResize(wxSizeEvent& event);

	/**
	 * @brief Handle paint events
	 * @param event The paint event
	 */
	void OnPaint(wxPaintEvent& event);

	/**
	 * @brief Handle mouse events
	 * @param event The mouse event
	 */
	void OnMouse(wxMouseEvent& event);

	/**
	 * @brief Handle key events
	 * @param event The key event
	 */
	void OnKey(wxKeyEvent& event);

	/**
	 * @brief Handle mouse capture lost events
	 * @param event The mouse capture lost event
	 */
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);

	/**
	 * @brief Handle mouse capture changed events
	 * @param event The mouse capture changed event
	 */
	void OnMouseCaptureChanged(wxMouseCaptureChangedEvent& event);

	/**
	 * @brief Build path links between nodes
	 */
	void BuildPathLinks(void);

public:
	/**
	 * @brief Handle cube tool action
	 * @param event The command event
	 */
	void OnToolCube(wxCommandEvent& event);

	/**
	 * @brief Handle cylinder tool action
	 * @param event The command event
	 */
	void OnToolCylinder(wxCommandEvent& event);

	/**
	 * @brief Handle sphere tool action
	 * @param event The command event
	 */
	void OnToolSphere(wxCommandEvent& event);

	/**
	 * @brief Handle plane tool action
	 * @param event The command event
	 */
	void OnToolPlane(wxCommandEvent& event);

	/**
	 * @brief Handle terrain tool action
	 * @param event The command event
	 */
	void OnToolTerrain(wxCommandEvent& event);

	/**
	 * @brief Handle skybox tool action
	 * @param event The command event
	 */
	void OnToolSkybox(wxCommandEvent& event);

	/**
	 * @brief Handle player start tool action
	 * @param event The command event
	 */
	void OnToolPlayerStart(wxCommandEvent& event);

	/**
	 * @brief Handle light tool action
	 * @param event The command event
	 */
	void OnToolLight(wxCommandEvent& event);

	/**
	 * @brief Handle path node tool action
	 * @param event The command event
	 */
	void OnToolPathNode(wxCommandEvent& event);

	/**
	 * @brief Handle actor tool action
	 * @param event The command event
	 */
	void OnToolActor(wxCommandEvent& event);

	/**
	 * @brief Handle cut action
	 * @param event The command event
	 */
	void OnEditCut(wxCommandEvent& event);

	/**
	 * @brief Handle copy action
	 * @param event The command event
	 */
	void OnEditCopy(wxCommandEvent& event);

	/**
	 * @brief Handle paste action
	 * @param event The command event
	 */
	void OnEditPaste(wxCommandEvent& event);

	/**
	 * @brief Handle delete action
	 * @param event The command event
	 */
	void OnEditDelete(wxCommandEvent& event);

	/**
	 * @brief Handle align top action
	 * @param event The command event
	 */
	void OnMenuAlignTop(wxCommandEvent& event);

	/**
	 * @brief Handle align middle action
	 * @param event The command event
	 */
	void OnMenuAlignMiddle(wxCommandEvent& event);

	/**
	 * @brief Handle align bottom action
	 * @param event The command event
	 */
	void OnMenuAlignBottom(wxCommandEvent& event);

	/**
	 * @brief Handle free look action
	 * @param event The command event
	 */
	void OnMenuFreeLook(wxCommandEvent& event);

	/**
	 * @brief Handle set texture action
	 * @param event The command event
	 */
	void OnMenuSetTexture(wxCommandEvent& event);
};
