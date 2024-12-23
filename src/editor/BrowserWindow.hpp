/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/dialog.h>
#include <wx/filename.h>
#include <wx/listbox.h>
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

class TextureBrowser;
class ActorBrowser;

class BrowserWindow : public wxDialog
{
public:
	enum PageNumbers : int
	{
		PAGE_TEXTURES,
		PAGE_ACTORS,
	};

private:
	wxNotebook* m_Notebook;

	TextureBrowser* m_Textures;
	ActorBrowser* m_Actors;

public:
	BrowserWindow(wxWindow* parent);
	~BrowserWindow(void);

	void SetRenderDevice(irr::IrrlichtDevice* renderDevice);

	void SwitchTo(int pageNumber);

	const wxString& GetTexture(void);

private:
	void OnCloseEvent(wxCloseEvent& event);
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
	typedef std::list<wxString> packagelist_t;
	static packagelist_t ms_Packages;

	typedef std::vector<TextureEntry> textures_t;
	textures_t m_Textures;

	typedef std::map<wxString, textures_t::iterator> texturemap_t;
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

	static void AddPackage(const wxString& path);

private:
	bool LoadPackage(const wxString& path, bool preload = false);

	void ResizePreview(void);
	
	void OnToolAdd(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
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

	wxFileName m_DefinitionFile;

public:
	ActorBrowser(wxWindow* parent);
	~ActorBrowser(void);

	//const wxString& GetSelection(void);

private:
	void OnToolAdd(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
	void OnToolSave(wxCommandEvent& event);
	void OnToolSaveAs(wxCommandEvent& event);

	void OnItemActivate(wxTreeEvent& event);

private:
	void AddActor(wxXmlNode* actor);
	wxTreeItemId FindItem(const wxString& name, wxTreeItemId& start);
};
