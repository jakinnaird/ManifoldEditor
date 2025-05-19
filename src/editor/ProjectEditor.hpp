/**
 * @file ProjectEditor.hpp
 * @brief Project editor implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include "Editor.hpp"
#include "ProjectExplorer.hpp"

#include <wx/aui/aui.h>
#include <wx/filename.h>

#include <memory>

#include "irrlicht.h"

/**
 * @class ProjectEditor
 * @brief Editor class for managing project files and resources
 * 
 * The ProjectEditor class provides functionality for managing project files,
 * resources, and configurations in the Manifold Editor. It includes features
 * for file organization, resource management, and project building.
 */
class ProjectEditor : public Editor
{
private:
	wxAuiManager m_AuiMgr;              ///< AUI manager for window layout
	wxAuiNotebook* m_Pages;             ///< Notebook for multiple editor pages

	wxFileName m_FileName;              ///< Current project file

	ProjectExplorer* m_Explorer;        ///< Project explorer panel

	irr::IrrlichtDevice* m_RenderDevice; ///< Irrlicht rendering device

public:
	/**
	 * @brief Constructor for the ProjectEditor class
	 * @param parent Pointer to the parent window
	 * @param editMenu Pointer to the edit menu
	 * @param browserWindow Pointer to the browser window
	 * @param fileName Name of the project file
	 */
	ProjectEditor(MainWindow* parent, wxMenu* editMenu, 
		BrowserWindow* browserWindow, const wxFileName& fileName);
	~ProjectEditor(void);

	/**
	 * @brief Get the current project file
	 * @return The project file name
	 */
	const wxFileName& GetFileName(void) { return m_FileName; }

	/**
	 * @brief Open a new editor window for the specified file
	 * @param fileName Path to the file to open
	 */
	void OpenFile(const wxFileName& fileName);

	/**
	 * @brief Load a project from file
	 * @param filePath Path to the project file
	 */
	void Load(const wxFileName& filePath);

	/**
	 * @brief Check if the project has unsaved changes
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
	 * @brief Save the current project
	 * @param allFiles Whether to save all open files
	 * @return true if save was successful, false otherwise
	 */
	bool OnSave(bool allFiles);

	/**
	 * @brief Save the current project to a new file
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
	 * @brief Handle page close events
	 * @param event The notebook event
	 */
	void OnPageClose(wxAuiNotebookEvent& event);

	/**
	 * @brief Handle build project action
	 * @param event The command event
	 */
	void OnBuildProject(wxCommandEvent& event);
};

/**
 * @class EditorPage
 * @brief Base class for editor pages in the project editor
 * 
 * The EditorPage class serves as the base class for different types of
 * editor pages that can be opened in the project editor.
 */
class EditorPage : public wxPanel
{
protected:
	wxMenu* m_EditMenu;     ///< The edit menu for this page

public:
	/**
	 * @brief Constructor for the EditorPage class
	 * @param parent Pointer to the parent window
	 * @param editMenu Pointer to the edit menu
	 */
	EditorPage(wxWindow* parent, wxMenu* editMenu)
		: wxPanel(parent), m_EditMenu(editMenu) {}
	
	/**
	 * @brief Virtual destructor
	 */
	virtual ~EditorPage(void) {}

	/**
	 * @brief Check if the page has unsaved changes
	 * @return true if there are unsaved changes, false otherwise
	 */
	virtual bool HasChanged(void) = 0;

	/**
	 * @brief Save the current content
	 */
	virtual void Save(void) = 0;

	/**
	 * @brief Undo the last action
	 */
	virtual void OnUndo(void) = 0;

	/**
	 * @brief Redo the last undone action
	 */
	virtual void OnRedo(void) = 0;

	/**
	 * @brief Cut the selected content
	 */
	virtual void OnCut(void) = 0;

	/**
	 * @brief Copy the selected content
	 */
	virtual void OnCopy(void) = 0;

	/**
	 * @brief Paste content from clipboard
	 */
	virtual void OnPaste(void) = 0;
};
