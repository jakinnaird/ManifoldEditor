/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "BrowserWindow.hpp"
#include "Common.hpp"
#include "FSHandler.hpp"

#include <wx/artprov.h>
#include <wx/busyinfo.h>
#include <wx/choicdlg.h>
#include <wx/dcclient.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/mimetype.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>
#include <wx/propgrid/propgrid.h>
#include <wx/sizer.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

BrowserWindow::packagelist_t BrowserWindow::ms_Packages;

BrowserWindow::BrowserWindow(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, wxEmptyString)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->SetMinSize(640, 480);

	m_Notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxNB_TOP | wxNB_FIXEDWIDTH);

	m_Actors = new ActorBrowser(m_Notebook);
	m_Textures = new TextureBrowser(m_Notebook);
	m_Sounds = new SoundBrowser(m_Notebook);
	m_Meshes = new MeshBrowser(m_Notebook);

	m_Notebook->InsertPage(PAGE_ACTORS, m_Actors, _("Actor"), false);
	m_Notebook->InsertPage(PAGE_TEXTURES, m_Textures, _("Texture"), true);
	m_Notebook->InsertPage(PAGE_SOUNDS, m_Sounds, _("Sound"), false);
	m_Notebook->InsertPage(PAGE_MESHES, m_Meshes, _("Mesh"), false);

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

void BrowserWindow::SetAudioSystem(std::shared_ptr<AudioSystem>& audioSystem)
{
	m_Sounds->SetAudioSystem(audioSystem);
}

void BrowserWindow::SwitchTo(int pageNumber)
{
	m_Notebook->SetSelection(pageNumber);
	switch (pageNumber)
	{
	case PAGE_ACTORS:
		SetTitle(_("Actor Browser"));
		break;
	case PAGE_TEXTURES:
		SetTitle(_("Texture Browser"));
		break;
	case PAGE_SOUNDS:
		SetTitle(_("Sound Browser"));
		break;
	case PAGE_MESHES:
		SetTitle(_("Mesh Browser"));
		break;
	}
}

const wxString& BrowserWindow::GetTexture(void)
{
	return m_Textures->GetSelection();
}

const wxString& BrowserWindow::GetActor(void)
{
	return m_Actors->GetSelection();
}

wxString BrowserWindow::GetActorDefinition(const wxString& name)
{
	return m_Actors->GetDefinition(name);
}

const wxString& BrowserWindow::GetMesh(void)
{
	return m_Meshes->GetSelection();
}

void BrowserWindow::AddPackage(const wxString& path)
{
	for (packagelist_t::iterator i = ms_Packages.begin();
		i != ms_Packages.end(); ++i)
	{
		if ((*i) == path)
			return; // already exists
	}

	ms_Packages.push_back(path);
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
		if (BrowserWindow::ms_Packages.size() > 0)
		{
			for (BrowserWindow::packagelist_t::iterator i = BrowserWindow::ms_Packages.begin();
				i != BrowserWindow::ms_Packages.end(); ++i)
			{
				LoadPackage(*i, true);
			}
		}

		ResizePreview();
		ScrollTo(m_Selected);
	}
}

bool TextureBrowser::LoadPackage(const wxString& path, bool preload)
{
	if (!preload)
	{
		for (BrowserWindow::packagelist_t::iterator i = BrowserWindow::ms_Packages.begin();
			i != BrowserWindow::ms_Packages.end(); ++i)
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
			BrowserWindow::ms_Packages.push_back(path);

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

	wxBusyInfo wait(wxBusyInfoFlags()
		.Parent(this)
		.Title(_("Opening package"))
		.Text(_("Please wait..."))
		.Foreground(*wxBLACK)
		.Background(*wxWHITE));

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

class PropertyType : public wxClientData
{
public:
	enum PROPTYPE
	{
		STRING,
		FLOAT,
		INTEGER,
		VECTOR2,
		VECTOR3,
		VECTOR4
	} Type;

public:
	PropertyType(PROPTYPE type)
		: Type(type) {}

	wxString GetTypeAsString(void)
	{
		switch (Type)
		{
		case STRING:
			return wxT("String");
		case FLOAT:
			return wxT("Float");
		case INTEGER:
			return wxT("Integer");
		case VECTOR2:
			return wxT("Vector2");
		case VECTOR3:
			return wxT("Vector3");
		case VECTOR4:
			return wxT("Vector4");
		}

		return wxEmptyString;
	}
};

class ActorItemData : public wxTreeItemData
{
public:
	wxString Definition;

public:
	ActorItemData(wxXmlNode* actor)
	{
		wxXmlDocument doc;
		doc.SetRoot(new wxXmlNode(*actor));
		wxStringOutputStream stream;
		doc.Save(stream);
		Definition = stream.GetString();
	}

	~ActorItemData(void)
	{
	}
};

class EditActorDialog : public wxDialog
{
private:
	wxPropertyGrid* m_Properties;
	wxPGProperty* m_GeneralProperties;
	wxPGProperty* m_CustomProperties;
	wxArrayString m_ActorCategories;

	int32_t m_NextId;

public:
	EditActorDialog(wxWindow* parent, wxArrayString actorCategories,
		ActorItemData* data = nullptr)
		: wxDialog(parent, wxID_ANY, _("Add actor")),
			m_ActorCategories(actorCategories),
			m_NextId(0)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->SetMinSize(640, 480);

		wxToolBar* toolBar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
			wxTB_FLAT | wxTB_HORIZONTAL);
		toolBar->AddTool(wxID_ADD, _("Add"), wxArtProvider::GetBitmap(wxART_PLUS),
			_("Add custom property"));
		toolBar->AddTool(wxID_REMOVE, _("Delete"), wxArtProvider::GetBitmap(wxART_MINUS),
			_("Delete custom property"));
		toolBar->Realize();
		sizer->Add(toolBar, wxSizerFlags().Expand());

		m_Properties = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition,
			wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE);
		sizer->Add(m_Properties, wxSizerFlags(9).Expand());

		m_Properties->EnableCategories(true);
		m_Properties->MakeColumnEditable(0);

		m_GeneralProperties = new wxPropertyCategory(_("General"));
		m_CustomProperties = new wxPropertyCategory(_("Custom"));
		m_Properties->Append(m_GeneralProperties);
		m_Properties->Append(m_CustomProperties);

		// name
		wxStringProperty* name = new wxStringProperty(_("Name"));
		m_Properties->AppendIn(m_GeneralProperties, name);
		// category
		wxEditEnumProperty* category = new wxEditEnumProperty(
			_("Category"), wxPG_LABEL, m_ActorCategories, wxArrayInt());
		m_Properties->AppendIn(m_GeneralProperties, category);
		// type
		wxArrayString typeChoices;
		typeChoices.Add(_("Model"));
		typeChoices.Add(_("Emitter"));
		typeChoices.Add(_("Custom"));
		wxEditEnumProperty* type = new wxEditEnumProperty(_("Type"), 
			wxPG_LABEL, typeChoices, wxArrayInt());
		m_Properties->AppendIn(m_GeneralProperties, type);

		if (data)
		{
			wxStringInputStream stream(data->Definition);
			wxXmlDocument doc(stream);
			wxXmlNode* root = doc.GetRoot();

			name->SetValueFromString(root->GetAttribute(wxT("Name"),
				root->GetAttribute(wxT("name"))));
			name->ChangeFlag(wxPG_PROP_READONLY, true);
			category->SetValueFromString(root->GetAttribute(wxT("Category"),
				root->GetAttribute(wxT("category"))));
			category->ChangeFlag(wxPG_PROP_READONLY, true);
			type->SetValueFromString(root->GetAttribute(wxT("Type"),
				root->GetAttribute(wxT("type"))));
			type->ChangeFlag(wxPG_PROP_READONLY, true);

			wxXmlNode* child = root->GetChildren();
			while (child)
			{
				wxXmlAttribute* attribute = child->GetAttributes();

				// there should only be 1 attribute per node, so work with that
				AddCustomAttribute(child->GetName(), attribute->GetName(),
					attribute->GetValue());

				child = child->GetNext();
			}

			m_Properties->Expand(m_CustomProperties);
		}

		wxSizer* buttons = CreateButtonSizer(wxOK | wxCANCEL);
		sizer->Add(buttons, wxSizerFlags(1).Expand().Border(wxALL));

		SetSizerAndFit(sizer);

		Bind(wxEVT_BUTTON, &EditActorDialog::OnOKEvent, this, wxID_OK);
		Bind(wxEVT_MENU, &EditActorDialog::OnToolAdd, this, wxID_ADD);
		Bind(wxEVT_MENU, &EditActorDialog::OnToolRemove, this, wxID_REMOVE);
		m_Properties->Bind(wxEVT_PG_LABEL_EDIT_BEGIN, &EditActorDialog::OnLabelEditBegin,
			this);
		m_Properties->Bind(wxEVT_PG_CHANGED, &EditActorDialog::OnPropertyChanged, this);
	}

	~EditActorDialog(void)
	{
	}

	wxXmlDocument GetDefinition(void)
	{
		wxXmlDocument doc;
		wxXmlNode* actor = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, wxT("Actor"));
		doc.SetRoot(actor);

		wxXmlNode* custom = nullptr;

		wxPropertyGridIterator iter = m_Properties->GetIterator();
		while (!iter.AtEnd())
		{
			wxPGProperty* prop = iter.GetProperty();
			if (prop->GetParent() == m_GeneralProperties)
			{
				actor->AddAttribute(prop->GetLabel(), prop->GetValueAsString());
			}
			else if (prop->GetParent() == m_CustomProperties)
			{
				PropertyType* type = dynamic_cast<PropertyType*>(prop->GetClientObject());
				wxXmlNode* custom = new wxXmlNode(actor, wxXML_ELEMENT_NODE, 
					type->GetTypeAsString());
				custom->AddAttribute(prop->GetLabel(), prop->GetValueAsString());
			}

			iter.Next(false);
		}

		return doc;
	}

	void OnOKEvent(wxCommandEvent& event)
	{
		wxPropertyGridIterator iter = m_Properties->GetIterator();
		while (!iter.AtEnd())
		{
			wxPGProperty* prop = iter.GetProperty();
			if (prop->GetLabel().CompareTo(_("Mesh")) != 0 &&
				prop->GetParent() != m_CustomProperties &&
				prop->GetValueAsString().IsEmpty())
			{
				wxMessageBox(_("Name and Category must be set"), _("Information required"));
				return;
			}

			iter.Next(false);
		}

		event.Skip();
	}

	void OnLabelEditBegin(wxPropertyGridEvent& event)
	{
		wxPGProperty* parent = event.GetProperty()->GetParent();
		if (event.GetProperty()->IsCategory() ||
			parent->IsCategory() && parent->GetLabel() == _("General"))
			event.Veto(); // we do not allow editing labels under General
	}

	void OnPropertyChanged(wxPropertyGridEvent& event)
	{
		wxPGProperty* prop = event.GetProperty();
		if (prop->GetLabel().CompareTo(_("Type"), wxString::ignoreCase) == 0)
		{
			// Add mesh and texture custom properties if type is Model
			if (prop->GetValueAsString().CompareTo(_("Model"), wxString::ignoreCase) == 0)
			{
				// remove emitter custom properties
				RemoveCustomAttribute(_("Emitter"));

				// add mesh and texture custom properties
				AddCustomAttribute(_("String"), _("Mesh"));
				AddCustomAttribute(_("String"), _("Texture"));
			}
			else if (prop->GetValueAsString().CompareTo(_("Emitter"), wxString::ignoreCase) == 0)
			{
				// remove mesh and texture custom properties
				RemoveCustomAttribute(_("Mesh"));
				RemoveCustomAttribute(_("Texture"));

				// add emitter custom properties
				AddCustomAttribute(_("String"), _("Emitter"));
			}
			else if (prop->GetValueAsString().CompareTo(_("Custom"), wxString::ignoreCase) == 0)
			{
				// remove mesh and texture custom properties
				RemoveCustomAttribute(_("Mesh"));
				RemoveCustomAttribute(_("Texture"));

				// remove emitter custom properties
				RemoveCustomAttribute(_("Emitter"));
			}
		}
	}

	void OnToolAdd(wxCommandEvent& event)
	{
		wxArrayString propertyChoices;
		propertyChoices.Add(_("String"));
		propertyChoices.Add(_("Float"));
		propertyChoices.Add(_("Integer"));
		propertyChoices.Add(_("Vector2"));
		propertyChoices.Add(_("Vector3"));
		propertyChoices.Add(_("Vector4"));

		wxSingleChoiceDialog dialog(this, _("Select property type"),
			_("Add custom property"), propertyChoices, nullptr, wxOK | wxCANCEL | wxCENTRE);
		if (dialog.ShowModal() == wxID_OK)
		{
			wxString propName;
			do
			{
				propName = wxString::Format(_("Custom%d"), ++m_NextId);
				wxPGProperty* prop = m_Properties->GetPropertyByName(propName);
				if (prop == nullptr)
					break;

			} while (true);

			wxString selection = dialog.GetStringSelection();
			AddCustomAttribute(selection, propName);

			// m_Properties->Expand(m_CustomProperties);
		}
	}

	void OnToolRemove(wxCommandEvent& event)
	{
		wxPGProperty* selection = m_Properties->GetSelection();
		if (selection->IsCategory() ||
			selection->GetParent() == m_GeneralProperties)
			return;

		m_Properties->DeleteProperty(selection);
	}

	void AddCustomAttribute(const wxString& type, const wxString& name,
		const wxString& value = wxEmptyString)
	{
		if (type.CompareTo(_("String"), wxString::ignoreCase) == 0)
		{
			wxPGProperty* prop = new wxStringProperty(name);
			prop->SetValueFromString(value);
			prop->SetClientObject(new PropertyType(PropertyType::STRING));
			m_Properties->AppendIn(m_CustomProperties, prop);
		}
		else if (type.CompareTo(_("Float"), wxString::ignoreCase) == 0)
		{
			wxPGProperty* prop = new wxFloatProperty(name);
			prop->SetValueFromString(value);
			prop->SetClientObject(new PropertyType(PropertyType::FLOAT));
			m_Properties->AppendIn(m_CustomProperties, prop);
		}
		else if (type.CompareTo(_("Integer"), wxString::ignoreCase) == 0)
		{
			wxPGProperty* prop = new wxIntProperty(name);
			prop->SetValueFromString(value);
			prop->SetClientObject(new PropertyType(PropertyType::INTEGER));
			m_Properties->AppendIn(m_CustomProperties, prop);
		}
		else if (type.CompareTo(_("Vector2"), wxString::ignoreCase) == 0)
		{
			wxPGProperty* prop = m_Properties->AppendIn(m_CustomProperties,
				new wxStringProperty(name, wxPG_LABEL, "<composed>"));
			prop->SetClientObject(new PropertyType(PropertyType::VECTOR2));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("x")));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("y")));
			prop->SetValueFromString(value);
			m_Properties->Collapse(prop);
		}
		else if (type.CompareTo(_("Vector3"), wxString::ignoreCase) == 0)
		{
			wxPGProperty* prop = m_Properties->AppendIn(m_CustomProperties,
				new wxStringProperty(name, wxPG_LABEL, "<composed>"));
			prop->SetClientObject(new PropertyType(PropertyType::VECTOR3));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("x")));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("y")));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("z")));
			prop->SetValueFromString(value);
			m_Properties->Collapse(prop);
		}
		else if (type.CompareTo(_("Vector4"), wxString::ignoreCase) == 0)
		{
			wxPGProperty* prop = m_Properties->AppendIn(m_CustomProperties,
				new wxStringProperty(name, wxPG_LABEL, "<composed>"));
			prop->SetClientObject(new PropertyType(PropertyType::VECTOR4));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("x")));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("y")));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("z")));
			m_Properties->AppendIn(prop, new wxFloatProperty(_("w")));
			prop->SetValueFromString(value);
			m_Properties->Collapse(prop);
		}

		m_Properties->Expand(m_CustomProperties);
	}

	void RemoveCustomAttribute(const wxString& name)
	{
		wxPGProperty* prop = m_Properties->GetPropertyByName(name);
		if (prop)
			m_Properties->DeleteProperty(prop);
	}
};

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
	tools->AddSeparator();
	tools->AddTool(wxID_SAVE, _("Save"), wxArtProvider::GetBitmap(wxART_FILE_SAVE),
		_("Save actor definition"));
	tools->AddTool(wxID_SAVEAS, _("Save As"), wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS),
		_("Save actor definition as"));
	tools->Realize();

	m_Tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxTR_HAS_BUTTONS | wxTR_SINGLE);
	m_Root = m_Tree->AddRoot(_("Actors"));
	m_Tree->Expand(m_Root);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(tools, wxSizerFlags(1).Expand());
	sizer->Add(m_Tree, wxSizerFlags(9).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_MENU, &ActorBrowser::OnToolAdd, this, wxID_NEW);
	Bind(wxEVT_MENU, &ActorBrowser::OnToolOpen, this, wxID_OPEN);
	Bind(wxEVT_MENU, &ActorBrowser::OnToolSave, this, wxID_SAVE);
	Bind(wxEVT_MENU, &ActorBrowser::OnToolSaveAs, this, wxID_SAVEAS);
	m_Tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &ActorBrowser::OnItemActivate, this);
	m_Tree->Bind(wxEVT_TREE_SEL_CHANGED, &ActorBrowser::OnItemSelected, this);

	// for (BrowserWindow::packagelist_t::iterator i = BrowserWindow::ms_Packages.begin();
	// 	i != BrowserWindow::ms_Packages.end(); ++i)
	// {
	// 	LoadPackage(*i, true);
	// }
}

ActorBrowser::~ActorBrowser(void)
{
}

const wxString& ActorBrowser::GetSelection(void)
{
	return m_Selected;
}

wxString ActorBrowser::GetDefinition(const wxString& name)
{
	wxTreeItemId actorId = FindItem(name, m_Root);
	if (actorId.IsOk())
	{
		ActorItemData* data = dynamic_cast<ActorItemData*>(m_Tree->GetItemData(actorId));
		if (data)
			return data->Definition;
	}

	return wxEmptyString;
}

void ActorBrowser::OnToolAdd(wxCommandEvent& event)
{
	EditActorDialog dialog(this, m_Categories);
	if (dialog.ShowModal() == wxID_OK)
	{
		wxXmlDocument doc = dialog.GetDefinition();
		AddActor(doc.GetRoot());
	}
}

void ActorBrowser::OnToolOpen(wxCommandEvent& event)
{
	wxFileDialog dialog(this,
		_("Open Actor definition file"), wxEmptyString,
		wxT("Actors.xml"), _("Actor Definition (*.xml)|*.xml"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dialog.ShowModal() == wxID_CANCEL)
		return;

	wxXmlDocument doc;
	if (!doc.Load(dialog.GetPath()) ||
		doc.GetRoot()->GetName().CompareTo(wxT("Actors"), wxString::ignoreCase) != 0)
	{
		wxLogWarning(_("Invalid Actor Definition file: %s"), dialog.GetPath());
		return;
	}

	m_Tree->DeleteChildren(m_Root);

	wxXmlNode* actor = doc.GetRoot()->GetChildren();
	while (actor)
	{
		AddActor(actor);
		actor = actor->GetNext();
	}

	m_DefinitionFile = wxFileName(dialog.GetPath());
}

void ActorBrowser::OnToolSave(wxCommandEvent& event)
{
	if (!m_DefinitionFile.IsOk())
	{
		OnToolSaveAs(event);
		return;
	}

	wxXmlDocument doc;
	wxXmlNode* root = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, wxT("Actors"));
	doc.SetRoot(root);

	// walk the actor tree
	wxTreeItemIdValue cookie;
	wxTreeItemId item = m_Tree->GetFirstChild(m_Root, cookie);
	while (item.IsOk())
	{
		wxTreeItemIdValue childCookie;
		wxTreeItemId child = m_Tree->GetFirstChild(item, childCookie);
		while (child.IsOk())
		{
			ActorItemData* data = dynamic_cast<ActorItemData*>(m_Tree->GetItemData(child));
			if (data)
			{
				wxStringInputStream stream(data->Definition);
				wxXmlDocument _doc(stream);
				wxXmlNode* node = _doc.GetRoot();

				// append to the document
				root->AddChild(new wxXmlNode(*node));
			}

			child = m_Tree->GetNextChild(child, childCookie);
		}

		item = m_Tree->GetNextChild(item, cookie);
	}

	doc.Save(m_DefinitionFile.GetFullPath());
}

void ActorBrowser::OnToolSaveAs(wxCommandEvent& event)
{
	wxFileDialog dialog(this,
		_("Save Actor Definition As..."), 
		wxEmptyString,
		wxT("Actors.xml"),
		_("Actor Definition (*.xml)|*.xml"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dialog.ShowModal() == wxID_CANCEL)
		return; // not saving today

	m_DefinitionFile = wxFileName(dialog.GetPath());
	OnToolSave(event);
}

void ActorBrowser::OnItemActivate(wxTreeEvent& event)
{
	ActorItemData* data = dynamic_cast<ActorItemData*>(
		m_Tree->GetItemData(event.GetItem()));
	if (data)
	{
		EditActorDialog dialog(this, m_Categories, data);
		if (dialog.ShowModal() == wxID_OK)
		{
			wxXmlDocument doc = dialog.GetDefinition();
			AddActor(doc.GetRoot());
		}
	}
	else
		event.Skip();
}

void ActorBrowser::OnItemSelected(wxTreeEvent& event)
{
	wxTreeItemId item = event.GetItem();
	if (item.IsOk() &&
		item != m_Root &&
		!m_Tree->HasChildren(item))
	{
		m_Selected = m_Tree->GetItemText(item);
	}
	else
		m_Selected = wxEmptyString;
}

void ActorBrowser::AddActor(wxXmlNode* actor)
{
	if (actor->GetName().CompareTo(wxT("Actor"), wxString::ignoreCase) == 0)
	{
		wxString category = actor->GetAttribute(wxT("Category"));
		wxTreeItemId categoryId = FindItem(category, m_Root);
		if (!categoryId.IsOk())
		{
			m_Categories.Add(category);
			categoryId = m_Tree->AppendItem(m_Root, category);
		}

		// try to find the actor first, maybe it already exists and we are updating it
		wxTreeItemId actorId = FindItem(actor->GetAttribute(wxT("Name")), categoryId);
		if (actorId.IsOk())
		{
			// updating
			delete m_Tree->GetItemData(actorId);
			m_Tree->SetItemData(actorId, new ActorItemData(actor));
		}
		else
		{
			actorId = m_Tree->AppendItem(categoryId,
				actor->GetAttribute(wxT("Name")));
			m_Tree->SetItemData(actorId, new ActorItemData(actor));
			m_Tree->EnsureVisible(actorId);
		}
	}
}

wxTreeItemId ActorBrowser::FindItem(const wxString& name, wxTreeItemId& start)
{
	// First check direct children
	wxTreeItemIdValue cookie;
	wxTreeItemId item = m_Tree->GetFirstChild(start, cookie);
	while (item.IsOk())
	{
		wxString itemName = m_Tree->GetItemText(item);
		if (itemName.CompareTo(name, wxString::ignoreCase) == 0)
			return item;

		// Recursively search children of this item
		wxTreeItemId found = FindItem(name, item);
		if (found.IsOk())
			return found;

		item = m_Tree->GetNextSibling(item);
	}

	return wxTreeItemId(); // not found
}

SoundBrowser::SoundBrowser(wxWindow* parent)
	: wxPanel(parent)
{
	wxFileSystem fs;

	// create the toolbar
	wxToolBar* tools = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_HORIZONTAL);
	tools->AddTool(wxID_NEW, _("Add"), wxArtProvider::GetBitmap(wxART_NEW),
		_("Add sound"));
	tools->AddTool(wxID_OPEN, _("Open"), wxArtProvider::GetBitmap(wxART_FILE_OPEN),
		_("Open package"));
	tools->AddSeparator();

	wxVector<wxBitmap> playTool;
	playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play32.png", wxBITMAP_TYPE_PNG));
	playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play48.png", wxBITMAP_TYPE_PNG));
	playTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/play64.png", wxBITMAP_TYPE_PNG));
	tools->AddTool(MENU_PLAYSOUND, _("Play sound"), wxBitmapBundle::FromBitmaps(playTool),
		_("Play Sound"));

	wxVector<wxBitmap> stopTool;
	stopTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/stop32.png", wxBITMAP_TYPE_PNG));
	stopTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/stop48.png", wxBITMAP_TYPE_PNG));
	stopTool.push_back(BitmapFromFS(fs, "editor.mpk:icons/stop64.png", wxBITMAP_TYPE_PNG));
	tools->AddTool(MENU_STOPSOUND, _("Stop sound"), wxBitmapBundle::FromBitmaps(stopTool),
		_("Stop playing sound"));
	tools->Realize();

	m_List = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_VRULES);
	m_List->AppendColumn(_("Path"));
	m_List->AppendColumn(_("Type"));
	m_List->AppendColumn(_("Channels"));
	m_List->AppendColumn(_("Frequency"));
	m_List->AppendColumn(_("Package"));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(tools, wxSizerFlags(1).Expand());
	sizer->Add(m_List, wxSizerFlags(9).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_MENU, &SoundBrowser::OnToolAdd, this, wxID_NEW);
	Bind(wxEVT_MENU, &SoundBrowser::OnToolOpen, this, wxID_OPEN);
	Bind(wxEVT_MENU, &SoundBrowser::OnToolPlay, this, MENU_PLAYSOUND);
	Bind(wxEVT_MENU, &SoundBrowser::OnToolStop, this, MENU_STOPSOUND);
	m_List->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SoundBrowser::OnItemActivate, this);
}

SoundBrowser::~SoundBrowser(void)
{
}

void SoundBrowser::SetAudioSystem(std::shared_ptr<AudioSystem>& audioSystem)
{
	m_AudioSystem = audioSystem;
	if (m_AudioSystem)
	{
		// did we pre-load any packages?
		if (BrowserWindow::ms_Packages.size() > 0)
		{
			for (BrowserWindow::packagelist_t::iterator i = BrowserWindow::ms_Packages.begin();
				i != BrowserWindow::ms_Packages.end(); ++i)
			{
				LoadPackage(*i, true);
			}
		}
	}
}

bool SoundBrowser::LoadPackage(const wxString& path, bool preload)
{
	if (!preload)
	{
		for (BrowserWindow::packagelist_t::iterator i = BrowserWindow::ms_Packages.begin();
			i != BrowserWindow::ms_Packages.end(); ++i)
		{
			if ((*i) == path)
				return true; // already exists
		}
	}

	wxFileName _path(path);
	wxFileInputStream inStream(_path.GetFullPath());
	if (inStream.IsOk())
	{
		wxZipInputStream zipStream(inStream);
		if (!zipStream.IsOk())
		{
			wxLogWarning(_("Unsupported archive: %s"), _path.GetFullPath());
			return false;
		}

		wxZipEntry* entry = zipStream.GetNextEntry();
		while (entry)
		{
			wxString entryPath(entry->GetName());

			// support archives made on any platform
			if (entryPath.StartsWith(wxT("sounds/")) ||
				entryPath.StartsWith(wxT("sounds\\")) ||
				entryPath.StartsWith(wxT("music/")) ||
				entryPath.StartsWith(wxT("music\\")))
			{
				// build the full path
				wxString sndPath(_path.GetFullPath());
				if (_path.GetExt().CmpNoCase(wxT("zip")) == 0)
					sndPath.append(wxT("#zip"));
				// else if (path.GetExt().CmpNoCase(wxT("mpk")) == 0)
				// 	sndPath.append(wxT("#mpk"));

				sndPath.append(wxT(":"));
				sndPath.append(entryPath);

				// add this to the list
				long index = m_List->InsertItem(m_List->GetItemCount(), entry->GetName());
				m_List->SetItemData(index, -1);
				m_ItemPaths[index] = sndPath;

				wxFileName fn(entry->GetName());
				wxFileType* mimeType = wxTheMimeTypesManager->GetFileTypeFromExtension(fn.GetExt());
				if (mimeType)
				{
					wxString type;
					if (mimeType->GetMimeType(&type))
						m_List->SetItem(index, COL_TYPE, type);
					else
						m_List->SetItem(index, COL_TYPE, _("Unknown"));

					delete mimeType;
				}
				else
					m_List->SetItem(index, COL_TYPE, _("Unknown"));

				// get the meta data for the item
				uint32_t sampleRate, channels;
				m_AudioSystem->getSoundMetadata(sndPath, sampleRate, channels);
				m_List->SetItem(index, COL_CHANNELS, wxString::Format(_("%d"), channels));
				m_List->SetItem(index, COL_FREQ, wxString::Format(_("%d"), sampleRate));
				m_List->SetItem(index, COL_PACKAGE, _path.GetFullPath());
			}

			entry->UnRef();
			entry = zipStream.GetNextEntry();
		}
	}

	return true;
}

void SoundBrowser::OnToolAdd(wxCommandEvent& event)
{
}

void SoundBrowser::OnToolOpen(wxCommandEvent& event)
{
	wxFileDialog openDialog(this,
		_("Open package"), wxEmptyString, wxEmptyString,
		_("Manifold Archive Package (*.mpk)|*.mpk|Zip Archive (*.zip)|*.zip"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openDialog.ShowModal() == wxID_CANCEL)
		return; // not opening today

	wxBusyInfo wait(wxBusyInfoFlags()
		.Parent(this)
		.Title(_("Opening package"))
		.Text(_("Please wait..."))
		.Foreground(*wxBLACK)
		.Background(*wxWHITE));

	LoadPackage(openDialog.GetPath(), false);
}

void SoundBrowser::OnToolPlay(wxCommandEvent& event)
{
	long index = m_List->GetFocusedItem();
	if (index != -1)
	{
		m_AudioSystem->stopSound(); // stop any currently playing sound
		m_AudioSystem->playSound(m_ItemPaths[index]);
	}
}

void SoundBrowser::OnToolStop(wxCommandEvent& event)
{
	m_AudioSystem->stopSound();
}

void SoundBrowser::OnItemActivate(wxListEvent& event)
{
	long index = event.GetIndex();
	if (index != -1)
	{
		m_AudioSystem->stopSound(); // stop any currently playing sound
		m_AudioSystem->playSound(m_ItemPaths[index]);
	}
}

MeshBrowser::MeshBrowser(wxWindow* parent)
	: wxPanel(parent)
{
	// create the toolbar
	wxToolBar* tools = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxTB_FLAT | wxTB_HORIZONTAL);
	tools->AddTool(wxID_NEW, _("Add"), wxArtProvider::GetBitmap(wxART_NEW),
		_("Add mesh"));
	tools->AddTool(wxID_OPEN, _("Open"), wxArtProvider::GetBitmap(wxART_FILE_OPEN),
		_("Open package"));
	tools->Realize();

	m_List = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_VRULES);
	m_List->AppendColumn(_("Path"));
	m_List->AppendColumn(_("Type"));
	m_List->AppendColumn(_("Package"));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(tools, wxSizerFlags(1).Expand());
	sizer->Add(m_List, wxSizerFlags(9).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_MENU, &MeshBrowser::OnToolAdd, this, wxID_NEW);
	Bind(wxEVT_MENU, &MeshBrowser::OnToolOpen, this, wxID_OPEN);
	m_List->Bind(wxEVT_LIST_ITEM_SELECTED, &MeshBrowser::OnItemSelected, this);

	for (BrowserWindow::packagelist_t::iterator i = BrowserWindow::ms_Packages.begin();
		i != BrowserWindow::ms_Packages.end(); ++i)
	{
		LoadPackage(*i, true);
	}
}

MeshBrowser::~MeshBrowser(void)
{
}

bool MeshBrowser::LoadPackage(const wxString& path, bool preload)
{
	if (!preload)
	{
		for (BrowserWindow::packagelist_t::iterator i = BrowserWindow::ms_Packages.begin();
			i != BrowserWindow::ms_Packages.end(); ++i)
		{
			if ((*i) == path)
				return true; // already exists
		}
	}

	wxFileName _path(path);
	wxFileInputStream inStream(_path.GetFullPath());
	if (inStream.IsOk())
	{
		wxZipInputStream zipStream(inStream);
		if (!zipStream.IsOk())
		{
			wxLogWarning(_("Unsupported archive: %s"), _path.GetFullPath());
			return false;
		}

		wxZipEntry* entry = zipStream.GetNextEntry();
		while (entry)
		{
			wxString entryPath(entry->GetName());

			// support archives made on any platform
			if (entryPath.StartsWith(wxT("mesh/")) ||
				entryPath.StartsWith(wxT("mesh\\")) ||
				entryPath.StartsWith(wxT("models/")) ||
				entryPath.StartsWith(wxT("models\\")))
			{
				// build the full path
				wxString meshPath(_path.GetFullPath());
				if (_path.GetExt().CmpNoCase(wxT("zip")) == 0)
					meshPath.append(wxT("#zip"));
				// else if (_path.GetExt().CmpNoCase(wxT("mpk")) == 0)
				// 	meshPath.append(wxT("#mpk"));

				meshPath.append(wxT(":"));
				meshPath.append(entryPath);

				// add this to the list
				long index = m_List->InsertItem(m_List->GetItemCount(), entry->GetName());
				m_List->SetItemData(index, -1);
				m_ItemPaths[index] = meshPath;

				wxFileName fn(entry->GetName());
				wxFileType* mimeType = wxTheMimeTypesManager->GetFileTypeFromExtension(fn.GetExt());
				if (mimeType)
				{
					wxString type;
					if (mimeType->GetMimeType(&type))
						m_List->SetItem(index, COL_TYPE, type);
					else
						m_List->SetItem(index, COL_TYPE, _("Unknown"));

					delete mimeType;
				}
				else
					m_List->SetItem(index, COL_TYPE, _("Unknown"));

				m_List->SetItem(index, COL_PACKAGE, _path.GetFullPath());
			}

			entry->UnRef();
			entry = zipStream.GetNextEntry();
		}
	}

	return true;
}

void MeshBrowser::OnToolAdd(wxCommandEvent& event)
{
}

void MeshBrowser::OnToolOpen(wxCommandEvent& event)
{
	wxFileDialog openDialog(this,
		_("Open package"), wxEmptyString, wxEmptyString,
		_("Manifold Archive Package (*.mpk)|*.mpk|Zip Archive (*.zip)|*.zip"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openDialog.ShowModal() == wxID_CANCEL)
		return; // not opening today

	wxBusyInfo wait(wxBusyInfoFlags()
		.Parent(this)
		.Title(_("Opening package"))
		.Text(_("Please wait..."))
		.Foreground(*wxBLACK)
		.Background(*wxWHITE));

	LoadPackage(openDialog.GetPath(), false);	
}

void MeshBrowser::OnItemSelected(wxListEvent& event)
{
	wxFileName selection(m_List->GetItemText(event.GetIndex()));
	m_Selected = selection.GetName();
}

const wxString& MeshBrowser::GetSelection(void)
{
	return m_Selected;
}
