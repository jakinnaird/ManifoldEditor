/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "BrowserWindow.hpp"

#include <wx/artprov.h>
#include <wx/dcclient.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/sizer.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>


BrowserWindow::BrowserWindow(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, wxEmptyString)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->SetMinSize(640, 480);

	m_Notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxNB_TOP | wxNB_FIXEDWIDTH);

	m_Textures = new TextureBrowser(m_Notebook);
	m_Actors = new ActorBrowser(m_Notebook);

	m_Notebook->InsertPage(PAGE_TEXTURES, m_Textures, _("Textures"), true);
	m_Notebook->InsertPage(PAGE_ACTORS, m_Actors, _("Actors"), false);

	sizer->Add(m_Notebook, wxSizerFlags(9).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_CLOSE_WINDOW, &BrowserWindow::OnCloseEvent, this);

	SwitchTo(PAGE_ACTORS);
}

BrowserWindow::~BrowserWindow(void)
{
}

void BrowserWindow::SetRenderDevice(irr::IrrlichtDevice* renderDevice)
{
	m_Textures->SetRenderDevice(renderDevice);
}

void BrowserWindow::SwitchTo(int pageNumber)
{
	m_Notebook->SetSelection(pageNumber);
	switch (pageNumber)
	{
	case PAGE_TEXTURES:
		SetTitle(_("Texture Browser"));
		break;
	case PAGE_ACTORS:
		SetTitle(_("Actor Browser"));
		break;
	}
}

const wxString& BrowserWindow::GetTexture(void)
{
	return m_Textures->GetSelection();
}

void BrowserWindow::OnCloseEvent(wxCloseEvent& event)
{
	if (event.CanVeto())
	{
		this->Show(false); // hide ourselves
		event.Veto();
	}
}

#define START_X		5
#define START_Y		2
#define SPACE_Y		5

TextureBrowser::TextureBrowser(wxWindow* parent)
	: wxPanel(parent)
{
	// create the toolbar
	wxToolBar* tools = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_HORIZONTAL);
	tools->AddTool(wxID_NEW, _("Add"), wxArtProvider::GetBitmap(wxART_NEW),
		_("Add new texture"));
	tools->AddTool(wxID_OPEN, _("Open"), wxArtProvider::GetBitmap(wxART_FILE_OPEN),
		_("Open package"));
	tools->Realize();

	// create the widgets
	m_Preview = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxVSCROLL | wxALWAYS_SHOW_SB | wxRETAINED);
	m_Preview->SetScrollRate(0, 25);
	m_Preview->SetVirtualSize(600, 100);

	m_StatusBar = new wxStatusBar(this, wxID_ANY,
		wxSTB_ELLIPSIZE_MIDDLE | wxFULL_REPAINT_ON_RESIZE);
	m_StatusBar->SetStatusText(m_Selected);

	// create the sizer
	wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
	boxSizer->Add(tools, wxSizerFlags(1).Expand());
	boxSizer->Add(m_Preview, wxSizerFlags(9).Expand());
	boxSizer->Add(m_StatusBar, wxSizerFlags(1).Expand());
	this->SetSizerAndFit(boxSizer);

	m_RenderDevice = nullptr;

	// configure the event handling
	Bind(wxEVT_MENU, &TextureBrowser::OnToolAdd, this, wxID_NEW);
	Bind(wxEVT_MENU, &TextureBrowser::OnToolOpen, this, wxID_OPEN);
	m_Preview->Bind(wxEVT_PAINT, &TextureBrowser::OnPaint, this);
	m_Preview->Bind(wxEVT_LEFT_UP, &TextureBrowser::OnMouse, this);
}

TextureBrowser::~TextureBrowser(void)
{
	for (textures_t::iterator tex = m_Textures.begin();
		tex != m_Textures.end(); ++tex)
	{
		(*tex).image->drop();
	}
}

void TextureBrowser::SetRenderDevice(irr::IrrlichtDevice* renderDevice)
{
	m_RenderDevice = renderDevice;
	if (m_RenderDevice)
	{
		// did we pre-load any packages?
		if (ms_Packages.size() > 0)
		{
			for (packagelist_t::iterator i = ms_Packages.begin();
				i != ms_Packages.end(); ++i)
			{
				LoadPackage(*i, true);
			}
		}

		ResizePreview();
		ScrollTo(m_Selected);
	}
}

void TextureBrowser::AddPackage(const wxString& path)
{
	for (packagelist_t::iterator i = ms_Packages.begin();
		i != ms_Packages.end(); ++i)
	{
		if ((*i) == path)
			return; // already exists
	}

	ms_Packages.push_back(path);
}

bool TextureBrowser::LoadPackage(const wxString& path, bool preload)
{
	if (!preload)
	{
		for (packagelist_t::iterator i = ms_Packages.begin();
			i != ms_Packages.end(); ++i)
		{
			if ((*i) == path)
				return true; // already exists
		}
	}

	wxFileInputStream inStream(path);
	if (inStream.IsOk())
	{
		wxZipInputStream zipStream(inStream);
		if (!zipStream.IsOk())
		{
			wxLogWarning(_("Unsupported archive: %s"), path);
			return false;
		}

		if (!preload)
			ms_Packages.push_back(path);

		wxZipEntry* entry = zipStream.GetNextEntry();
		while (entry)
		{
			wxString texPath(entry->GetName());

			// support archives made on any platform
			if (texPath.StartsWith(wxT("textures/")) ||
				texPath.StartsWith(wxT("textures\\")))
			{
				// build the image path
				wxString imagePath(path);
				imagePath.append(wxT(":"));
				imagePath.append(texPath);

				irr::io::path filename(imagePath.c_str().AsChar());
				irr::video::IImage* image = m_RenderDevice->getVideoDriver()
					->createImageFromFile(filename);
				if (image)
				{
					wxSize size(image->getDimension().Width,
						image->getDimension().Height);
					wxImage img(size, (unsigned char*)image->lock(), true);
					image->unlock();

					if (img.IsOk())
						AddImage(imagePath, img, image);
				}
			}

			entry->UnRef();
			entry = zipStream.GetNextEntry();
		}
	}

	return true;
}

void TextureBrowser::ResizePreview(void)
{
	wxSize size(600, 10);
	for (textures_t::iterator i = m_Textures.begin(); i != m_Textures.end(); ++i)
	{
		size.y += i->clickMap.height + SPACE_Y;
	}

	m_Preview->SetVirtualSize(size);
}

void TextureBrowser::OnToolAdd(wxCommandEvent& event)
{
	wxFileDialog openFile(this, _("Select image file"), wxEmptyString,
		wxEmptyString, _("Image Files|*.jpg;*.png;*.bmp;*.tga"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openFile.ShowModal() == wxID_CANCEL)
		return;

	wxString path = openFile.GetPath();

	// check if the path has already been added
	texturemap_t::iterator i = m_TextureMap.find(path);
	if (i != m_TextureMap.end())
	{
		// it's already loaded, so scroll there
		m_Selected = path;
		ScrollTo(m_Selected);
		m_StatusBar->SetStatusText(m_Selected);
		m_Preview->Refresh();
	}
	else
	{
		// load the image
		irr::io::path filename(path.c_str().AsChar());
		irr::video::IImage* image = m_RenderDevice->getVideoDriver()
			->createImageFromFile(filename);
		if (image)
		{
			wxSize size(image->getDimension().Width,
				image->getDimension().Height);
			wxImage img(size, (unsigned char*)image->lock(), true);
			image->unlock();

			if (img.IsOk())
			{
				AddImage(path, img, image);
				ResizePreview();

				m_Selected = path;

				// scroll there
				ScrollTo(m_Selected);
				m_StatusBar->SetStatusText(m_Selected);
				m_Preview->Refresh();
			}
		}
	}
}

void TextureBrowser::OnToolOpen(wxCommandEvent& event)
{
	wxFileDialog openFile(this, _("Select image file"),
		wxEmptyString, wxEmptyString,
		_("Manifold Archive Package (*.mpk)|*.mpk|Zip Archive (*.zip)|*.zip"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openFile.ShowModal() == wxID_CANCEL)
		return;

	if (LoadPackage(openFile.GetPath()))
	{
		ResizePreview();
		m_Preview->Refresh();
	}
}

void TextureBrowser::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(m_Preview);
	m_Preview->DoPrepareDC(dc);

	for (textures_t::iterator tex = m_Textures.begin(); tex != m_Textures.end(); ++tex)
	{
		if (tex->path == m_Selected)
		{
			// draw the selection rectangle
			wxSize sz = tex->bitmap.GetSize();
			sz.x += 4;
			sz.y += 4;

			dc.SetBrush(*wxWHITE_BRUSH);
			dc.DrawRectangle(wxPoint(START_X - 2, tex->clickMap.y - 2), sz);
		}

		dc.DrawBitmap(tex->bitmap, START_X, tex->clickMap.y, true);
	}
}

void TextureBrowser::OnMouse(wxMouseEvent& event)
{
	wxPoint pos = m_Preview->CalcUnscrolledPosition(event.GetPosition());
	for (textures_t::iterator i = m_Textures.begin(); i != m_Textures.end(); ++i)
	{
		if (i->clickMap.Contains(pos))
		{
			m_Selected = i->path;
			m_StatusBar->SetStatusText(m_Selected);
			m_Preview->Refresh();
		}
	}
}

void TextureBrowser::AddImage(const wxString& path, wxImage& image,
	irr::video::IImage* irrImage)
{
	TextureEntry entry;

	// resize the image to fit the preview
	const wxSize& size = m_Preview->GetSize() * m_Preview->GetContentScaleFactor();
	if (image.GetWidth() > size.GetWidth())
	{
		float aspect = image.GetHeight() / (float)image.GetWidth() * size.x;
		image.Rescale(size.x, aspect);
	}

	// add the image to the list
	entry.path = path;
	entry.bitmap = image;
	entry.image = irrImage;

	// calculate the click map
	textures_t::reverse_iterator t = m_Textures.rbegin();
	if (t != m_Textures.rend())
	{
		entry.clickMap.x = t->clickMap.x;
		entry.clickMap.y = t->clickMap.y + t->clickMap.height + SPACE_Y;
		entry.clickMap.width = image.GetWidth();
		entry.clickMap.height = image.GetHeight();
	}
	else
		entry.clickMap = wxRect(wxPoint(START_X, START_Y), image.GetSize());

	m_Textures.push_back(entry);
	m_TextureMap.emplace(path, std::prev(m_Textures.end()));
}

void TextureBrowser::ScrollTo(const wxString& image)
{
	texturemap_t::iterator i = m_TextureMap.find(m_Selected);
	if (i != m_TextureMap.end())
		m_Preview->Scroll(wxPoint(0, i->second->clickMap.y));
}

TextureBrowser::packagelist_t TextureBrowser::ms_Packages;

ActorBrowser::ActorBrowser(wxWindow* parent)
	: wxPanel(parent)
{
	// create the toolbar
	wxToolBar* tools = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_HORIZONTAL);
	tools->AddTool(wxID_NEW, _("Add"), wxArtProvider::GetBitmap(wxART_NEW),
		_("Add new actor"));
	tools->AddTool(wxID_OPEN, _("Open"), wxArtProvider::GetBitmap(wxART_FILE_OPEN),
		_("Open actor definition"));
	tools->Realize();

	m_Tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxTR_HAS_BUTTONS | wxTR_SINGLE);
	m_Root = m_Tree->AddRoot(_("Actors"));
	m_Tree->Expand(m_Root);

	// did we pre-load any packages?
	if (ms_Packages.size() > 0)
	{
		for (packagelist_t::iterator i = ms_Packages.begin();
			i != ms_Packages.end(); ++i)
		{
			LoadPackage(*i, true);
		}
	}

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(tools, wxSizerFlags(1).Expand());
	sizer->Add(m_Tree, wxSizerFlags(9).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_MENU, &ActorBrowser::OnToolAdd, this, wxID_NEW);
	Bind(wxEVT_MENU, &ActorBrowser::OnToolOpen, this, wxID_OPEN);
}

ActorBrowser::~ActorBrowser(void)
{
}

//const wxString& ActorBrowser::GetSelection(void)
//{
//}

void ActorBrowser::AddPackage(const wxString& path)
{
	for (packagelist_t::iterator i = ms_Packages.begin();
		i != ms_Packages.end(); ++i)
	{
		if ((*i) == path)
			return; // already exists
	}

	ms_Packages.push_back(path);
}

bool ActorBrowser::LoadPackage(const wxString& path, bool preload)
{
	if (!preload)
	{
		for (packagelist_t::iterator i = ms_Packages.begin();
			i != ms_Packages.end(); ++i)
		{
			if ((*i) == path)
				return true; // already exists
		}
	}

	wxFileInputStream inStream(path);
	if (inStream.IsOk())
	{
		wxZipInputStream zipStream(inStream);
		if (!zipStream.IsOk())
		{
			wxLogWarning(_("Unsupported archive: %s"), path);
			return false;
		}

		if (!preload)
			ms_Packages.push_back(path);

		wxZipEntry* entry = zipStream.GetNextEntry();
		while (entry)
		{
			wxFileName fn(entry->GetName());
			//wxString texPath(entry->GetName());

			//// support archives made on any platform
			//if (texPath.StartsWith(wxT("textures/")) ||
			//	texPath.StartsWith(wxT("textures\\")))
			//{
			//	// read the image data
			//	wxMemoryOutputStream memStream;
			//	zipStream.Read(memStream);

			//	// convert to something we can use
			//	wxMemoryInputStream imageStream(memStream);

			//	// read the image
			//	wxImage image(imageStream);
			//	if (image.IsOk())
			//	{
			//		// build the image path
			//		wxString imagePath(path);
			//		imagePath.append(wxT(":"));
			//		imagePath.append(texPath);
			//		AddImage(imagePath, image);
			//	}
			//}

			entry->UnRef();
			entry = zipStream.GetNextEntry();
		}
	}

	return true;
}

void ActorBrowser::OnToolAdd(wxCommandEvent& event)
{
}

void ActorBrowser::OnToolOpen(wxCommandEvent& event)
{
}

ActorBrowser::packagelist_t ActorBrowser::ms_Packages;
