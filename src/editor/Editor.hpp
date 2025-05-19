/**
 * @file Editor.hpp
 * @brief Base editor class for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include <wx/filename.h>
#include <wx/menu.h>
#include <wx/panel.h>

class MainWindow;
class BrowserWindow;

/**
 * @class Editor
 * @brief Base class for all editor types in the Manifold Editor
 * 
 * The Editor class serves as the base class for different types of editors
 * in the Manifold Editor application. It provides common functionality and
 * interface for map and project editors.
 */
class Editor : public wxPanel
{
public:
	/**
	 * @enum EDITOR_TYPE
	 * @brief Enumeration of supported editor types
	 */
	enum EDITOR_TYPE
	{
		MAP_EDITOR,     ///< Map editor type
		PROJECT_EDITOR  ///< Project editor type
	};

protected:
	EDITOR_TYPE m_Type;     ///< The type of this editor
	wxString m_Title;       ///< The title of this editor
	wxMenu* m_EditMenu;     ///< The edit menu for this editor

	BrowserWindow* m_Browser; ///< The browser window associated with this editor

public:
	/**
	 * @brief Constructor for the Editor class
	 * @param parent Pointer to the parent window
	 * @param editMenu Pointer to the edit menu
	 * @param type The type of editor to create
	 * @param browserWindow Pointer to the browser window
	 */
	Editor(MainWindow* parent, wxMenu* editMenu, EDITOR_TYPE type,
		BrowserWindow* browserWindow);
	
	/**
	 * @brief Virtual destructor
	 */
	virtual ~Editor(void);

	/**
	 * @brief Get the type of this editor
	 * @return The editor type
	 */
	EDITOR_TYPE GetType(void) { return m_Type; }

	/**
	 * @brief Get the title of this editor
	 * @return The editor title
	 */
	const wxString& GetTitle(void) { return m_Title; }

	/**
	 * @brief Load content from a file
	 * @param filePath Path to the file to load
	 */
	virtual void Load(const wxFileName& filePath) = 0;

	/**
	 * @brief Handle tool actions
	 * @param event The command event containing tool action details
	 */
	virtual void OnToolAction(wxCommandEvent& event) = 0;

	/**
	 * @brief Check if the editor has unsaved changes
	 * @return true if there are unsaved changes, false otherwise
	 */
	virtual bool HasChanged(void) = 0;

	/**
	 * @brief Undo the last action
	 */
	virtual void OnUndo(void) = 0;

	/**
	 * @brief Redo the last undone action
	 */
	virtual void OnRedo(void) = 0;

	/**
	 * @brief Save the current content
	 * @param allFiles Whether to save all open files
	 * @return true if save was successful, false otherwise
	 */
	virtual bool OnSave(bool allFiles) = 0;

	/**
	 * @brief Save the current content to a new file
	 * @return true if save was successful, false otherwise
	 */
	virtual bool OnSaveAs(void) = 0;

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

	/**
	 * @brief Delete the selected content
	 */
	virtual void OnDelete(void) = 0;
};
