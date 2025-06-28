/**
 * @file BrowserWindow.hpp
 * @brief Browser window implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include <wx/dialog.h>
#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/statusbr.h>
#include <wx/toolbar.h>
#include <wx/treectrl.h>
#include <wx/xml/xml.h>

#include <list>
#include <map>
#include <vector>

#include "irrlicht.h"

#include "AudioSystem.hpp"

class TextureBrowser;
class ActorBrowser;
class SoundBrowser;
class MeshBrowser;

/**
 * @class BrowserWindow
 * @brief Window class for browsing and selecting resources
 * 
 * The BrowserWindow class provides a dialog window for browsing and selecting
 * various resources such as actors, textures, and sounds. It includes multiple
 * tabs for different resource types and supports preview functionality.
 */
class BrowserWindow : public wxDialog
{
	friend class TextureBrowser;
	friend class ActorBrowser;
	friend class SoundBrowser;
	friend class MeshBrowser;

public:
	/**
	 * @enum PageNumbers
	 * @brief Enumeration of browser page types
	 */
	enum PageNumbers : int
	{
		PAGE_ACTORS,    ///< Actor browser page
		PAGE_TEXTURES,  ///< Texture browser page
		PAGE_SOUNDS,    ///< Sound browser page
		PAGE_MESHES,    ///< Mesh browser page
	};

protected:
	typedef std::list<wxString> packagelist_t;
	static packagelist_t ms_Packages;

	typedef std::list<wxString> definitionlist_t;
	static definitionlist_t ms_Definitions;

private:
	wxNotebook* m_Notebook;     ///< Notebook for page management
	ActorBrowser* m_Actors;     ///< Actor browser panel
	TextureBrowser* m_Textures; ///< Texture browser panel
	SoundBrowser* m_Sounds;     ///< Sound browser panel
	MeshBrowser* m_Meshes;     ///< Mesh browser panel

public:
	/**
	 * @brief Constructor for the BrowserWindow class
	 * @param parent Pointer to the parent window
	 */
	BrowserWindow(wxWindow* parent);
	
	/**
	 * @brief Destructor
	 */
	~BrowserWindow(void);

	/**
	 * @brief Set the render device
	 * @param renderDevice Pointer to the Irrlicht render device
	 */
	void SetRenderDevice(irr::IrrlichtDevice* renderDevice);

	/**
	 * @brief Set the audio system
	 * @param audioSystem Shared pointer to the audio system
	 */
	void SetAudioSystem(std::shared_ptr<AudioSystem>& audioSystem);

	/**
	 * @brief Switch to a specific page
	 * @param pageNumber The page number to switch to
	 */
	void SwitchTo(int pageNumber);

	/**
	 * @brief Get the selected texture
	 * @return The selected texture name
	 */
	const wxString& GetTexture(void);

	/**
	 * @brief Get the selected actor
	 * @return The selected actor name
	 */
	const wxString& GetActor(void);

	/**
	 * @brief Get the definition of an actor
	 * @param name The name of the actor
	 * @return The actor definition
	 */
	wxString GetActorDefinition(const wxString& name);

	/**
	 * @brief Get the selected mesh
	 * @return The selected mesh name
	 */
	const wxString& GetMesh(void);

	/**
	 * @brief Get the definition of a mesh
	 * @return The mesh definition
	 */
	const wxString& GetMeshDefinition(void);

	/**
	 * @brief Add a package to the browser
	 * @param path The path to the package
	 */
	static void AddPackage(const wxString& path);

	/**
	 * @brief Add a definition to the browser
	 * @param path The path to the definition
	 */
	static void AddDefinition(const wxString& path);

private:
	/**
	 * @brief Handle window close events
	 * @param event The close event
	 */
	void OnCloseEvent(wxCloseEvent& event);

	/**
	 * @brief Handle page change events
	 * @param event The page change event
	 */
	void OnPageChanged(wxNotebookEvent& event);
};

class TextureBrowser : public wxPanel
{
private:
	struct TextureEntry
	{
		wxString path;
		wxRect clickMap;
		wxBitmap bitmap;
		irr::video::IImage* image;
	};

private:
	typedef std::map<wxString, TextureEntry> texturemap_t;
	texturemap_t m_TextureMap;

private:
	wxScrolledWindow* m_Preview;
	wxStatusBar* m_StatusBar;

	wxString m_Selected;

	irr::IrrlichtDevice* m_RenderDevice;

public:
	TextureBrowser(wxWindow* parent);
	~TextureBrowser(void);

	void SetRenderDevice(irr::IrrlichtDevice* renderDevice);

	const wxString& GetSelection(void) { return m_Selected; }

private:
	bool LoadPackage(const wxString& path, bool preload = false);

	void ResizePreview(void);
	
	void OnToolAdd(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
	void OnToolRefresh(wxCommandEvent& event);

	void OnPaint(wxPaintEvent& event);
	void OnMouse(wxMouseEvent& event);
	void AddImage(const wxString& path, wxImage& image,
		irr::video::IImage* irrImage);
	void ScrollTo(const wxString& image);
};

class ActorBrowser : public wxPanel
{
private:
	wxTreeCtrl* m_Tree;
	wxTreeItemId m_Root;
	wxArrayString m_Categories;

	typedef std::map<wxTreeItemId, wxString> itempath_t;
	itempath_t m_ItemPaths;
	// wxArrayString m_DefinitionFiles;

	wxString m_Selected;

public:
	ActorBrowser(wxWindow* parent);
	~ActorBrowser(void);

	const wxString& GetSelection(void);
	wxString GetDefinition(const wxString& name);

private:
	bool LoadPackage(const wxString& path, bool preload = false);
	bool LoadDefinition(const wxString& path, bool preload = false);

	void OnToolAdd(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
	void OnToolSave(wxCommandEvent& event);
	void OnToolRefresh(wxCommandEvent& event);

	void OnItemActivate(wxTreeEvent& event);
	void OnItemSelected(wxTreeEvent& event);

private:
	void AddActor(const wxXmlDocument& definition, const wxString& sourceFile, bool fromPackage);
	wxTreeItemId FindItem(const wxString& name, wxTreeItemId& start);
};

class SoundBrowser : public wxPanel
{
private:
	enum
	{
		COL_PATH = 0,
		COL_TYPE,
		COL_CHANNELS,
		COL_FREQ,
		COL_PACKAGE
	};

private:
	wxListView* m_List;

	typedef std::map<long, wxString> itempath_t;
	itempath_t m_ItemPaths;

	std::shared_ptr<AudioSystem> m_AudioSystem;

public:
	SoundBrowser(wxWindow* parent);
	~SoundBrowser(void);

	void SetAudioSystem(std::shared_ptr<AudioSystem>& audioSystem);

private:
	bool LoadPackage(const wxString& path, bool preload = false);

	void OnToolAdd(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
	void OnToolPlay(wxCommandEvent& event);
	void OnToolStop(wxCommandEvent& event);
	void OnToolRefresh(wxCommandEvent& event);

	void OnItemActivate(wxListEvent& event);
};

class MeshBrowser : public wxPanel
{
private:
	enum
	{
		COL_NAME = 0,
		COL_PACKAGE
	};

	wxListView* m_List;

	typedef std::map<long, wxString> itemdefinition_t;
	itemdefinition_t m_ItemDefinitions;

	wxString m_Selection;
	wxString m_Definition;

public:
	MeshBrowser(wxWindow* parent);
	~MeshBrowser(void);

	const wxString& GetSelection(void);
	const wxString& GetDefinition(void);

private:
	bool LoadPackage(const wxString& path, bool preload = false);
	bool LoadDefinition(const wxString& path, bool preload = false);

	void OnToolAdd(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
	void OnToolRefresh(wxCommandEvent& event);

	void OnItemSelected(wxListEvent& event);
};

