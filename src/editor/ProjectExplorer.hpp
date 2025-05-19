/**
 * @file ProjectExplorer.hpp
 * @brief Project explorer implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include <wx/filename.h>
#include <wx/panel.h>
#include <wx/treectrl.h>

class ProjectEditor;

/**
 * @class ProjectExplorer
 * @brief Panel class for project file and resource management
 * 
 * The ProjectExplorer class provides a tree view panel for managing project
 * files and resources. It supports file organization, package management,
 * and project building operations.
 */
class ProjectExplorer : public wxPanel
{
private:
	ProjectEditor* m_Editor;            ///< Parent project editor
	wxTreeCtrl* m_Explorer;             ///< Tree control for file hierarchy
	wxTreeItemId m_Root;                ///< Root tree item

public:
	/**
	 * @brief Constructor for the ProjectExplorer class
	 * @param parent Pointer to the parent project editor
	 */
	ProjectExplorer(ProjectEditor* parent);
	
	/**
	 * @brief Destructor
	 */
	~ProjectExplorer(void);

	/**
	 * @brief Save the project to a file
	 * @param fileName Path to save the project
	 */
	void Save(const wxFileName& fileName);

	/**
	 * @brief Load a project from a file
	 * @param fileName Path to load the project from
	 */
	void Load(const wxFileName& fileName);

	/**
	 * @brief Clear the project explorer
	 */
	void Clear(void);

	/**
	 * @brief Check if the project has a filename
	 * @return true if the project has a filename, false otherwise
	 */
	bool HasFilename(void);

	/**
	 * @brief Get the project filename
	 * @return The project filename
	 */
	const wxFileName& GetFilename(void);

private:
	/**
	 * @brief Handle new MPK package creation
	 * @param fileName Path for the new package
	 */
	void OnNewMpkPackage(const wxFileName& fileName);

	/**
	 * @brief Handle new ZIP package creation
	 * @param fileName Path for the new package
	 */
	void OnNewZipPackage(const wxFileName& fileName);

	/**
	 * @brief Build a package
	 * @param package Tree item ID of the package to build
	 */
	void BuildPackage(const wxTreeItemId& package);

	/**
	 * @brief Clean a package
	 * @param package Tree item ID of the package to clean
	 */
	void CleanPackage(const wxTreeItemId& package);

	/**
	 * @brief Open a map file
	 * @param fileName Path to the map file
	 */
	void OpenMap(const wxFileName& fileName);

private:
	/**
	 * @brief Handle tree item right-click events
	 * @param event The tree event
	 */
	void OnItemRightClick(wxTreeEvent& event);

	/**
	 * @brief Handle tree item activation events
	 * @param event The tree event
	 */
	void OnItemActivated(wxTreeEvent& event);

	/**
	 * @brief Handle new package menu action
	 * @param event The command event
	 */
	void OnMenuNewPackage(wxCommandEvent& event);

	/**
	 * @brief Handle new map menu action
	 * @param event The command event
	 */
	void OnMenuNewMap(wxCommandEvent& event);

	/**
	 * @brief Handle add new item menu action
	 * @param event The command event
	 */
	void OnMenuAddNewItem(wxCommandEvent& event);

	/**
	 * @brief Handle add existing item menu action
	 * @param event The command event
	 */
	void OnMenuAddExistingItem(wxCommandEvent& event);

	/**
	 * @brief Handle add filter menu action
	 * @param event The command event
	 */
	void OnMenuAddFilter(wxCommandEvent& event);

	/**
	 * @brief Handle open file menu action
	 * @param event The command event
	 */
	void OnMenuOpenFile(wxCommandEvent& event);

	/**
	 * @brief Handle remove menu action
	 * @param event The command event
	 */
	void OnMenuRemove(wxCommandEvent& event);

public:
	/**
	 * @brief Handle build project menu action
	 * @param event The command event
	 */
	void OnMenuBuildProject(wxCommandEvent& event);

	/**
	 * @brief Handle clean project menu action
	 * @param event The command event
	 */
	void OnMenuCleanProject(wxCommandEvent& event);

	/**
	 * @brief Handle build package menu action
	 * @param event The command event
	 */
	void OnMenuBuildPackage(wxCommandEvent& event);

	/**
	 * @brief Handle clean package menu action
	 * @param event The command event
	 */
	void OnMenuCleanPackage(wxCommandEvent& event);
};
