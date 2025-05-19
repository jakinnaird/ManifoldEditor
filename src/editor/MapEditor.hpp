/**
 * @file MapEditor.hpp
 * @brief Map editor implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include "Editor.hpp"
#include "ExplorerPanel.hpp"
#include "Map.hpp"
#include "PlayProcess.hpp"
#include "PropertyPanel.hpp"
#include "ViewPanel.hpp"

#include <wx/aui/aui.h>
#include <wx/cmdproc.h>
#include <wx/filename.h>

#include <memory>

/**
 * @class MapEditor
 * @brief Editor class for creating and editing maps
 * 
 * The MapEditor class provides functionality for creating and editing maps
 * in the Manifold Editor. It includes features for 3D view manipulation,
 * object placement, and map property editing.
 */
class MapEditor : public Editor
{
private:
	wxAuiManager m_AuiMgr;              ///< AUI manager for window layout
	wxCommandProcessor m_Commands;       ///< Command processor for undo/redo

	ViewPanel* m_ViewPanel;             ///< Panel for 3D view
	ExplorerPanel* m_ExplorerPanel;     ///< Panel for scene hierarchy
	PropertyPanel* m_PropertyPanel;     ///< Panel for property editing

	wxFileName m_FileName;              ///< Current map file
	std::shared_ptr<Map> m_Map;         ///< The current map data
	PlayProcess* m_PlayMapProcess;      ///< Process for playing the map

public:
	/**
	 * @brief Constructor for the MapEditor class
	 * @param parent Pointer to the parent window
	 * @param editMenu Pointer to the edit menu
	 * @param browserWindow Pointer to the browser window
	 * @param mapName Name of the map file
	 */
	MapEditor(MainWindow* parent, wxMenu* editMenu, 
		BrowserWindow* browserWindow, const wxFileName& mapName);
	~MapEditor(void);

	/**
	 * @brief Get the view panel
	 * @return Pointer to the view panel
	 */
	ViewPanel* GetViewPanel(void) { return m_ViewPanel; }

	/**
	 * @brief Handle play process termination
	 */
	void PlayProcessTerminated(void);

	/**
	 * @brief Get the current FPS
	 * @return Current frames per second
	 */
	int GetFPS(void);

	/**
	 * @brief Load a map from file
	 * @param filePath Path to the map file
	 */
	void Load(const wxFileName& filePath);

	/**
	 * @brief Check if the map has unsaved changes
	 * @return true if there are unsaved changes, false otherwise
	 */
	bool HasChanged(void);

	/**
	 * @brief Undo the last action
	 */
	void OnUndo(void);

	/**
	 * @brief Redo the last undone action
	 */
	void OnRedo(void);

	/**
	 * @brief Save the current map
	 * @param allFiles Whether to save all open files
	 * @return true if save was successful, false otherwise
	 */
	bool OnSave(bool allFiles = false);

	/**
	 * @brief Save the current map to a new file
	 * @return true if save was successful, false otherwise
	 */
	bool OnSaveAs(void);

	/**
	 * @brief Cut the selected content
	 */
	void OnCut(void);

	/**
	 * @brief Copy the selected content
	 */
	void OnCopy(void);

	/**
	 * @brief Paste content from clipboard
	 */
	void OnPaste(void);

	/**
	 * @brief Delete the selected content
	 */
	void OnDelete(void);

	/**
	 * @brief Handle tool actions
	 * @param event The command event containing tool action details
	 */
	void OnToolAction(wxCommandEvent& event);

private:
	/**
	 * @brief Handle idle events
	 * @param event The idle event
	 */
	void OnIdle(wxIdleEvent& event);

	/**
	 * @brief Handle recompute lighting tool action
	 * @param event The command event
	 */
	void OnToolsRecomputeLighting(wxCommandEvent& event);

	/**
	 * @brief Handle play map tool action
	 * @param event The command event
	 */
	void OnToolsPlayMap(wxCommandEvent& event);
};
