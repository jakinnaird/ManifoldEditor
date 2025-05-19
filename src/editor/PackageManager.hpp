/**
 * @file PackageManager.hpp
 * @brief Package manager implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include <wx/cmdproc.h>
#include <wx/dialog.h>
#include <wx/filename.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>

#include <map>

/**
 * @class PackageManager
 * @brief Dialog class for managing resource packages
 * 
 * The PackageManager class provides a dialog window for managing resource
 * packages, including creating, opening, saving, and modifying package
 * contents. It supports undo/redo operations and file extraction.
 */
class PackageManager : public wxDialog
{
private:
	wxCommandProcessor m_Commands;       ///< Command processor for undo/redo
	wxStaticText* m_FileText;           ///< Text display for current file
	wxListView* m_FileList;             ///< List view for package contents

	typedef std::map<long, wxString> itempath_t; ///< Type for item path mapping
	itempath_t m_ItemPaths;             ///< Map of item IDs to paths

public:
	/**
	 * @brief Constructor for the PackageManager class
	 * @param parent Pointer to the parent window
	 */
	PackageManager(wxWindow* parent);
	
	/**
	 * @brief Destructor
	 */
	~PackageManager(void);

	/**
	 * @brief Save the current package
	 * @param destPath Destination path for the package
	 * @param srcPath Source path for the package (optional)
	 * @return true if save was successful, false otherwise
	 */
	bool Save(const wxString& destPath, const wxString& srcPath = wxEmptyString);

private:
	/**
	 * @brief Handle window close events
	 * @param event The close event
	 */
	void OnCloseEvent(wxCloseEvent& event);

	/**
	 * @brief Handle new package action
	 * @param event The command event
	 */
	void OnToolNew(wxCommandEvent& event);

	/**
	 * @brief Handle open package action
	 * @param event The command event
	 */
	void OnToolOpen(wxCommandEvent& event);

	/**
	 * @brief Handle save package action
	 * @param event The command event
	 */
	void OnToolSave(wxCommandEvent& event);

	/**
	 * @brief Handle save as package action
	 * @param event The command event
	 */
	void OnToolSaveAs(wxCommandEvent& event);

	/**
	 * @brief Handle undo action
	 * @param event The command event
	 */
	void OnToolUndo(wxCommandEvent& event);

	/**
	 * @brief Handle redo action
	 * @param event The command event
	 */
	void OnToolRedo(wxCommandEvent& event);

	/**
	 * @brief Handle add file action
	 * @param event The command event
	 */
	void OnToolAdd(wxCommandEvent& event);

	/**
	 * @brief Handle remove file action
	 * @param event The command event
	 */
	void OnToolRemove(wxCommandEvent& event);

	/**
	 * @brief Handle extract file action
	 * @param event The command event
	 */
	void OnToolExtract(wxCommandEvent& event);
};
