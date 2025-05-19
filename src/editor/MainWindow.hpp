/**
 * @file MainWindow.hpp
 * @brief Main window implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include "AudioSystem.hpp"
#include "BrowserWindow.hpp"
#include "Editor.hpp"
#include "MapEditor.hpp"
#include "PackageManager.hpp"

#include <wx/aui/aui.h>
#include <wx/cmdproc.h>
#include <wx/frame.h>

#include <memory>

/**
 * @class MainWindow
 * @brief Main application window class
 * 
 * The MainWindow class serves as the main application window and manages
 * the overall application state, including active editors, menus, and
 * global resources.
 */
class MainWindow : public wxFrame
{
private:
	wxAuiManager m_AuiMgr;              ///< AUI manager for window layout
	wxMenu* m_EditMenu;                 ///< Edit menu

	BrowserWindow* m_Browser;           ///< Browser window for resources
	PackageManager* m_PackageManager;   ///< Package manager

	Editor* m_ActiveEditor;             ///< Currently active editor

	int m_LastFPS;                      ///< Last recorded FPS

	std::shared_ptr<AudioSystem> m_AudioSystem; ///< Audio system

public:
	/**
	 * @brief Constructor for the MainWindow class
	 */
	MainWindow(void);
	
	/**
	 * @brief Destructor
	 */
	~MainWindow(void);

	/**
	 * @brief Get the AUI manager
	 * @return Reference to the AUI manager
	 */
	wxAuiManager& GetAuiMgr(void) { return m_AuiMgr; }

	/**
	 * @brief Load a file into the editor
	 * @param filePath Path to the file to load
	 */
	void LoadFile(const wxString& filePath);

	/**
	 * @brief Set the window caption
	 * @param fileName Name of the current file
	 */
	void SetCaption(const wxString& fileName);

	/**
	 * @brief Update the frame time display
	 * @param fps Current frames per second
	 */
	void UpdateFrameTime(int fps);

	/**
	 * @brief Handle tool actions
	 * @param event The command event containing tool action details
	 */
	void OnToolAction(wxCommandEvent& event);

private:
	/**
	 * @brief Handle window close events
	 * @param event The close event
	 */
	void OnClose(wxCloseEvent& event);

	/**
	 * @brief Handle configuration change events
	 * @param event The command event
	 */
	void OnConfigChanged(wxCommandEvent& event);

	/**
	 * @brief Handle new map action
	 * @param event The command event
	 */
	void OnFileNewMap(wxCommandEvent& event);

	/**
	 * @brief Handle new project action
	 * @param event The command event
	 */
	void OnFileNewProject(wxCommandEvent& event);

	/**
	 * @brief Handle open map action
	 * @param event The command event
	 */
	void OnFileOpenMap(wxCommandEvent& event);

	/**
	 * @brief Handle open project action
	 * @param event The command event
	 */
	void OnFileOpenProject(wxCommandEvent& event);

	/**
	 * @brief Handle open file action
	 * @param event The command event
	 */
	void OnFileOpen(wxCommandEvent& event);

	/**
	 * @brief Handle save action
	 * @param event The command event
	 */
	void OnFileSave(wxCommandEvent& event);

	/**
	 * @brief Handle save as action
	 * @param event The command event
	 */
	void OnFileSaveAs(wxCommandEvent& event);

	/**
	 * @brief Handle close action
	 * @param event The command event
	 */
	void OnFileClose(wxCommandEvent& event);

	/**
	 * @brief Handle preferences action
	 * @param event The command event
	 */
	void OnFilePreferences(wxCommandEvent& event);

	/**
	 * @brief Handle exit action
	 * @param event The command event
	 */
	void OnFileExit(wxCommandEvent& event);

	/**
	 * @brief Handle undo action
	 * @param event The command event
	 */
	void OnEditUndo(wxCommandEvent& event);

	/**
	 * @brief Handle redo action
	 * @param event The command event
	 */
	void OnEditRedo(wxCommandEvent& event);

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
	 * @brief Handle about action
	 * @param event The command event
	 */
	void OnHelpAbout(wxCommandEvent& event);

	/**
	 * @brief Handle entity browser action
	 * @param event The command event
	 */
	void OnToolsEntityBrowser(wxCommandEvent& event);

	/**
	 * @brief Handle actor browser action
	 * @param event The command event
	 */
	void OnToolsActorBrowser(wxCommandEvent& event);

	/**
	 * @brief Handle texture browser action
	 * @param event The command event
	 */
	void OnToolsTextureBrowser(wxCommandEvent& event);

	/**
	 * @brief Handle sound browser action
	 * @param event The command event
	 */
	void OnToolsSoundBrowser(wxCommandEvent& event);

	/**
	 * @brief Handle package manager action
	 * @param event The command event
	 */
	void OnToolsPackageManager(wxCommandEvent& event);
};
