/**
 * @file ExplorerPanel.hpp
 * @brief Scene explorer panel implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include "BrowserWindow.hpp"
#include "ViewPanel.hpp"

#include <wx/cmdproc.h>
#include <wx/panel.h>
#include <wx/treectrl.h>

#include "irrlicht.h"

class ViewPanel;

/**
 * @class ExplorerPanel
 * @brief Panel class for scene hierarchy and object management
 * 
 * The ExplorerPanel class provides a tree view panel for managing scene
 * objects and their hierarchy. It supports object selection, property
 * editing, and scene organization.
 */
class ExplorerPanel : public wxPanel
{
private:
	wxCommandProcessor& m_Commands;
	BrowserWindow* m_Browser;
	ViewPanel* m_ViewPanel;

	wxTreeCtrl* m_Explorer;
	wxTreeItemId m_Root;
	wxTreeItemId m_GeometryRoot;
	wxTreeItemId m_ActorRoot;

	irr::scene::ISceneManager* m_SceneMgr;

	bool m_Changing;

public:
	/**
	 * @brief Constructor for the ExplorerPanel class
	 * @param parent Pointer to the parent window
	 * @param cmdProc Reference to the command processor
	 * @param browser Pointer to the browser window
	 */
	ExplorerPanel(wxWindow* parent, wxCommandProcessor& cmdProc,
		BrowserWindow* browser);

	/**
	 * @brief Destructor
	 */
	~ExplorerPanel(void);

	/**
	 * @brief Set the view panel
	 * @param viewPanel Pointer to the view panel
	 */
	void SetViewPanel(ViewPanel* viewPanel);

	/**
	 * @brief Get the browser
	 * @return Pointer to the browser window
	 */
	BrowserWindow* GetBrowser(void);

	/**
	 * @brief Set the scene manager
	 * @param sceneMgr Pointer to the scene manager
	 */
	void SetSceneManager(irr::scene::ISceneManager* sceneMgr);

	/**
	 * @brief Set the map name
	 * @param name The name of the map
	 */
	void SetMapName(const wxString& name);

	/**
	 * @brief Clear the explorer
	 */
	void Clear(void);

	/**
	 * @brief Select an item
	 * @param name The name of the item to select
	 */
	void SelectItem(const wxString& name);

	/**
	 * @brief Unselect an item
	 * @param name The name of the item to unselect
	 */
	void UnselectItem(const wxString& name);

	/**
	 * @brief Unselect all items
	 */
	void UnselectAll(void);

	/**
	 * @brief Add a geometry node
	 * @param name The name of the geometry
	 */
	void AddGeometry(const wxString& name);

	/**
	 * @brief Remove a geometry node
	 * @param name The name of the geometry
	 */
	void RemoveGeometry(const wxString& name);

	/**
	 * @brief Check if a node is a geometry node
	 * @param name The name of the node
	 * @return True if the node is a geometry node, false otherwise
	 */
	bool IsGeometry(const wxString& name);

	/**
	 * @brief Add an actor node
	 * @param name The name of the actor
	 */
	void AddActor(const wxString& name);

	/**
	 * @brief Remove an actor node
	 * @param name The name of the actor
	 */
	void RemoveActor(const wxString& name);

	/**
	 * @brief Check if a node is an actor node
	 * @param name The name of the node
	 * @return True if the node is an actor node, false otherwise
	 */
	bool IsActor(const wxString& name);

private:
	/**
	 * @brief Find an item in the tree
	 * @param name The name of the item to find
	 * @param start The starting tree item ID
	 * @return The tree item ID for the found item
	 */
	wxTreeItemId FindItem(const wxString& name, wxTreeItemId& start);

	/**
	 * @brief Handle tree item selection changed events
	 * @param event The tree event
	 */
	void OnSelectionChanged(wxTreeEvent& event);

	/**
	 * @brief Handle tree item right-click events
	 * @param event The tree event
	 */
	void OnItemRightClick(wxTreeEvent& event);
};
