/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/statusbr.h>
#include <wx/toolbar.h>
#include <wx/treectrl.h>

#include <list>
#include <map>
#include <vector>

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

public:
	TextureBrowser(wxWindow* parent);
	~TextureBrowser(void);

	const wxString& GetSelection(void) { return m_Selected; }

	static void AddPackage(const wxString& path);

private:
	bool LoadPackage(const wxString& path, bool preload = false);

	void ResizePreview(void);
	
	void OnToolAdd(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnMouse(wxMouseEvent& event);
	void AddImage(const wxString& path, wxImage& image);
	void ScrollTo(const wxString& image);
};

class ActorBrowser : public wxPanel
{
private:
	typedef std::list<wxString> packagelist_t;
	static packagelist_t ms_Packages;

private:
	wxTreeCtrl* m_Tree;
	wxTreeItemId m_Root;

public:
	ActorBrowser(wxWindow* parent);
	~ActorBrowser(void);

	//const wxString& GetSelection(void);

	static void AddPackage(const wxString& path);

private:
	bool LoadPackage(const wxString& path, bool preload = false);

	void OnToolAdd(wxCommandEvent& event);
	void OnToolOpen(wxCommandEvent& event);
};
