/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Commands.hpp"
#include "Component.hpp"
#include "Convert.hpp"
#include "../extend/CylinderSceneNode.hpp"
#include "../extend/PathSceneNode.hpp"
#include "../extend/PlaneSceneNode.hpp"
#include "UpdatableTerrainSceneNode.hpp"

#include <wx/log.h>
#include <wx/sstream.h>

AddNodeCommand::AddNodeCommand(TOOLID toolId,
	ExplorerPanel* explorerPanel,
	irr::scene::ISceneManager* sceneMgr,
	irr::scene::ISceneNode* mapRoot,
	std::shared_ptr<Map>& map, const irr::core::vector3df& position, 
	const wxString& name)
	: m_ToolId(toolId), m_ExplorerPanel(explorerPanel),
	  m_SceneMgr(sceneMgr), m_MapRoot(mapRoot), 
	  m_Map(map), m_Position(position), m_Name(name)
{
	// handle TOOL_ACTOR
	if (m_ToolId == TOOL_ACTOR)
	{
		m_Actor = name;
		m_Name = m_Map->NextName(name);
	}
	else if (m_ToolId == TOOL_MESH)
	{
		m_Mesh = name; // store the XML definition
		wxStringInputStream stream(m_Mesh);
		wxXmlDocument doc(stream);
		wxXmlNode* root = doc.GetRoot();
		if (root)
		{
			wxString meshName = root->GetAttribute("name");
			m_Name = m_Map->NextName(meshName);
		}
	}
}

AddNodeCommand::AddNodeCommand(const wxString& nodeType,
	ExplorerPanel* explorerPanel,
	irr::scene::ISceneManager* sceneMgr,
	irr::scene::ISceneNode* mapRoot,
	std::shared_ptr<Map>& map,
	const wxString& name)
	: m_ExplorerPanel(explorerPanel),
	m_SceneMgr(sceneMgr), m_MapRoot(mapRoot),
	m_Map(map), m_Name(name)
{
	// convert from nodeType string to TOOLID
	if (nodeType.CmpNoCase("cube") == 0)
		m_ToolId = TOOL_CUBE;
	else if (nodeType.CmpNoCase("cylinder") == 0)
		m_ToolId = TOOL_CYLINDER;
	else if (nodeType.CmpNoCase("sphere") == 0)
		m_ToolId = TOOL_SPHERE;
	else if (nodeType.CmpNoCase("plane") == 0)
		m_ToolId = TOOL_PLANE;
	else if (nodeType.CmpNoCase("terrain") == 0)
		m_ToolId = TOOL_TERRAIN;
	else if (nodeType.CmpNoCase("skydome") == 0)
		m_ToolId = TOOL_SKYBOX;
	else if (nodeType.CmpNoCase("playerstart") == 0)
		m_ToolId = TOOL_PLAYERSTART;
	else if (nodeType.CmpNoCase("light") == 0)
		m_ToolId = TOOL_LIGHT;
	else if (nodeType.CmpNoCase("pathnode") == 0)
		m_ToolId = TOOL_PATHNODE;
	else if (nodeType.CmpNoCase("actor") == 0)
	{
		m_ToolId = TOOL_ACTOR;
		m_Actor = name;
		m_Name = m_Map->NextName(name);
	}
	else if (nodeType.CmpNoCase("animatedMesh") == 0)
		m_ToolId = (TOOLID)irr::scene::ESNT_ANIMATED_MESH;
	else if (nodeType.CmpNoCase("mesh") == 0)
		m_ToolId = (TOOLID)irr::scene::ESNT_MESH;
	else
		m_ToolId = (TOOLID)0; // unknown
}

AddNodeCommand::~AddNodeCommand(void)
{
}

bool AddNodeCommand::CanUndo(void) const
{
	return true;
}

bool AddNodeCommand::Do(void)
{
	irr::scene::IMeshSceneNode* node = nullptr;
	//irr::scene::ITerrainSceneNode* terrain = nullptr;
	irr::scene::ISceneNode* skybox = nullptr;

	irr::io::IAttributes* attribs = m_SceneMgr->getFileSystem()->createEmptyAttributes(nullptr);

	bool isGeometry = false;
	bool isActor = false;

	switch (m_ToolId)
	{
	case TOOL_CUBE:
		node = m_SceneMgr->addCubeSceneNode(10, m_MapRoot, NID_PICKABLE);
		isGeometry = true;
		break;
	case TOOL_CYLINDER:
	{
		node = static_cast<irr::scene::IMeshSceneNode*>(m_SceneMgr->addSceneNode("cylinder", m_MapRoot));
		node->setID(NID_PICKABLE);
		isGeometry = true;
	} break;
	case TOOL_SPHERE:
		node = m_SceneMgr->addSphereSceneNode(5, 16, m_MapRoot, NID_PICKABLE);
		isGeometry = true;
		break;
	case TOOL_PLANE:
	{
		node = static_cast<irr::scene::IMeshSceneNode*>(m_SceneMgr->addSceneNode("plane", m_MapRoot));
		node->setID(NID_PICKABLE);
		isGeometry = true;
	} break;
	case TOOL_TERRAIN:
	{
		// Create an updatable terrain scene node
		UpdatableTerrainSceneNode* terrain = new UpdatableTerrainSceneNode(
			m_MapRoot, m_SceneMgr, m_SceneMgr->getFileSystem(), NID_PICKABLE, 
			5, irr::scene::ETPS_17, m_Position);

		// Create a default 129x129 heightmap (fits in 16-bit indices)
		// terrain->createHeightmap(129, 0.0f);

		// create a 257x257 heightmap (fits in 32-bit indices)
		terrain->createHeightmap(257, 0.0f);
		terrain->setName(m_Name.c_str());

		terrain->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		terrain->setMaterialTexture(0, m_SceneMgr->getVideoDriver()->getTexture(
			"editor.mpk:textures/terrain.jpg"));

		// Create triangle selector for collision
		irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelectorFromBoundingBox(
			terrain);
		if (selector)
		{
			// this selector needs to be recreated when the terrain is updated
			terrain->setTriangleSelector(selector);
			selector->drop();
		}

		// terrain->drop(); // Scene manager will hold reference
		isGeometry = true;
	} break;
	case TOOL_SKYBOX:
	{
		skybox = m_SceneMgr->addSkyDomeSceneNode(m_SceneMgr->getVideoDriver()->getTexture(
			"editor.mpk:textures/skybox.png"), 32, 16, 0.9f, 2, 1000, m_MapRoot, NID_PICKABLE);
		skybox->setName(m_Name.c_str());
		skybox->setMaterialFlag(irr::video::EMF_LIGHTING, false);

		isGeometry = true;
	} break;
	case TOOL_PLAYERSTART:
	{
		irr::scene::ISceneNode* start = m_SceneMgr->addSceneNode("playerstart", m_MapRoot);
		start->setName(m_Name.c_str());
		start->setID(NID_PICKABLE);
		start->setPosition(m_Position);

		wxString name(m_Name);
		name.append(wxT("_marker"));
		irr::scene::IBillboardSceneNode* marker = m_SceneMgr->addBillboardSceneNode(
			start, irr::core::dimension2df(5, 5), irr::core::vector3df(), NID_NOSAVE);
		marker->setName(name.c_str());
		marker->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		marker->setMaterialFlag(irr::video::EMF_ZWRITE_ENABLE, false);
		marker->setMaterialType(irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);
		marker->setMaterialTexture(0, m_SceneMgr->getVideoDriver()->getTexture(
			"editor.mpk:icons/player-start.png"));

		// pick the player start based on the marker
		irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelectorFromBoundingBox(
			marker);
		start->setTriangleSelector(selector);
		selector->drop();
		isActor = true;
	} break;
	case TOOL_LIGHT:
	{
		irr::scene::ILightSceneNode* light = m_SceneMgr->addLightSceneNode(
			m_MapRoot, m_Position, irr::video::SColorf(1, 1, 1, 1), 100, NID_PICKABLE);
		light->setName(m_Name.c_str());
		light->enableCastShadow(true);

		wxString name(m_Name);
		name.append(wxT("_marker"));
		irr::scene::IBillboardSceneNode* marker = m_SceneMgr->addBillboardSceneNode(
			light, irr::core::dimension2df(5, 5), irr::core::vector3df(), NID_NOSAVE);
		marker->setName(name.c_str());
		marker->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		marker->setMaterialFlag(irr::video::EMF_ZWRITE_ENABLE, false);
		marker->setMaterialType(irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);
		marker->setMaterialTexture(0, m_SceneMgr->getVideoDriver()->getTexture(
			"editor.mpk:icons/light-bulb.png"));

		// pick the light based on the marker
		irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelectorFromBoundingBox(
			marker);
		light->setTriangleSelector(selector);
		selector->drop();

		isActor = true;
	} break;
	case TOOL_PATHNODE:
	{
		irr::scene::ISceneNode* pathNode = m_SceneMgr->addSceneNode("pathnode", m_MapRoot);
		pathNode->setName(m_Name.c_str());
		pathNode->setID(NID_PICKABLE);
		pathNode->setPosition(m_Position);

		dynamic_cast<PathSceneNode*>(pathNode)->drawLink(true); // always show the links

		wxString name(m_Name);
		name.append(wxT("_marker"));
		irr::scene::IBillboardSceneNode* marker = m_SceneMgr->addBillboardSceneNode(
			pathNode, irr::core::dimension2df(5, 5), irr::core::vector3df(), NID_NOSAVE);
		marker->setName(name.c_str());
		marker->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		marker->setMaterialFlag(irr::video::EMF_ZWRITE_ENABLE, false);
		marker->setMaterialType(irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);
		marker->setMaterialTexture(0, m_SceneMgr->getVideoDriver()->getTexture(
			"editor.mpk:icons/path-node.png"));

		// pick the path node based on the marker
		irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelectorFromBoundingBox(
			marker);
		pathNode->setTriangleSelector(selector);
		selector->drop();

		isActor = true;
	} break;
	case TOOL_ACTOR:
	{
		// get the actor definition
		wxString definition = m_ExplorerPanel->GetBrowser()->GetActorDefinition(m_Actor);
		wxStringInputStream stream(definition);
		wxXmlDocument doc(stream);
		wxXmlNode* actorDefinition = doc.GetRoot();
		if (!actorDefinition)
			return false;

		// get the mesh and texture
		wxString mesh;
		wxString texture;
		std::map<irr::core::stringc, irr::io::IAttributes*> components;

		// figure out the type of actor
		wxString type = actorDefinition->GetAttribute("type");
		if (type.CmpNoCase("Model") == 0)
		{
			wxXmlNode* child = actorDefinition->GetChildren();
			if (!child)
				return false;

			while (child)
			{
				if (child->GetName().CmpNoCase("properties") == 0)
				{
					wxXmlNode* property = child->GetChildren();
					while (property)
					{
						if (property->GetName().CmpNoCase("string") == 0)
						{
							if (property->HasAttribute("Mesh"))
								mesh = property->GetAttribute("Mesh");
							else if (property->HasAttribute("Texture"))
								texture = property->GetAttribute("Texture");
							else // add the attribute as user data
							{
								wxXmlAttribute* attribute = property->GetAttributes();
								if (attribute) // we only have one attribute
								{
									wxString key = attribute->GetName();
									wxString value = attribute->GetValue();
									attribs->addString(key.c_str().AsChar(), value.c_str().AsChar());
								}
							}
						}
						else if (property->GetName().CmpNoCase("float") == 0)
						{
							wxXmlAttribute* attribute = property->GetAttributes();
							if (attribute) // we only have one attribute
							{
								wxString key = attribute->GetName();
								wxString value = attribute->GetValue();
								attribs->addFloat(key.c_str().AsChar(), valueToFloat(value));
							}
						}
						else if (property->GetName().CmpNoCase("int") == 0)
						{
							wxXmlAttribute* attribute = property->GetAttributes();
							if (attribute) // we only have one attribute
							{
								wxString key = attribute->GetName();
								wxString value = attribute->GetValue();
								attribs->addInt(key.c_str().AsChar(), valueToInt(value));
							}
						}
						else if (property->GetName().CmpNoCase("vec2") == 0)
						{
							wxXmlAttribute* attribute = property->GetAttributes();
							if (attribute) // we only have one attribute
							{
								wxString key = attribute->GetName();
								wxString value = attribute->GetValue();
								attribs->addVector2d(key.c_str().AsChar(), valueToVec2(value));
							}
						}
						else if (property->GetName().CmpNoCase("vec3") == 0)
						{
							wxXmlAttribute* attribute = property->GetAttributes();
							if (attribute) // we only have one attribute
							{
								wxString key = attribute->GetName();
								wxString value = attribute->GetValue();
								attribs->addVector3d(key.c_str().AsChar(), valueToVec3(value));
							}
						}
						
						property = property->GetNext();
					}
				}
				else if (child->GetName().CmpNoCase("components") == 0)
				{
					wxXmlNode* component = child->GetChildren();
					while (component)
					{
						if (component->GetName().CmpNoCase("component") == 0)
						{
							wxXmlAttribute* attribute = component->GetAttributes();
							wxString componentName = attribute->GetValue();
							
							// create attributes
							irr::io::IAttributes* attributes = m_SceneMgr->getFileSystem()->createEmptyAttributes(nullptr);
							wxXmlNode* property = component->GetChildren();
							while (property)
							{
								wxXmlAttribute* attribute = property->GetAttributes();
								wxString key = attribute->GetName();
								wxString value = attribute->GetValue();
								if (property->GetName().CmpNoCase("int") == 0)
								{
									attributes->addInt(key.c_str(), valueToInt(value));
								}
								else if (property->GetName().CmpNoCase("float") == 0)
								{
									attributes->addFloat(key.c_str(), valueToFloat(value));
								}
								else if (property->GetName().CmpNoCase("string") == 0)
								{
									attributes->addString(key.c_str().AsChar(), value.c_str().AsChar());
								}
								else if (property->GetName().CmpNoCase("vec2") == 0)
								{
									attributes->addVector2d(key.c_str(), valueToVec2(value));
								}
								else if (property->GetName().CmpNoCase("vec3") == 0)
								{
									attributes->addVector3d(key.c_str(), valueToVec3(value));
								}

								property = property->GetNext();
							}

							// add to the component map
							components[componentName.c_str().AsChar()] = attributes;
						}

						component = component->GetNext();
					}
				}
			
				child = child->GetNext();
			}

			// add a model actor
			irr::scene::IAnimatedMesh* animatedMesh = m_SceneMgr->getMesh(mesh.c_str().AsChar());
			if (!animatedMesh)
				return false;

			irr::scene::IAnimatedMeshSceneNode* model = m_SceneMgr->addAnimatedMeshSceneNode(
				animatedMesh, m_MapRoot, NID_PICKABLE);
			model->setName(m_Name.c_str());
			model->setPosition(m_Position);
			model->setAnimationSpeed(0); // we don't want to have it animated
			model->setMaterialFlag(irr::video::EMF_LIGHTING, false);

			// set the texture
			if (!texture.IsEmpty())
				model->setMaterialTexture(0, m_SceneMgr->getVideoDriver()->getTexture(
			 		texture.c_str().AsChar()));
			else
				model->setMaterialTexture(0, m_SceneMgr->getVideoDriver()->getTexture(
			 		"editor.mpk:textures/default.jpg"));

			if (!model->getTriangleSelector())
			{
				irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelector(
					model->getMesh(), model);
				if (selector)
				{
					model->setTriangleSelector(selector);
					selector->drop();
				}
			}

			// add the components
			for (std::map<irr::core::stringc, irr::io::IAttributes*>::iterator it = components.begin();
				it != components.end(); ++it)
			{
				// irr::scene::ISceneNodeAnimator* anim = new Component(
				// 	ComponentFactory::HashComponentName(it->first.c_str()), it->second);
				irr::scene::ISceneNodeAnimator* anim = m_SceneMgr->createSceneNodeAnimator(it->first.c_str());
				if (anim)
				{
					anim->deserializeAttributes(it->second);
					model->addAnimator(anim);
				}

				it->second->drop();
			}
		}

		isActor = true;
	} break;
	case irr::scene::ESNT_ANIMATED_MESH:
	{
		// Add an empty animated mesh scene node
		irr::scene::IAnimatedMeshSceneNode* sceneNode = m_SceneMgr->addAnimatedMeshSceneNode(
			nullptr, m_MapRoot, NID_PICKABLE, m_Position, 
			irr::core::vector3df(0, 0, 0), irr::core::vector3df(1, 1, 1),
			true);

		sceneNode->setName(m_Name.c_str());
		isActor = true;
	} break;
	case TOOL_MESH:
	{
		wxString model;
		wxString textures[4];

		wxStringInputStream stream(m_Mesh);
		wxXmlDocument doc(stream);
		wxXmlNode* root = doc.GetRoot();
		if (root)
		{
			wxXmlNode* entry = root->GetChildren();
			while (entry)
			{
				wxString entryName = entry->GetName();
				if (entryName.CmpNoCase(wxT("mesh")) == 0)
					model = entry->GetNodeContent();
				else if (entryName.CmpNoCase(wxT("texture0")) == 0)
					textures[0] = entry->GetNodeContent();
				else if (entryName.CmpNoCase(wxT("texture1")) == 0)
					textures[1] = entry->GetNodeContent();
				else if (entryName.CmpNoCase(wxT("texture2")) == 0)
					textures[2] = entry->GetNodeContent();
				else if (entryName.CmpNoCase(wxT("texture3")) == 0)
					textures[3] = entry->GetNodeContent();

				entry = entry->GetNext();
			}
		}

		if (!model.IsEmpty())
		{
			irr::scene::IMeshSceneNode* node = m_SceneMgr->addMeshSceneNode(
				m_SceneMgr->getMesh(model.c_str().AsChar()), m_MapRoot, NID_PICKABLE,
				m_Position, irr::core::vector3df(0, 0, 0), irr::core::vector3df(1, 1, 1),
				true);

			node->setName(m_Name.c_str());
			node->setMaterialFlag(irr::video::EMF_LIGHTING, false);

			if (!textures[0].IsEmpty())
				node->setMaterialTexture(0, m_SceneMgr->getVideoDriver()->getTexture(
					textures[0].c_str().AsChar()));
			if (!textures[1].IsEmpty())
				node->setMaterialTexture(1, m_SceneMgr->getVideoDriver()->getTexture(
					textures[1].c_str().AsChar()));
			if (!textures[2].IsEmpty())
				node->setMaterialTexture(2, m_SceneMgr->getVideoDriver()->getTexture(
					textures[2].c_str().AsChar()));
			if (!textures[3].IsEmpty())
				node->setMaterialTexture(3, m_SceneMgr->getVideoDriver()->getTexture(
					textures[3].c_str().AsChar()));

			if (!node->getTriangleSelector())
			{
				irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelector(
					node->getMesh(), node);
				if (selector)
				{
					node->setTriangleSelector(selector);
					selector->drop();
				}
			}
		}
		else
			return false;

		isGeometry = true;
	} break;
	case irr::scene::ESNT_MESH:
	{
		node = m_SceneMgr->addMeshSceneNode(nullptr, m_MapRoot, NID_PICKABLE,
			m_Position, irr::core::vector3df(0, 0, 0), irr::core::vector3df(1, 1, 1),
			true);

		isGeometry = true;
	} break;
	default:
		return false;
	}

	if (node)
	{
		node->setName(m_Name.c_str());
		node->setPosition(m_Position);
		node->setMaterialFlag(irr::video::EMF_LIGHTING, false);
		node->setMaterialTexture(0, m_SceneMgr->getVideoDriver()->getTexture(
			"editor.mpk:textures/default.jpg"));

		if (!node->getTriangleSelector())
		{
			irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelector(
				node->getMesh(), node);
			if (selector)
			{
				node->setTriangleSelector(selector);
				selector->drop();
			}
		}
	}

	m_Map->AddEntity(m_Name, attribs);

	if (isGeometry)
		m_ExplorerPanel->AddGeometry(m_Name);
	if (isActor)
		m_ExplorerPanel->AddActor(m_Name);

	return true;
}

wxString AddNodeCommand::GetName(void) const
{
	return wxString(_("Add ")).Append(m_Name);
}

bool AddNodeCommand::Undo(void)
{
	irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName(m_Name.c_str(), m_MapRoot);
	if (node)
		node->remove();

	switch (m_ToolId)
	{
	case TOOL_LIGHT:
	case TOOL_PLAYERSTART:
	case TOOL_PATHNODE:
	case TOOL_ACTOR:
		m_ExplorerPanel->RemoveActor(m_Name);
		break;
	default:
		m_ExplorerPanel->RemoveGeometry(m_Name);
	}

	m_Map->RemoveEntity(m_Name);
	
	return true;
}

TranslateNodeCommand::TranslateNodeCommand(irr::scene::ISceneManager* sceneMgr, 
	const selection_t& selection, const irr::core::vector3df& start)
	: m_SceneMgr(sceneMgr), m_Selection(selection), m_Delta(start)
{
}

TranslateNodeCommand::TranslateNodeCommand(irr::scene::ISceneNode* node,
	const irr::core::vector3df& start, const irr::core::vector3df& end)
	: m_SceneMgr(node->getSceneManager())
{
	m_Selection.push_back(node->getName());
	m_Delta = end - start;
}

TranslateNodeCommand::~TranslateNodeCommand(void)
{
}

void TranslateNodeCommand::Update(const irr::core::vector3df& delta)
{
	m_Delta += delta;

	// move the selection in real time
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df pos = node->getPosition();
			node->setPosition(pos + delta);
		}
	}
}

bool TranslateNodeCommand::CanUndo(void) const
{
	return true;
}

bool TranslateNodeCommand::Do(void)
{
	// move the selection
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df pos = node->getPosition();
			node->setPosition(pos + m_Delta);
		}
	}

	return true;
}

wxString TranslateNodeCommand::GetName(void) const
{
	return wxString(_("Translate selection"));
}

bool TranslateNodeCommand::Undo(void)
{
	// move the selection
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df pos = node->getPosition();
			node->setPosition(pos - m_Delta);
		}
	}

	return true;
}

//RotateNodeCommand::RotateNodeCommand(irr::scene::ISceneManager* sceneMgr,
//	irr::scene::ISceneNode* parent, const selection_t& selection/*,
//	const irr::core::vector3df& start*/)
//	: m_SceneMgr(sceneMgr), m_Parent(parent), m_Selection(selection)/*,
//	  m_Delta(start)*/ 
//{
//}

RotateNodeCommand::RotateNodeCommand(irr::scene::ISceneNode* node,
	const irr::core::vector3df& start, const irr::core::vector3df& end)
	: m_SceneMgr(node->getSceneManager())
{
	m_Selection.push_back(node->getName());
	m_Delta = end - start;
}

RotateNodeCommand::~RotateNodeCommand(void)
{
}

void RotateNodeCommand::Update(const irr::core::vector3df& delta)
{
	m_Delta += delta;

	// move the selection in real time
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df rotation = node->getRotation();
			node->setRotation(rotation + delta);
		}
	}
}

bool RotateNodeCommand::CanUndo(void) const
{
	return true;
}

bool RotateNodeCommand::Do(void)
{
	// rotate the selection
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df rotation = node->getRotation();
			node->setRotation(rotation + m_Delta);
		}
	}

	return true;
}

wxString RotateNodeCommand::GetName(void) const
{
	return wxString(_("Rotate selection"));
}

bool RotateNodeCommand::Undo(void)
{
	// rotate the selection
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df rotation = node->getRotation();
			node->setRotation(rotation - m_Delta);
		}
	}

	return true;
}

ScaleNodeCommand::ScaleNodeCommand(irr::scene::ISceneNode* node,
	const irr::core::vector3df& start, const irr::core::vector3df& end)
	: m_SceneMgr(node->getSceneManager())
{
	m_Selection.push_back(node->getName());
	m_Delta = end - start;
}

ScaleNodeCommand::~ScaleNodeCommand(void)
{
}

void ScaleNodeCommand::Update(const irr::core::vector3df& delta)
{
	m_Delta += delta;

	// update in real time
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df scale = node->getScale();
			node->setScale(scale + delta);
		}
	}
}

bool ScaleNodeCommand::CanUndo(void) const
{
	return true;
}

bool ScaleNodeCommand::Do(void)
{
	// scale the selection
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df scale = node->getScale();
			node->setScale(scale + m_Delta);
		}
	}

	return true;
}

wxString ScaleNodeCommand::GetName(void) const
{
	return _("Scale selection");
}

bool ScaleNodeCommand::Undo(void)
{
	// scale the selection
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::core::vector3df scale = node->getScale();
			node->setScale(scale - m_Delta);
		}
	}

	return true;
}

ResizeNodeCommand::ResizeNodeCommand(irr::scene::ISceneNode* node, 
	const irr::core::vector3df& newSize)
	: m_SceneMgr(node->getSceneManager()), m_Name(node->getName()),
	m_Size(newSize)
{
}

ResizeNodeCommand::ResizeNodeCommand(irr::scene::ISceneNode* node,
	const irr::core::dimension2df& newSize,
	const irr::core::dimension2du& newCount)
	: m_SceneMgr(node->getSceneManager()), m_Name(node->getName()),
	m_TileSize(newSize), m_TileCount(newCount)
{
}

ResizeNodeCommand::~ResizeNodeCommand(void)
{
}

bool ResizeNodeCommand::CanUndo(void) const
{
	return true;
}

bool ResizeNodeCommand::Do(void)
{
	irr::io::IAttributes* attribs = m_SceneMgr->getFileSystem()->createEmptyAttributes();
	irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName(m_Name);
	node->serializeAttributes(attribs);

	// store the material
	irr::video::SMaterial& material = node->getMaterial(0);
	irr::io::SAttributeReadWriteOptions opts;
	opts.Filename = nullptr;
	opts.Flags = 0/*irr::io::EARWF_USE_RELATIVE_PATHS*/;
	irr::io::IAttributes* matAttribs = m_SceneMgr->getVideoDriver()->createAttributesFromMaterial(
		material, &opts);

	irr::core::vector3df oldSize;
	irr::core::dimension2df oldTileSize;
	irr::core::dimension2du oldTileCount;
	switch (node->getType())
	{
	case irr::scene::ESNT_CUBE:
		oldSize = irr::core::vector3df(attribs->getAttributeAsFloat("Size"));
		attribs->setAttribute("Size", m_Size.X);
		break;
	case irr::scene::ESNT_SPHERE:
		oldSize.X = attribs->getAttributeAsFloat("Radius");
		oldSize.Y = attribs->getAttributeAsInt("PolyCountX");
		oldSize.Z = attribs->getAttributeAsInt("PolyCountY");
		attribs->setAttribute("Radius", m_Size.X);
		attribs->setAttribute("PolyCountX", m_Size.Y);
		attribs->setAttribute("PolyCountY", m_Size.Z);
		break;
	case ESNT_CYLINDER:
		oldSize.X = attribs->getAttributeAsFloat("Radius");
		oldSize.Y = attribs->getAttributeAsFloat("Length");
		oldSize.Z = attribs->getAttributeAsInt("Tesselation");
		attribs->setAttribute("Radius", m_Size.X);
		attribs->setAttribute("Length", m_Size.Y);
		attribs->setAttribute("Tesselation", (irr::s32)m_Size.Z);
		break;
	case ESNT_PLANE:
	{
		irr::core::vector2df _oldTileSize = attribs->getAttributeAsVector2d("TileSize");
		oldTileSize.Width = _oldTileSize.X;
		oldTileSize.Height = _oldTileSize.Y;
		attribs->setAttribute("TileSize", irr::core::vector2df(m_TileSize.Width, m_TileSize.Height));
		attribs->setAttribute("TileCount", m_TileCount);
	} break;
	case irr::scene::ESNT_SKY_DOME:
	{
		oldSize.X = attribs->getAttributeAsFloat("Radius");
		attribs->setAttribute("Radius", m_Size.X);
	} break;
	}

	// store the old size for undo
	m_Size = oldSize;
	m_TileSize = oldTileSize;
	m_TileCount = oldTileCount;

	// update the scene node
	node->deserializeAttributes(attribs);
	attribs->drop();

	// restore the material
	m_SceneMgr->getVideoDriver()->fillMaterialStructureFromAttributes(
		node->getMaterial(0), matAttribs);
	matAttribs->drop();

	// rebuild the triangle selector
	if (node->getType() != irr::scene::ESNT_SKY_DOME)
	{
		irr::scene::IMeshSceneNode* meshNode = static_cast<irr::scene::IMeshSceneNode*>(node);
		irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelector(
			meshNode->getMesh(), meshNode);
		node->setTriangleSelector(selector);
		selector->drop();
	}

	return true;
}

wxString ResizeNodeCommand::GetName(void) const
{
	return _("Resize selection");
}

bool ResizeNodeCommand::Undo(void)
{
	return Do(); // we store the previous details in Do()
}

ChangeColorCommand::ChangeColorCommand(COLOR_TYPE type, irr::scene::ISceneNode* node,
	irr::u32 material, const irr::video::SColorf& color)
	: m_SceneMgr(node->getSceneManager()), m_Type(type), 
	m_Name(node->getName()), m_Material(material), m_Color(color), m_Shiny(0)
{
}

ChangeColorCommand::ChangeColorCommand(COLOR_TYPE type, irr::scene::ISceneNode* node,
	irr::u32 material, const irr::f32& shiny)
	: m_SceneMgr(node->getSceneManager()), m_Type(type), 
	m_Name(node->getName()), m_Material(material), m_Shiny(shiny)
{
}

ChangeColorCommand::~ChangeColorCommand(void)
{
}

bool ChangeColorCommand::CanUndo(void) const
{
	return true;
}

bool ChangeColorCommand::Do(void)
{
	irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName(m_Name);
	if (node->getType() == irr::scene::ESNT_LIGHT)
	{
		irr::scene::ILightSceneNode* light = static_cast<irr::scene::ILightSceneNode*>(node);
		irr::video::SLight& data = light->getLightData();
		switch (m_Type)
		{
		case CT_AMBIENT:
		{
			irr::video::SColorf old(data.AmbientColor);
			data.AmbientColor.set(m_Color.a, m_Color.r, m_Color.g, m_Color.b);
			m_Color.set(old.a, old.r, old.g, old.b);
		} break;
		case CT_DIFFUSE:
		{
			irr::video::SColorf old(data.AmbientColor);
			data.DiffuseColor.set(m_Color.a, m_Color.r, m_Color.g, m_Color.b);
			m_Color.set(old.a, old.r, old.g, old.b);
		} break;
		case CT_SPECULAR:
		{
			irr::video::SColorf old(data.AmbientColor);
			data.SpecularColor.set(m_Color.a, m_Color.r, m_Color.g, m_Color.b);
			m_Color.set(old.a, old.r, old.g, old.b);
		} break;
		}
	}
	else
	{
		irr::video::SMaterial& mat = node->getMaterial(m_Material);
		switch (m_Type)
		{
		case CT_AMBIENT:
		{
			irr::video::SColorf old(mat.AmbientColor);
			mat.AmbientColor.set(m_Color.a, m_Color.r, m_Color.g, m_Color.b);
			m_Color.set(old.a, old.r, old.g, old.b);
		} break;
		case CT_DIFFUSE:
		{
			irr::video::SColorf old(mat.AmbientColor);
			mat.DiffuseColor.set(m_Color.a, m_Color.r, m_Color.g, m_Color.b);
			m_Color.set(old.a, old.r, old.g, old.b);
		} break;
		case CT_EMISSIVE:
		{
			irr::video::SColorf old(mat.AmbientColor);
			mat.EmissiveColor.set(m_Color.a, m_Color.r, m_Color.g, m_Color.b);
			m_Color.set(old.a, old.r, old.g, old.b);
		} break;
		case CT_SPECULAR:
		{
			irr::video::SColorf old(mat.AmbientColor);
			mat.SpecularColor.set(m_Color.a, m_Color.r, m_Color.g, m_Color.b);
			m_Color.set(old.a, old.r, old.g, old.b);
		} break;
		case CT_SHINY:
		{
			irr::f32 old(mat.Shininess);
			mat.Shininess = m_Shiny;
			m_Shiny = old;
		} break;
		}
	}

	return true;
}

wxString ChangeColorCommand::GetName(void) const
{
	switch (m_Type)
	{
	case CT_AMBIENT:
		return _("Update Ambient Color");
	case CT_DIFFUSE:
		return _("Update Diffuse Color");
	case CT_EMISSIVE:
		return _("Update Emissive Color");
	case CT_SPECULAR:
		return _("Update Specular Color");
	case CT_SHINY:
		return _("Update Shininess");
	}

	return _("Update Color");
}

bool ChangeColorCommand::Undo(void)
{
	return Do(); // we capture the changed value during Do()
}

ChangeTextureCommand::ChangeTextureCommand(irr::scene::ISceneNode* node,
	irr::u32 material, irr::u32 textureId, const wxString& texture)
	: m_SceneMgr(node->getSceneManager()),
	  m_Material(material), m_TextureId(textureId)
{
	m_Selection.push_back(node->getName());
	m_Textures.emplace(node->getName(), texture);
}

ChangeTextureCommand::ChangeTextureCommand(irr::scene::ISceneManager* sceneMgr,
	const selection_t& selection, irr::u32 material, irr::u32 textureId,
	const wxString& texture)
	: m_SceneMgr(sceneMgr), m_Selection(selection), m_Material(material),
	  m_TextureId(textureId)
{
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		m_Textures.emplace(*item, texture);
	}
}

ChangeTextureCommand::~ChangeTextureCommand(void)
{
}

bool ChangeTextureCommand::CanUndo(void) const
{
	return true;
}

bool ChangeTextureCommand::Do(void)
{
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		if (node) // should always be there
		{
			irr::video::SMaterial& mat = node->getMaterial(m_Material);

			// get the existing texture
			wxString oldTexture;
			irr::video::ITexture* oldTex = mat.getTexture(m_TextureId - 1);
			if (oldTex)
				oldTexture = oldTex->getName().getPath().c_str();

			if (m_Textures[*item].empty())
				mat.setTexture(m_TextureId - 1, nullptr);
			else
			{
				// load the new texture
				irr::io::path texturePath(m_Textures[*item].ToStdString().c_str());
				irr::video::ITexture* texture = m_SceneMgr->getVideoDriver()->getTexture(
					texturePath);
				if (texture)
					mat.setTexture(m_TextureId - 1, texture);
			}

			m_Textures[*item] = oldTexture; // store the original texture
		}
	}

	return true;
}

wxString ChangeTextureCommand::GetName(void) const
{
	return wxString::Format(_("Update texture %d"), m_TextureId);
}

bool ChangeTextureCommand::Undo(void)
{
	return Do(); // we capture the changed value during Do() 
}

AlignNodeCommand::AlignNodeCommand(irr::scene::ISceneManager* sceneMgr,
	const selection_t& selection, ALIGN_TYPE type)
	: m_SceneMgr(sceneMgr), m_Selection(selection), m_Type(type)
{
	// get all the current positions
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		m_OldPosition.emplace((*item), node->getPosition());
	}
}

AlignNodeCommand::~AlignNodeCommand(void)
{
}

bool AlignNodeCommand::CanUndo(void) const
{
	return true;
}

bool AlignNodeCommand::Do(void)
{
	bool first = true;

	switch (m_Type)
	{
	case ALIGN_TOP:
	{
		// find the bounding box with the highest Ymax
		irr::core::aabbox3df box;
		for (selection_t::iterator item = m_Selection.begin();
			item != m_Selection.end(); ++item)
		{
			irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
			irr::core::aabbox3df aabb = node->getTransformedBoundingBox();
			if (first)
			{
				box.reset(aabb);
				first = false;
			}
			else
			{
				if (aabb.MaxEdge.Y > box.MaxEdge.Y)
					box.reset(aabb);
			}
		}

		// translate each scene node based on the aabb delta
		for (selection_t::iterator item = m_Selection.begin();
			item != m_Selection.end(); ++item)
		{
			irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
			irr::core::aabbox3df aabb = node->getTransformedBoundingBox();
			irr::core::vector3df position = node->getPosition();
			position.Y += box.MaxEdge.Y - aabb.MaxEdge.Y;
			node->setPosition(position);
		}
	} break;
	case ALIGN_MIDDLE:
	{
		// align all the entities to the first middle Y value
		irr::core::aabbox3df box;
		for (selection_t::iterator item = m_Selection.begin();
			item != m_Selection.end(); ++item)
		{
			irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
			irr::core::aabbox3df aabb = node->getTransformedBoundingBox();
			if (first)
			{
				box.reset(aabb);
				first = false;
			}
			else
			{
				irr::core::vector3df position = node->getPosition();
				position.Y += box.getCenter().Y - aabb.getCenter().Y;
				node->setPosition(position);
			}
		}
	} break;
	case ALIGN_BOTTOM:
	{
		// find the bounding box with the lowest Ymin
		irr::core::aabbox3df box;
		for (selection_t::iterator item = m_Selection.begin();
			item != m_Selection.end(); ++item)
		{
			irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
			irr::core::aabbox3df aabb = node->getTransformedBoundingBox();

			if (first)
			{
				box.reset(aabb);
				first = false;
			}
			else
			{
				if (aabb.MinEdge.Y < box.MinEdge.Y)
					box.reset(aabb);
			}
		}

		// translate each scene node based on the aabb delta
		for (selection_t::iterator item = m_Selection.begin();
			item != m_Selection.end(); ++item)
		{
			irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
			irr::core::aabbox3df aabb = node->getTransformedBoundingBox();
			irr::core::vector3df position = node->getPosition();
			position.Y -= aabb.MinEdge.Y - box.MinEdge.Y;
			node->setPosition(position);
		}
	} break;
	}
	return true;
}

wxString AlignNodeCommand::GetName(void) const
{
	switch (m_Type)
	{
	case ALIGN_TOP:
		return _("Align top");
	case ALIGN_MIDDLE:
		return _("Align middle");
	case ALIGN_BOTTOM:
		return _("Align bottom");
	}

	return wxT("Alignment");
}

bool AlignNodeCommand::Undo(void)
{
	// revert all the positions
	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		node->setPosition(m_OldPosition[(*item)]);
	}

	return true;
}

DeleteNodeCommand::DeleteNodeCommand(ExplorerPanel* explorerPanel, 
	irr::scene::ISceneManager* sceneMgr, irr::scene::ISceneNode* mapRoot,
	std::shared_ptr<Map>& map, const selection_t& selection)
	: m_ExplorerPanel(explorerPanel), m_SceneMgr(sceneMgr), m_MapRoot(mapRoot),
	  m_Map(map), m_Selection(selection)
{
}

DeleteNodeCommand::~DeleteNodeCommand(void)
{
}

bool DeleteNodeCommand::CanUndo(void) const
{
	return true;
}

bool DeleteNodeCommand::Do(void)
{
	irr::io::SAttributeReadWriteOptions opts;
	opts.Filename = ".";
	opts.Flags = irr::io::EARWF_USE_RELATIVE_PATHS;

	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::io::IAttributes* attribs = m_SceneMgr->getFileSystem()->createEmptyAttributes(
			m_SceneMgr->getVideoDriver());
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*item).c_str());
		node->serializeAttributes(attribs, &opts);

		m_Type.emplace((*item), node->getType());

		if (m_ExplorerPanel->IsGeometry((*item)))
		{
			irr::scene::IMeshSceneNode* meshNode = static_cast<irr::scene::IMeshSceneNode*>(node);
			irr::video::SMaterial& material = meshNode->getMaterial(0);
			irr::io::IAttributes* materialAttribs = m_SceneMgr->getVideoDriver()->createAttributesFromMaterial(material, &opts);

			m_Geometry.emplace((*item), attribs);
			m_Materials.emplace((*item), materialAttribs);
			m_ExplorerPanel->RemoveGeometry((*item));
		}
		else if (m_ExplorerPanel->IsActor((*item)))
		{
			m_Actors.emplace((*item), attribs);
			m_ExplorerPanel->RemoveActor((*item));

			wxString name(*item);
			name.append(wxT("_marker"));
			irr::scene::ISceneNode* marker = m_SceneMgr->getSceneNodeFromName(name.c_str());
			if (marker)
			{
				irr::io::IAttributes* markerAttribs = m_SceneMgr->getFileSystem()->createEmptyAttributes(
					m_SceneMgr->getVideoDriver());
				
				marker->serializeAttributes(markerAttribs, &opts);
				
				irr::scene::IMeshSceneNode* meshNode = static_cast<irr::scene::IMeshSceneNode*>(marker);
				irr::video::SMaterial& material = meshNode->getMaterial(0);
				irr::io::IAttributes* materialAttribs = m_SceneMgr->getVideoDriver()->createAttributesFromMaterial(material, &opts);

				m_Actors.emplace(name, markerAttribs);
				m_Materials.emplace(name, materialAttribs);
				marker->remove();
			}
		}

		m_Map->RemoveEntity((*item));
		node->remove();
	}

	return true;
}

wxString DeleteNodeCommand::GetName(void) const
{
	return _("Delete selection");
}

bool DeleteNodeCommand::Undo(void)
{
	irr::io::SAttributeReadWriteOptions opts;
	opts.Filename = ".";
	opts.Flags = irr::io::EARWF_USE_RELATIVE_PATHS;

	for (selection_t::iterator item = m_Selection.begin();
		item != m_Selection.end(); ++item)
	{
		irr::scene::ESCENE_NODE_TYPE type = m_Type[(*item)];
		const irr::c8* typeName = m_SceneMgr->getSceneNodeTypeName(type);
		irr::scene::ISceneNode* node = m_SceneMgr->addSceneNode(typeName,
			m_MapRoot);

		// is it geometry
		itemmap_t::iterator _item = m_Geometry.find((*item));
		if (_item != m_Geometry.end())
		{
			node->deserializeAttributes((*_item).second, &opts);
			irr::scene::IMeshSceneNode* meshNode = static_cast<irr::scene::IMeshSceneNode*>(node);
			m_SceneMgr->getVideoDriver()->fillMaterialStructureFromAttributes(node->getMaterial(0),
				m_Materials[(*item)]);

			// set the triangle selector
			if (node->getType() != irr::scene::ESNT_SKY_DOME)
			{
				irr::scene::IMeshSceneNode* meshNode = static_cast<irr::scene::IMeshSceneNode*>(node);
				irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelector(
					meshNode->getMesh(), meshNode);
				node->setTriangleSelector(selector);
				selector->drop();
			}

			// add the geometry
			m_ExplorerPanel->AddGeometry((*item));
		}
		else // Actor
		{
			_item = m_Actors.find((*item));
			node->deserializeAttributes((*_item).second, &opts);

			// extra special case is pathnode, we need to draw any links
			if (type == ESNT_PATHNODE)
				dynamic_cast<PathSceneNode*>(node)->drawLink(true);

			// add the marker
			if (type < TOOL_IRRLICHT_ID)
			{
				wxString name(*item);
				name.append(wxT("_marker"));
				_item = m_Actors.find(name);
				irr::scene::IBillboardSceneNode* marker = m_SceneMgr->addBillboardSceneNode(
					node, irr::core::dimension2df(5, 5), irr::core::vector3df(), NID_NOSAVE);
				marker->deserializeAttributes((*_item).second, &opts);

				m_SceneMgr->getVideoDriver()->fillMaterialStructureFromAttributes(marker->getMaterial(0),
					m_Materials[name]);

				// set the triangle selector
				irr::scene::ITriangleSelector* selector = m_SceneMgr->createTriangleSelectorFromBoundingBox(
					marker);
				node->setTriangleSelector(selector);
				selector->drop();
			}

			// add the actor
			m_ExplorerPanel->AddActor((*item));
		}

		m_Map->AddEntity((*item), _item->second);
	}

	return true;
}

UpdatePathNameCommand::UpdatePathNameCommand(irr::scene::ISceneManager* sceneMgr,
	const wxString& pathNode, const wxString& pathName)
	: m_SceneMgr(sceneMgr), m_PathNode(pathNode), m_PathName(pathName)
{
}

UpdatePathNameCommand::~UpdatePathNameCommand(void)
{
}

bool UpdatePathNameCommand::CanUndo(void) const
{
	return true;
}

bool UpdatePathNameCommand::Do(void)
{
	PathSceneNode* pathNode = dynamic_cast<PathSceneNode*>(
		m_SceneMgr->getSceneNodeFromName(m_PathNode.c_str().AsChar()));

	// store the current path name
	wxString oldPathName(pathNode->getPathName().c_str());

	pathNode->setPathName(m_PathName.c_str().AsChar());

	// update all connected path nodes
	PathSceneNode* nextNode = pathNode->getNext();
	while (nextNode && nextNode != pathNode)
	{
		nextNode->setPathName(m_PathName.c_str().AsChar());
		nextNode = nextNode->getNext();
	}

	PathSceneNode* prevNode = pathNode->getPrev();
	while (prevNode && prevNode != pathNode)
	{
		prevNode->setPathName(m_PathName.c_str().AsChar());
		prevNode = prevNode->getPrev();
	}

	// store the Undo() details
	m_PathName = oldPathName;

	return true;
}

wxString UpdatePathNameCommand::GetName(void) const
{
	return _("Update path name");
}

bool UpdatePathNameCommand::Undo(void)
{
	return Do(); // the undo details are captured during Do()
}

UpdatePathLinkCommand::UpdatePathLinkCommand(irr::scene::ISceneManager* sceneMgr,
	const wxString& pathNode, const wxString& prevNode, const wxString& nextNode,
	bool updatePrev, bool updateNext)
	: m_SceneMgr(sceneMgr), m_PathNode(pathNode), m_PrevNode(prevNode),
	  m_NextNode(nextNode), m_UpdatePrev(updatePrev), m_UpdateNext(updateNext)
{
}

UpdatePathLinkCommand::~UpdatePathLinkCommand(void)
{
}

bool UpdatePathLinkCommand::CanUndo(void) const
{
	return true;
}

bool UpdatePathLinkCommand::Do(void)
{
	PathSceneNode* pathNode = dynamic_cast<PathSceneNode*>(
		m_SceneMgr->getSceneNodeFromName(m_PathNode.c_str().AsChar()));
	if (pathNode == nullptr)
		return false;

	if (m_UpdatePrev)
	{
		wxString oldPrevNode;
		PathSceneNode* prevNode = pathNode->getPrev();
		if (prevNode)
			oldPrevNode = prevNode->getName();

		if (m_PrevNode.CompareTo(wxT("--none--")) == 0)
		{
			pathNode->setPrev(nullptr);
			pathNode->drawLink(false);
		}
		else
		{
			if (m_PrevNode.empty())
			{
				pathNode->setPrev(nullptr);
				pathNode->drawLink(false);
			}
			else
			{
				PathSceneNode* prevNode = dynamic_cast<PathSceneNode*>(
					m_SceneMgr->getSceneNodeFromName(m_PrevNode.c_str().AsChar()));
				if (prevNode)
				{
					pathNode->setPrev(prevNode);
					pathNode->setPathName(prevNode->getPathName());
					pathNode->drawLink(true);
				}
			}
		}

		m_PrevNode = oldPrevNode; // store the old prev node for undo
	}
	else if (m_UpdateNext)
	{
		wxString oldNextNode;
		PathSceneNode* nextNode = pathNode->getNext();
		if (nextNode)
			oldNextNode = nextNode->getName();

		if (m_NextNode.CompareTo(wxT("--none--")) == 0)
		{
			pathNode->setNext(nullptr);
			pathNode->drawLink(false);
		}
		else
		{
			if (m_NextNode.empty())
			{
				pathNode->setNext(nullptr);
				pathNode->drawLink(false);
			}
			else
			{
				PathSceneNode* nextNode = dynamic_cast<PathSceneNode*>(
					m_SceneMgr->getSceneNodeFromName(m_NextNode.c_str().AsChar()));
				if (nextNode)
				{
					pathNode->setNext(nextNode);
					pathNode->setPathName(nextNode->getPathName());
					pathNode->drawLink(true);
				}
			}
		}

		m_NextNode = oldNextNode; // store the old next node for undo
	}
	else
		return false;

	return true;
}

wxString UpdatePathLinkCommand::GetName(void) const
{
	return wxString::Format(_("Update path link: %s"), m_PathNode.c_str().AsChar());
}

bool UpdatePathLinkCommand::Undo(void)
{
	return Do();
}

UpdateActorAttributeCommand::UpdateActorAttributeCommand(irr::io::E_ATTRIBUTE_TYPE type,
	const wxString& sceneNode, std::shared_ptr<Map>& map,
	PropertyPanel* propertyPanel, const wxString& attribute, const wxString& value)
	: m_Type(type), m_SceneNode(sceneNode), m_Map(map), m_PropertyPanel(propertyPanel),
	  m_Attribute(attribute), m_Value(value)
{
}

UpdateActorAttributeCommand::~UpdateActorAttributeCommand(void)
{
}

bool UpdateActorAttributeCommand::CanUndo(void) const
{
	return true;
}

bool UpdateActorAttributeCommand::Do(void)
{
	irr::io::IAttributes* attribs = m_Map->GetAttributes(m_SceneNode.c_str().AsChar());
	wxString oldValue = attribs->getAttributeAsString(m_Attribute.c_str().AsChar()).c_str();

	switch (m_Type)
	{
	case irr::io::EAT_STRING:
		attribs->setAttribute(m_Attribute.c_str().AsChar(), m_Value.c_str().AsChar());
		break;
	case irr::io::EAT_VECTOR3D:
		attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToVec3(m_Value));
		break;
	case irr::io::EAT_VECTOR2D:
		attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToVec2(m_Value));
		break;
	case irr::io::EAT_COLOR:
		attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToColor(m_Value));
		break;
	case irr::io::EAT_FLOAT:
		attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToFloat(m_Value));
		break;
	case irr::io::EAT_BOOL:
		attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToBool(m_Value));
		break;
	case irr::io::EAT_INT:
		attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToInt(m_Value));
		break;
	}
	
	m_Value = oldValue; // for undo

	return true;
}

wxString UpdateActorAttributeCommand::GetName(void) const
{
	return wxString::Format(_("Update actor attribute: %s"), m_Attribute.c_str().AsChar());
}

bool UpdateActorAttributeCommand::Undo(void)
{
	if (Do())
	{
		m_PropertyPanel->Refresh();
		return true;
	}

	return false;
}

UpdateComponentAttributeCommand::UpdateComponentAttributeCommand(irr::io::E_ATTRIBUTE_TYPE type,
	const wxString& sceneNode, std::shared_ptr<Map>& map,
	PropertyPanel* propertyPanel, const irr::scene::ESCENE_NODE_ANIMATOR_TYPE& component,
	const wxString& attribute, const wxString& value)
	: m_Type(type), m_SceneNode(sceneNode), m_Map(map),
	  m_PropertyPanel(propertyPanel), m_Component(component),
	  m_Attribute(attribute), m_Value(value)
{
}

UpdateComponentAttributeCommand::~UpdateComponentAttributeCommand(void)
{
}

bool UpdateComponentAttributeCommand::CanUndo(void) const
{
	return true;
}

bool UpdateComponentAttributeCommand::Do(void)
{
	// look up the scene node
	irr::scene::ISceneNode* node = m_Map->GetSceneMgr()->getSceneNodeFromName(
		m_SceneNode.c_str().AsChar());
	if (node == nullptr)
		return false;

	// look up the animator
	irr::io::IAttributes* attribs = m_Map->GetSceneMgr()->getFileSystem()->createEmptyAttributes();
	const irr::scene::ISceneNodeAnimatorList animators = node->getAnimators();
	for (irr::scene::ISceneNodeAnimatorList::ConstIterator i = animators.begin();
		i != animators.end(); ++i)
	{
		if ((*i)->getType() == m_Component)
		{
			(*i)->serializeAttributes(attribs);

			wxString oldValue;

			switch (m_Type)
			{
			case irr::io::EAT_INT:
				oldValue = wxString::Format("%d", attribs->getAttributeAsInt(m_Attribute.c_str().AsChar()));
				attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToInt(m_Value));
				break;
			case irr::io::EAT_FLOAT:
				oldValue = wxString::Format("%g", attribs->getAttributeAsFloat(m_Attribute.c_str().AsChar()));
				attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToFloat(m_Value));
				break;
			case irr::io::EAT_STRING:
				oldValue = wxString::Format("%s", attribs->getAttributeAsString(m_Attribute.c_str().AsChar()).c_str());
				attribs->setAttribute(m_Attribute.c_str().AsChar(), m_Value.c_str().AsChar());
				break;
			case irr::io::EAT_VECTOR3D:
			{
				irr::core::vector3df vec = attribs->getAttributeAsVector3d(m_Attribute.c_str().AsChar());
				oldValue = wxString::Format("%g; %g; %g", vec.X, vec.Y, vec.Z);
				attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToVec3(m_Value));
			} break;
			case irr::io::EAT_VECTOR2D:
			{
				irr::core::vector2df vec = attribs->getAttributeAsVector2d(m_Attribute.c_str().AsChar());
				oldValue = wxString::Format("%g; %g", vec.X, vec.Y);
				attribs->setAttribute(m_Attribute.c_str().AsChar(), valueToVec2(m_Value));
			} break;
			}

			// write the attribute back to the animator
			(*i)->deserializeAttributes(attribs);

			m_Value = oldValue; // for undo
			break;
		}
	}

	attribs->drop();

	return true;
}

wxString UpdateComponentAttributeCommand::GetName(void) const
{
	return wxString::Format(_("Update attribute: %s"), 
		m_Attribute.c_str().AsChar());
}

bool UpdateComponentAttributeCommand::Undo(void)
{
	if (Do())
	{
		m_PropertyPanel->Refresh();
		return true;
	}

	return false;
}
