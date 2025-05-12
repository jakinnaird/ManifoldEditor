/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Commands.hpp"
#include "MapEditor.hpp"
#include "PropertyPanel.hpp"
#include "../extend/CylinderSceneNode.hpp"
#include "../extend/PlaneSceneNode.hpp"
#include "../extend/PathSceneNode.hpp"
#include "ViewPanel.hpp"

#include <wx/artprov.h>
#include <wx/log.h>
#include <wx/propgrid/advprops.h>
#include <wx/sizer.h>

#include <sstream>

class PropertyClientData : public wxClientData
{
public:
	PropertyClientData(irr::io::E_ATTRIBUTE_TYPE type)
		: m_Type(type)
	{
	}

	irr::io::E_ATTRIBUTE_TYPE m_Type;
};

PropertyPanel::PropertyPanel(wxWindow* parent, wxCommandProcessor& cmdProc)
	: wxPanel(parent), m_Commands(cmdProc), m_Properties(nullptr),
	  m_SceneNode(nullptr), m_PosX(nullptr), m_PosY(nullptr), m_PosZ(nullptr)
{
	m_ToolBar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxTB_FLAT | wxTB_HORIZONTAL);
	m_ToolBar->AddTool(wxID_ADD, _("Add"), wxArtProvider::GetBitmap(wxART_PLUS),
		_("Add property"));
	m_ToolBar->AddTool(wxID_REMOVE, _("Delete"), wxArtProvider::GetBitmap(wxART_MINUS),
		_("Delete property"));
	m_ToolBar->Realize();

	m_Properties = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE);

	m_Properties->EnableCategories(true);
	m_Properties->MakeColumnEditable(0);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_ToolBar, wxSizerFlags(1).Expand());
	sizer->Add(m_Properties, wxSizerFlags(9).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_MENU, &PropertyPanel::OnToolAdd, this, wxID_ADD);
	Bind(wxEVT_MENU, &PropertyPanel::OnToolRemove, this, wxID_REMOVE);
	Bind(wxEVT_PG_CHANGING, &PropertyPanel::OnValueChanging, this);
	Bind(wxEVT_PG_CHANGED, &PropertyPanel::OnValueChanged, this);
}

PropertyPanel::~PropertyPanel(void)
{
}

void PropertyPanel::SetMap(std::shared_ptr<Map>& map)
{
	m_Map = map;
}

void PropertyPanel::Clear(void)
{
	m_PosX = m_PosY = m_PosZ = nullptr;
	m_Properties->Clear();
	m_SceneNode = nullptr;
}

void PropertyPanel::Refresh(void)
{
	if (m_SceneNode == nullptr)
		return;

	if (m_Properties->GetRoot()->GetChildCount() == 0) // first time
	{
		irr::io::SAttributeReadWriteOptions opts;
		opts.Filename = ".";
		opts.Flags = irr::io::EARWF_USE_RELATIVE_PATHS;
		irr::io::IAttributes* attribs = m_SceneNode->getSceneManager()->getFileSystem()->createEmptyAttributes(nullptr);
		m_SceneNode->serializeAttributes(attribs, &opts);

		m_GeneralProperties = new wxPropertyCategory(_("General"));
		m_CustomProperties = new wxPropertyCategory(_("Custom"));
		m_Properties->Append(m_GeneralProperties);
		m_Properties->Append(m_CustomProperties);

		// name
		wxStringProperty* name = new wxStringProperty(_("Name"));
		name->SetValueFromString(m_SceneNode->getName());
		m_Properties->AppendIn(m_GeneralProperties, name);

		// build the position
		irr::core::vector3df pos = m_SceneNode->getAbsolutePosition();
		wxPGProperty* position = m_Properties->AppendIn(m_GeneralProperties, new wxStringProperty(_("Position"),
			wxPG_LABEL, "<composed>"));
		m_PosX = new wxFloatProperty(_("x"), wxPG_LABEL, pos.X);
		m_Properties->AppendIn(position, m_PosX);
		m_PosY = new wxFloatProperty(_("y"), wxPG_LABEL, pos.Y);
		m_Properties->AppendIn(position, m_PosY);
		m_PosZ = new wxFloatProperty(_("z"), wxPG_LABEL, pos.Z);
		m_Properties->AppendIn(position, m_PosZ);
		m_Properties->Collapse(position);

		// build the rotation
		irr::core::vector3df rot = m_SceneNode->getRotation();
		wxPGProperty* rotation = m_Properties->AppendIn(m_GeneralProperties, new wxStringProperty(_("Rotation"),
			wxPG_LABEL, "<composed>"));
		m_Properties->AppendIn(rotation, new wxFloatProperty(_("x"), wxPG_LABEL, rot.X));
		m_Properties->AppendIn(rotation, new wxFloatProperty(_("y"), wxPG_LABEL, rot.Y));
		m_Properties->AppendIn(rotation, new wxFloatProperty(_("z"), wxPG_LABEL, rot.Z));
		m_Properties->Collapse(rotation);

		// scale
		irr::core::vector3df _scale = m_SceneNode->getScale();
		wxPGProperty* scale = m_Properties->AppendIn(m_GeneralProperties, new wxStringProperty(_("Scale"),
			wxPG_LABEL, "<composed>"));
		m_Properties->AppendIn(scale, new wxFloatProperty(_("x"), wxPG_LABEL, _scale.X));
		m_Properties->AppendIn(scale, new wxFloatProperty(_("y"), wxPG_LABEL, _scale.Y));
		m_Properties->AppendIn(scale, new wxFloatProperty(_("z"), wxPG_LABEL, _scale.Z));
		m_Properties->Collapse(scale);

		// custom based on node type
		switch (m_SceneNode->getType())
		{
		case irr::scene::ESNT_CUBE:
			m_Properties->AppendIn(m_GeneralProperties, new wxFloatProperty(_("Size"), wxPG_LABEL,
				attribs->getAttributeAsFloat("Size")));
			break;
		case irr::scene::ESNT_SPHERE:
		{
			wxPGProperty* size = m_Properties->AppendIn(m_GeneralProperties, new wxStringProperty(_("Size"),
				wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(size, new wxFloatProperty(_("radius"), wxPG_LABEL, 
				attribs->getAttributeAsFloat("Radius")));
			m_Properties->AppendIn(size, new wxIntProperty(_("polyCountX"), wxPG_LABEL, 
				attribs->getAttributeAsInt("PolyCountX")));
			m_Properties->AppendIn(size, new wxIntProperty(_("polyCountY"), wxPG_LABEL, 
				attribs->getAttributeAsInt("PolyCountY")));
			m_Properties->Collapse(size);
		} break;
		case ESNT_CYLINDER:
		{
			wxPGProperty* size = m_Properties->AppendIn(m_GeneralProperties, new wxStringProperty(_("Size"),
				wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(size, new wxFloatProperty(_("radius"), wxPG_LABEL, 
				attribs->getAttributeAsFloat("Radius")));
			m_Properties->AppendIn(size, new wxFloatProperty(_("length"), wxPG_LABEL, 
				attribs->getAttributeAsFloat("Length")));
			m_Properties->AppendIn(size, new wxIntProperty(_("tessalation"), wxPG_LABEL, 
				attribs->getAttributeAsInt("Tesselation")));
			m_Properties->Collapse(size);
		} break;
		case ESNT_PLANE:
		{
			irr::core::vector2df size(attribs->getAttributeAsVector2d("TileSize"));
			const irr::core::dimension2df _tileSize(size.X, size.Y);
			const irr::core::dimension2du _tileCount(attribs->getAttributeAsDimension2d("TileCount"));

			wxPGProperty* count = m_Properties->AppendIn(m_GeneralProperties, new wxStringProperty(_("Tile Count"),
				wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(count, new wxUIntProperty(_("x"), wxPG_LABEL, _tileCount.Width));
			m_Properties->AppendIn(count, new wxUIntProperty(_("y"), wxPG_LABEL, _tileCount.Height));
			m_Properties->Collapse(count);

			wxPGProperty* sizeProp = m_Properties->AppendIn(m_GeneralProperties, new wxStringProperty(_("Tile Size"),
				wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(sizeProp, new wxFloatProperty(_("x"), wxPG_LABEL, _tileSize.Width));
			m_Properties->AppendIn(sizeProp, new wxFloatProperty(_("y"), wxPG_LABEL, _tileSize.Height));
			m_Properties->Collapse(sizeProp);
		} break;
		case irr::scene::ESNT_LIGHT:
		{
			m_Properties->AppendIn(m_GeneralProperties, new wxFloatProperty(_("Radius"), wxPG_LABEL,
				attribs->getAttributeAsFloat("Radius")));

			irr::video::SColorf color = attribs->getAttributeAsColor("AmbientColor");
			wxPGProperty* ambient = m_Properties->AppendIn(m_GeneralProperties,
				new wxStringProperty(_("Ambient"), wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(ambient, new wxUIntProperty(_("Alpha"), wxPG_LABEL,
				color.a));
			m_Properties->AppendIn(ambient, new wxUIntProperty(_("Red"), wxPG_LABEL,
				color.r));
			m_Properties->AppendIn(ambient, new wxUIntProperty(_("Green"), wxPG_LABEL,
				color.g));
			m_Properties->AppendIn(ambient, new wxUIntProperty(_("Blue"), wxPG_LABEL,
				color.b));
			m_Properties->Collapse(ambient);

			color = attribs->getAttributeAsColor("DiffuseColor");
			wxPGProperty* diffuse = m_Properties->AppendIn(m_GeneralProperties,
				new wxStringProperty(_("Diffuse"), wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(diffuse, new wxUIntProperty(_("Alpha"), wxPG_LABEL,
				color.a));
			m_Properties->AppendIn(diffuse, new wxUIntProperty(_("Red"), wxPG_LABEL,
				color.r));
			m_Properties->AppendIn(diffuse, new wxUIntProperty(_("Green"), wxPG_LABEL,
				color.g));
			m_Properties->AppendIn(diffuse, new wxUIntProperty(_("Blue"), wxPG_LABEL,
				color.b));
			m_Properties->Collapse(diffuse);

			color = attribs->getAttributeAsColor("SpecularColor");
			wxPGProperty* specular = m_Properties->AppendIn(m_GeneralProperties,
				new wxStringProperty(_("Specular"), wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(specular, new wxUIntProperty(_("Alpha"), wxPG_LABEL,
				color.a));
			m_Properties->AppendIn(specular, new wxUIntProperty(_("Red"), wxPG_LABEL,
				color.r));
			m_Properties->AppendIn(specular, new wxUIntProperty(_("Green"), wxPG_LABEL,
				color.g));
			m_Properties->AppendIn(specular, new wxUIntProperty(_("Blue"), wxPG_LABEL,
				color.b));
			m_Properties->Collapse(specular);
		} break;
		case irr::scene::ESNT_SKY_DOME:
		{
			m_Properties->AppendIn(m_GeneralProperties, new wxFloatProperty(_("Radius"), wxPG_LABEL,
				attribs->getAttributeAsFloat("Radius")));
			m_Properties->AppendIn(m_GeneralProperties, new wxFloatProperty(_("Arc"), wxPG_LABEL,
				attribs->getAttributeAsFloat("SpherePercentage")));
			m_Properties->AppendIn(m_GeneralProperties, new wxIntProperty(_("HorizontalResolution"), wxPG_LABEL,
				attribs->getAttributeAsInt("HorizontalResolution")));
			m_Properties->AppendIn(m_GeneralProperties, new wxIntProperty(_("VerticalResolution"), wxPG_LABEL,
				attribs->getAttributeAsInt("VerticalResolution")));
		} break;
		case ESNT_PATHNODE:
		{
			// build the list of path node names
			irr::scene::ISceneManager* smgr = m_SceneNode->getSceneManager();
			irr::core::array<irr::scene::ISceneNode*> nodes;
			smgr->getSceneNodesFromType((irr::scene::ESCENE_NODE_TYPE)ESNT_PATHNODE, 
				nodes, nullptr);

			PathSceneNode* pathNode = dynamic_cast<PathSceneNode*>(m_SceneNode);

			wxArrayString pathNames, nodeNames;
			nodeNames.push_back(wxT("--none--"));

			irr::u32 count = nodes.size();
			for (irr::u32 i = 0; i < count; ++i)
			{
				PathSceneNode* node = dynamic_cast<PathSceneNode*>(nodes[i]);

				wxString name(node->getName());
				if (name.CompareTo(wxString(m_SceneNode->getName()), wxString::ignoreCase) != 0)
					nodeNames.push_back(name);

				bool duplicatePathName = false;
				wxString path(node->getPathName().c_str());
				for (int j = 0; j < pathNames.Count(); ++j)
				{
					if (pathNames[j] == path)
					{
						duplicatePathName = true;
						break;
					}
				}

				if (!duplicatePathName)
					pathNames.push_back(path);
			}

			// populate the choice box
			PathSceneNode* prevNode = pathNode->getPrev();
			PathSceneNode* nextNode = pathNode->getNext();

			wxEditEnumProperty* pathChoices = new wxEditEnumProperty(
				_("Path Name"), wxPG_LABEL, pathNames, wxArrayInt(),
				wxString(pathNode->getPathName().c_str()));
			wxEnumProperty* prevChoices = new wxEnumProperty(
				_("Previous Node"), wxPG_LABEL, nodeNames);
			wxEnumProperty* nextChoices = new wxEnumProperty(
				_("Next Node"), wxPG_LABEL, nodeNames);

			if (prevNode)
				prevChoices->SetValueFromString(wxString(prevNode->getName()));

			if (nextNode)
				nextChoices->SetValueFromString(wxString(nextNode->getName()));

			m_Properties->AppendIn(m_GeneralProperties, pathChoices);
			m_Properties->AppendIn(m_GeneralProperties, prevChoices);
			m_Properties->AppendIn(m_GeneralProperties, nextChoices);
		} break;
		}

		attribs->drop();

		// materials
		irr::u32 numMaterials = m_SceneNode->getMaterialCount();
		if (numMaterials > 0)
		{
			if (numMaterials > 1)
				wxLogWarning(_("More than 1 material is defined, but we only support 1 material currently"));

			const irr::video::SMaterial& mat = m_SceneNode->getMaterial(0);
			
			// make sure we get the relative path for the textures
			irr::io::IAttributes* matAttribs = m_SceneNode->getSceneManager()->getVideoDriver()
				->createAttributesFromMaterial(mat, &opts);

			// textures
			for (irr::u32 j = 0; j < irr::video::MATERIAL_MAX_TEXTURES; ++j)
			{
				wxString texName;
				texName.assign(matAttribs->getAttributeAsString(wxString::Format(_("Texture%d"), j + 1).c_str()).c_str());
				if (texName.compare(wxT("../0")) == 0 || texName.compare(wxT("..\\0")) == 0 || texName.compare(wxT("0")) == 0)
					texName.clear();

				//m_Properties->Append(new TextureProperty(j, wxString::Format(_("Texture%d"), j + 1),
				//	wxPG_LABEL, texName));
				m_Properties->AppendIn(m_GeneralProperties, new wxStringProperty(wxString::Format(_("Texture%d"), j + 1),
					wxPG_LABEL, texName));
			}

			wxPGProperty* ambient = m_Properties->AppendIn(m_GeneralProperties,
				new wxStringProperty(_("Ambient"), wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(ambient, new wxUIntProperty(_("Alpha"), wxPG_LABEL, 
				mat.AmbientColor.getAlpha()));
			m_Properties->AppendIn(ambient, new wxUIntProperty(_("Red"), wxPG_LABEL,
				mat.AmbientColor.getRed()));
			m_Properties->AppendIn(ambient, new wxUIntProperty(_("Green"), wxPG_LABEL,
				mat.AmbientColor.getGreen()));
			m_Properties->AppendIn(ambient, new wxUIntProperty(_("Blue"), wxPG_LABEL,
				mat.AmbientColor.getBlue()));
			m_Properties->Collapse(ambient);

			wxPGProperty* diffuse = m_Properties->AppendIn(m_GeneralProperties,
				new wxStringProperty(_("Diffuse"), wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(diffuse, new wxUIntProperty(_("Alpha"), wxPG_LABEL,
				mat.DiffuseColor.getAlpha()));
			m_Properties->AppendIn(diffuse, new wxUIntProperty(_("Red"), wxPG_LABEL,
				mat.DiffuseColor.getRed()));
			m_Properties->AppendIn(diffuse, new wxUIntProperty(_("Green"), wxPG_LABEL,
				mat.DiffuseColor.getGreen()));
			m_Properties->AppendIn(diffuse, new wxUIntProperty(_("Blue"), wxPG_LABEL,
				mat.DiffuseColor.getBlue()));
			m_Properties->Collapse(diffuse);

			wxPGProperty* emissive = m_Properties->AppendIn(m_GeneralProperties,
				new wxStringProperty(_("Emissive"), wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(emissive, new wxUIntProperty(_("Alpha"), wxPG_LABEL,
				mat.EmissiveColor.getAlpha()));
			m_Properties->AppendIn(emissive, new wxUIntProperty(_("Red"), wxPG_LABEL,
				mat.EmissiveColor.getRed()));
			m_Properties->AppendIn(emissive, new wxUIntProperty(_("Green"), wxPG_LABEL,
				mat.EmissiveColor.getGreen()));
			m_Properties->AppendIn(emissive, new wxUIntProperty(_("Blue"), wxPG_LABEL,
				mat.EmissiveColor.getBlue()));
			m_Properties->Collapse(emissive);

			wxPGProperty* specular = m_Properties->AppendIn(m_GeneralProperties,
				new wxStringProperty(_("Specular"), wxPG_LABEL, "<composed>"));
			m_Properties->AppendIn(specular, new wxUIntProperty(_("Alpha"), wxPG_LABEL,
				mat.SpecularColor.getAlpha()));
			m_Properties->AppendIn(specular, new wxUIntProperty(_("Red"), wxPG_LABEL,
				mat.SpecularColor.getRed()));
			m_Properties->AppendIn(specular, new wxUIntProperty(_("Green"), wxPG_LABEL,
				mat.SpecularColor.getGreen()));
			m_Properties->AppendIn(specular, new wxUIntProperty(_("Blue"), wxPG_LABEL,
				mat.SpecularColor.getBlue()));
			m_Properties->Collapse(specular);

			m_Properties->AppendIn(m_GeneralProperties,
				new wxFloatProperty(_("Shininess"), wxPG_LABEL, mat.Shininess));
		}

		// read the custom attributes
		attribs = m_Map->GetAttributes(m_SceneNode->getName());
		if (attribs)
		{
			for (irr::u32 i = 0; i < attribs->getAttributeCount(); ++i)
			{
				wxString name = attribs->getAttributeName(i);
				PropertyClientData* clientData = new PropertyClientData(attribs->getAttributeType(i));
				switch (clientData->m_Type)
				{
				case irr::io::EAT_STRING:
				{
					wxStringProperty* property = new wxStringProperty(name, wxPG_LABEL,
						attribs->getAttributeAsString(i).c_str());
					property->SetClientData(clientData);
					m_Properties->AppendIn(m_CustomProperties, property);
				} break;
				case irr::io::EAT_VECTOR3D:
				{
					irr::core::vector3df vec = attribs->getAttributeAsVector3d(i);
					wxPGProperty* property = m_Properties->AppendIn(m_CustomProperties, 
						new wxStringProperty(name, wxPG_LABEL, "<composed>"));
					wxFloatProperty* x = new wxFloatProperty(_("x"), wxPG_LABEL, vec.X);
					x->SetClientData(clientData);
					wxFloatProperty* y = new wxFloatProperty(_("y"), wxPG_LABEL, vec.Y);
					y->SetClientData(clientData);
					wxFloatProperty* z = new wxFloatProperty(_("z"), wxPG_LABEL, vec.Z);
					z->SetClientData(clientData);
					m_Properties->AppendIn(property, x);
					m_Properties->AppendIn(property, y);
					m_Properties->AppendIn(property, z);
					m_Properties->Collapse(property);
					property->SetClientData(clientData);
				} break;
				case irr::io::EAT_VECTOR2D:
				{
					irr::core::vector2df vec = attribs->getAttributeAsVector2d(i);
					wxPGProperty* property = m_Properties->AppendIn(m_CustomProperties, 
						new wxStringProperty(name, wxPG_LABEL, "<composed>"));
					wxFloatProperty* x = new wxFloatProperty(_("x"), wxPG_LABEL, vec.X);
					x->SetClientData(clientData);
					wxFloatProperty* y = new wxFloatProperty(_("y"), wxPG_LABEL, vec.Y);
					y->SetClientData(clientData);
					m_Properties->AppendIn(property, x);
					m_Properties->AppendIn(property, y);
					m_Properties->Collapse(property);
					property->SetClientData(clientData);
				} break;
				case irr::io::EAT_COLOR:
				{
					irr::video::SColor color = attribs->getAttributeAsColor(i);
					wxPGProperty* property = m_Properties->AppendIn(m_CustomProperties, 
						new wxStringProperty(name, wxPG_LABEL, "<composed>"));
					wxUIntProperty* alpha = new wxUIntProperty(_("Alpha"), wxPG_LABEL, color.getAlpha());
					alpha->SetClientData(clientData);
					wxUIntProperty* red = new wxUIntProperty(_("Red"), wxPG_LABEL, color.getRed());
					red->SetClientData(clientData);
					wxUIntProperty* green = new wxUIntProperty(_("Green"), wxPG_LABEL, color.getGreen());
					green->SetClientData(clientData);
					wxUIntProperty* blue = new wxUIntProperty(_("Blue"), wxPG_LABEL, color.getBlue());
					blue->SetClientData(clientData);
					m_Properties->AppendIn(property, alpha);
					m_Properties->AppendIn(property, red);
					m_Properties->AppendIn(property, green);
					m_Properties->AppendIn(property, blue);
					m_Properties->Collapse(property);
					property->SetClientData(clientData);
				} break;
				case irr::io::EAT_FLOAT:
				{
					wxFloatProperty* property = new wxFloatProperty(name, wxPG_LABEL,
						attribs->getAttributeAsFloat(i));
					property->SetClientData(clientData);
					m_Properties->AppendIn(m_CustomProperties, property);
				} break;
				case irr::io::EAT_BOOL:
				{
					wxBoolProperty* property = new wxBoolProperty(name, wxPG_LABEL,
						attribs->getAttributeAsBool(i));
					property->SetClientData(clientData);
					m_Properties->AppendIn(m_CustomProperties, property);
				} break;
				case irr::io::EAT_INT:
				{
					wxIntProperty* property = new wxIntProperty(name, wxPG_LABEL,
						attribs->getAttributeAsInt(i));
					property->SetClientData(clientData);
					m_Properties->AppendIn(m_CustomProperties, property);
				} break;
				}
			}
		}
	}
	else // refresh the children
	{
		irr::core::vector3df pos = m_SceneNode->getAbsolutePosition();
		m_PosX->SetValue(pos.X);
		m_PosY->SetValue(pos.Y);
		m_PosZ->SetValue(pos.Z);
	}
}

void PropertyPanel::SetSceneNode(irr::scene::ISceneNode* node)
{
	m_SceneNode = node;
	Refresh();
}

void PropertyPanel::OnToolAdd(wxCommandEvent& event)
{
	wxLogMessage(_("Not implemented"));
}

void PropertyPanel::OnToolRemove(wxCommandEvent& event)
{
	wxLogMessage(_("Not implemented"));
}

void PropertyPanel::OnValueChanging(wxPropertyGridEvent& event)
{
	wxString propName(event.GetPropertyName());
	if (propName.compare(_("Name")) == 0)
	{
		event.Veto(); // for now, don't allow the name change
	}
}

irr::core::vector3df valueToVec3(const wxString& value)
{
	std::istringstream iss(value.utf8_string());
	std::string token;

	// X
	double _x = 0;
	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToDouble(&_x);

	// Y
	double _y = 0;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToDouble(&_y);

	// Z
	double _z = 0;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToDouble(&_z);

	return irr::core::vector3df(_x, _y, _z);
}

irr::core::vector2df valueToVec2(const wxString& value)
{
	std::istringstream iss(value.utf8_string());
	std::string token;

	// X
	double _x = 0;
	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToDouble(&_x);

	// Y
	double _y = 0;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToDouble(&_y);

	return irr::core::vector2df(_x, _y);
}

irr::core::dimension2df valueToDim2df(const wxString& value)
{
	irr::core::dimension2df result;

	std::istringstream iss(value.utf8_string());
	std::string token;

	double _value = 0;
	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToDouble(&_value);
	result.Width = _value;

	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToDouble(&_value);
	result.Height = _value;

	return result;
}

irr::core::dimension2du valueToDim2du(const wxString& value)
{
	irr::core::dimension2du result;

	std::istringstream iss(value.utf8_string());
	std::string token;

	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToUInt(&result.Width);

	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToUInt(&result.Height);

	return result;
}

irr::video::SColor valueToColor(const wxString& value)
{
	std::istringstream iss(value.utf8_string());
	std::string token;

	irr::u32 alpha = 255;
	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToUInt(&alpha);

	irr::u32 red = 255;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToUInt(&red);

	irr::u32 green = 255;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToUInt(&green);

	irr::u32 blue = 255;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToUInt(&blue);

	return irr::video::SColor(alpha, red, green, blue);
}

void PropertyPanel::OnValueChanged(wxPropertyGridEvent& event)
{
	wxString propName(event.GetPropertyName());

	if (propName.compare(_("Name")) == 0)
	{
	}
	else if (propName.compare(_("Position")) == 0)
	{
		irr::core::vector3df position = valueToVec3(event.GetValue());
		m_Commands.Submit(new TranslateNodeCommand(m_SceneNode, 
			m_SceneNode->getAbsolutePosition(), position));
	}
	else if (propName.compare(_("Rotation")) == 0)
	{
		irr::core::vector3df rotation = valueToVec3(event.GetValue());
		m_Commands.Submit(new RotateNodeCommand(m_SceneNode,
			m_SceneNode->getRotation(), rotation));
	}
	else if (propName.compare(_("Scale")) == 0)
	{
		irr::core::vector3df scale = valueToVec3(event.GetValue());
		m_Commands.Submit(new ScaleNodeCommand(m_SceneNode,
			m_SceneNode->getScale(), scale));
	}
	else if (propName.compare(_("Size")) == 0)
	{
		irr::core::vector3df size;

		if (m_SceneNode->getType() == irr::scene::ESNT_CUBE)
		{
			// only a single size value
			wxDouble value = event.GetValue();
			size.set(value, value, value);
		}
		else
			size = valueToVec3(event.GetValue());

		m_Commands.Submit(new ResizeNodeCommand(m_SceneNode,
			size));
	}
	else if (propName.compare(_("Tile Count")) == 0)
	{
		irr::core::dimension2du count = valueToDim2du(event.GetValue());
		wxStringProperty* prop = dynamic_cast<wxStringProperty*>(
			m_Properties->GetProperty(_("Tile Size")));
		irr::core::dimension2df size = valueToDim2df(prop->GetValue());
		m_Commands.Submit(new ResizeNodeCommand(m_SceneNode,
			size, count));
	}
	else if (propName.compare(_("Tile Size")) == 0)
	{
		irr::core::dimension2df size = valueToDim2df(event.GetValue());
		wxStringProperty* prop = dynamic_cast<wxStringProperty*>(
			m_Properties->GetProperty(_("Tile Count")));
		irr::core::dimension2du count = valueToDim2du(prop->GetValue());
		m_Commands.Submit(new ResizeNodeCommand(m_SceneNode,
			size, count));
	}
	else if (propName.compare(_("Radius")) == 0)
	{
		irr::core::vector3df size;
		size.X = (wxDouble)event.GetValue();
		m_Commands.Submit(new ResizeNodeCommand(m_SceneNode, size));

	}
	else if (propName.compare(_("Ambient")) == 0)
	{
		irr::video::SColor color = valueToColor(event.GetValue());
		m_Commands.Submit(new ChangeColorCommand(ChangeColorCommand::CT_AMBIENT,
			m_SceneNode, 0, color));
	}
	else if (propName.compare(_("Diffuse")) == 0)
	{
		irr::video::SColor color = valueToColor(event.GetValue());
		m_Commands.Submit(new ChangeColorCommand(ChangeColorCommand::CT_DIFFUSE,
			m_SceneNode, 0, color));
	}
	else if (propName.compare(_("Emissive")) == 0)
	{
		irr::video::SColor color = valueToColor(event.GetValue());
		m_Commands.Submit(new ChangeColorCommand(ChangeColorCommand::CT_EMISSIVE,
			m_SceneNode, 0, color));
	}
	else if (propName.compare(_("Specular")) == 0)
	{
		irr::video::SColor color = valueToColor(event.GetValue());
		m_Commands.Submit(new ChangeColorCommand(ChangeColorCommand::CT_SPECULAR,
			m_SceneNode, 0, color));
	}
	else if (propName.compare(_("Shininess")) == 0)
	{
		double value = event.GetValue();
		m_Commands.Submit(new ChangeColorCommand(ChangeColorCommand::CT_SHINY,
			m_SceneNode, 0, value));
	}
	else if (propName.StartsWith(_("Texture")))
	{
		wxString texture = event.GetValue();
		wxString sTexId = propName.substr(_("Texture").length());

		wxUint32 texId = 0;
		sTexId.ToUInt(&texId);

		m_Commands.Submit(new ChangeTextureCommand(m_SceneNode,
			0, texId, texture));
	}
	else if (propName.StartsWith(_("Path Name")))
	{
		wxString pathName = event.GetValue();

		m_Commands.Submit(new UpdatePathNameCommand(m_SceneNode->getSceneManager(),
			m_SceneNode->getName(), pathName));
	}
	else if (propName.StartsWith(_("Previous Node")))
	{
		wxEnumProperty* prop = (wxEnumProperty*)event.GetProperty();
		wxString nodeName = prop->GetChoices().GetLabel((long)event.GetPropertyValue());

		ViewPanel* viewPanel = ((MapEditor*)GetParent())->GetViewPanel();

		PathSceneNode* pathNode = dynamic_cast<PathSceneNode*>(m_SceneNode);
		if (nodeName.CompareTo(wxT("--none--")) == 0)
		{
			pathNode->setPrev(nullptr);
		}
		else
		{
			PathSceneNode* other = dynamic_cast<PathSceneNode*>(
				m_SceneNode->getSceneManager()
				->getSceneNodeFromName(nodeName.c_str().AsChar(), nullptr));
			if (other)
			{
				pathNode->setPrev(other);
				pathNode->setPathName(other->getPathName());
				pathNode->drawLink(true);
			}
		}
	}
	else if (propName.StartsWith(_("Next Node")))
	{
		wxEnumProperty* prop = (wxEnumProperty*)event.GetProperty();
		wxString nodeName = prop->GetChoices().GetLabel((long)event.GetPropertyValue());

		ViewPanel* viewPanel = ((MapEditor*)GetParent())->GetViewPanel();

		PathSceneNode* pathNode = dynamic_cast<PathSceneNode*>(m_SceneNode);
		if (nodeName.CompareTo(wxT("--none--")) == 0)
		{
			pathNode->setNext(nullptr);
		}
		else
		{
			PathSceneNode* other = dynamic_cast<PathSceneNode*>(
				m_SceneNode->getSceneManager()
				->getSceneNodeFromName(nodeName.c_str().AsChar(), nullptr));
			if (other)
			{
				pathNode->setNext(other);
				pathNode->setPathName(other->getPathName());
				pathNode->drawLink(true);
			}
		}
	}
	else
	{
		// custom property
		irr::io::IAttributes* attribs = m_Map->GetAttributes(m_SceneNode->getName());
		wxPGProperty* property = event.GetProperty();
		PropertyClientData* clientData = static_cast<PropertyClientData*>(property->GetClientData());
		switch (clientData->m_Type)
		{
		case irr::io::EAT_STRING:
			attribs->setAttribute(property->GetName(), event.GetValue().GetString().c_str().AsChar());
			break;
		case irr::io::EAT_VECTOR3D:
			attribs->setAttribute(property->GetName(), valueToVec3(event.GetValue()));
			break;
		case irr::io::EAT_VECTOR2D:
			attribs->setAttribute(property->GetName(), valueToVec2(event.GetValue()));
			break;
		case irr::io::EAT_COLOR:
			attribs->setAttribute(property->GetName(), valueToColor(event.GetValue()));
			break;
		case irr::io::EAT_FLOAT:
			attribs->setAttribute(property->GetName(), (float)event.GetValue().GetDouble());
			break;
		case irr::io::EAT_BOOL:
			attribs->setAttribute(property->GetName(), event.GetValue().GetBool());
			break;
		case irr::io::EAT_INT:
			attribs->setAttribute(property->GetName(), (int)event.GetValue().GetLong());
			break;
		}
	}
}
