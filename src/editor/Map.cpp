/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Commands.hpp"
#include "Common.hpp"
#include "ExplorerPanel.hpp"
#include "Map.hpp"
#include "Serialize.hpp"

#include "../extend/CylinderSceneNode.hpp"
#include "../extend/PlaneSceneNode.hpp"
#include "../extend/PlayerStartNode.hpp"
#include "../extend/PathSceneNode.hpp"

#include <wx/log.h>
#include <wx/stdpaths.h>

Map::Map(void)
	: m_SceneMgr(nullptr), m_MapRoot(nullptr)
{
	m_NextId = 1;
	m_Lighting = false;
}

Map::Map(const wxFileName& fileName)
	: m_SceneMgr(nullptr), m_MapRoot(nullptr), m_FileName(fileName)
{
	m_NextId = 1;
	m_Lighting = false;
}

Map::~Map(void)
{
	for (entities_t::iterator entity = m_Entities.begin();
		entity != m_Entities.end(); ++entity)
		entity->second->drop();

	if (m_SceneMgr)
		m_SceneMgr->drop();
}

void Map::SetSceneMgr(irr::scene::ISceneManager* sceneMgr)
{
	m_SceneMgr = sceneMgr;
	m_SceneMgr->grab();
}

irr::scene::ISceneManager* Map::GetSceneMgr(void)
{
	return m_SceneMgr;
}

bool Map::HasFilename(void)
{
	return m_FileName.IsOk();
}

const wxFileName& Map::GetFileName(void)
{
	return m_FileName;
}

void Map::Save(const wxFileName& fileName)
{
	// pick the right output file name
	wxFileName outFileName(fileName);
	if (!outFileName.IsOk())
		outFileName = m_FileName; // called by MainWindow::Save

	// get the serializer
	std::shared_ptr<Serializer> serializer =
		ISerializerFactory::GetSave(outFileName);
	serializer->SetFileSystem(m_SceneMgr->getFileSystem());
	serializer->SetVideoDriver(m_SceneMgr->getVideoDriver());

	if (serializer->Begin(m_NextId))
		m_FileName = outFileName;
	else
	{
		wxLogError(_("Unable to save map"));
		return;
	}

	// always use relative paths
	irr::io::SAttributeReadWriteOptions opts;
	opts.Filename = ".";
	opts.Flags = irr::io::EARWF_USE_RELATIVE_PATHS;

	// process the map entities
	for (entities_t::iterator entity = m_Entities.begin();
		entity != m_Entities.end(); ++entity)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName((*entity).first.c_str(), nullptr);
		if (node)
		{
			// do we save this one
			if (node->getID() & NID_NOSAVE)
				continue;

			// if the node has debug data turned on, store it for later and turn it off while saving
			irr::u32 debugData = node->isDebugDataVisible();
			node->setDebugDataVisible(irr::scene::EDS_OFF);

			irr::io::IAttributes* attributes = m_SceneMgr->getFileSystem()->createEmptyAttributes();
			node->serializeAttributes(attributes, &opts);
			
			irr::core::array<irr::io::IAttributes*> materials;
			for (irr::u32 i = 0; i < node->getMaterialCount(); ++i)
			{
				irr::video::SMaterial& material = node->getMaterial(i);
				irr::io::IAttributes* matAttribs = m_SceneMgr->getVideoDriver()->createAttributesFromMaterial(
					material, &opts);
				materials.push_back(matAttribs);
			}

			irr::core::array<irr::io::IAttributes*> animators;
			const irr::scene::ISceneNodeAnimatorList animator = node->getAnimators();
			for (irr::scene::ISceneNodeAnimatorList::ConstIterator i = animator.begin();
				i != animator.end(); ++i)
			{
				irr::io::IAttributes* animAttribs = m_SceneMgr->getFileSystem()->createEmptyAttributes();
				irr::scene::ESCENE_NODE_ANIMATOR_TYPE type = (*i)->getType();
				irr::u32 factoryCount = m_SceneMgr->getRegisteredSceneNodeAnimatorFactoryCount();
				for (irr::u32 j = 0; j < factoryCount; ++j)
				{
					irr::scene::ISceneNodeAnimatorFactory* factory = m_SceneMgr->getSceneNodeAnimatorFactory(j);
					const irr::c8* name = factory->getCreateableSceneNodeAnimatorTypeName(type);
					if (name)
					{
						(*i)->serializeAttributes(animAttribs, &opts);
						if (!animAttribs->existsAttribute("Type"))
							animAttribs->setAttribute("Type", name);
						animators.push_back(animAttribs);
						break;
					}
				}
			}

			irr::io::IAttributes* userData = GetAttributes((*entity).first);
			userData->grab();

			//irr::scene::ESCENE_NODE_TYPE type = node->getType();
			irr::core::stringc type = m_SceneMgr->getSceneNodeTypeName(node->getType());
			const irr::scene::ISceneNodeList& children = node->getChildren();
			bool child = false;
			for (irr::scene::ISceneNodeList::ConstIterator i = children.begin();
				i != children.end(); ++i)
			{
				if (!((*i)->getID() & NID_NOSAVE))
				{
					child = true;
					break;
				}
			}

			// restore the debug data
			node->setDebugDataVisible(debugData);

			serializer->Next(type, attributes, materials, animators, userData, child);
		}
	}

	serializer->Finalize();
}

void Map::Load(irr::scene::ISceneNode* mapRoot, 
	ExplorerPanel* explorerPanel)
{
	m_MapRoot = mapRoot;

	if (!HasFilename())
		return; // new map

	// get the serializer
	std::shared_ptr<Serializer> serializer =
		ISerializerFactory::GetLoad(m_FileName);
	serializer->SetFileSystem(m_SceneMgr->getFileSystem());
	serializer->SetVideoDriver(m_SceneMgr->getVideoDriver());

	if (!serializer->Begin(m_NextId))
	{
		wxLogError(L"Unable to load map");
		return;
	}

	// always use relative paths
	irr::io::SAttributeReadWriteOptions opts;
	opts.Filename = ".";
	opts.Flags = irr::io::EARWF_USE_RELATIVE_PATHS;

	irr::core::stringc type;
	irr::io::IAttributes* attributes = m_SceneMgr->getFileSystem()->createEmptyAttributes(
		m_SceneMgr->getVideoDriver());
	irr::core::array<irr::io::IAttributes*> materials;
	irr::core::array<irr::io::IAttributes*> animators;
	irr::io::IAttributes* userData = m_SceneMgr->getFileSystem()->createEmptyAttributes(
		m_SceneMgr->getVideoDriver());
	bool child = false;
	std::shared_ptr<Map> self = shared_from_this();
	while (serializer->Next(type, attributes, materials, animators, userData, child))
	{
		wxString _type = type.c_str();
		wxString name = attributes->getAttributeAsString("Name").c_str();
		AddNodeCommand cmd(_type, explorerPanel, m_SceneMgr,
			m_MapRoot, self, name);
		if (!cmd.Do())
			continue;

		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName(name.c_str().AsChar(),
			m_MapRoot);
		node->deserializeAttributes(attributes, &opts);

		for (irr::u32 i = 0; i < materials.size(); ++i)
		{
			if (node->getMaterialCount() > i)
			{
				m_SceneMgr->getVideoDriver()->fillMaterialStructureFromAttributes(
					node->getMaterial(i), materials[i]);
			}

			materials[i]->drop();
		}

		for (irr::u32 i = 0; i < animators.size(); ++i)
		{
			irr::core::stringc animType = animators[i]->getAttributeAsString("Type");
			irr::scene::ISceneNodeAnimator* animator = m_SceneMgr->createSceneNodeAnimator(animType.c_str(), node);
			if (animator)
			{
				animator->deserializeAttributes(animators[i], &opts);
				animator->drop();
			}

			animators[i]->drop();
		}

		// create a triangle selector if one doesn't exist, typically used by actors
		if (!node->getTriangleSelector())
		{
			irr::scene::ITriangleSelector* selector = nullptr;
			if (node->getType() == irr::scene::ESNT_MESH)
				selector = m_SceneMgr->createTriangleSelector(
					static_cast<irr::scene::IMeshSceneNode*>(node)->getMesh(), node);
			else if (node->getType() == irr::scene::ESNT_ANIMATED_MESH)
				selector = m_SceneMgr->createTriangleSelector(
					static_cast<irr::scene::IAnimatedMeshSceneNode*>(node)->getMesh(), node);
			else
				selector = m_SceneMgr->createTriangleSelectorFromBoundingBox(node);

			if (selector)
			{
				node->setTriangleSelector(selector);
				selector->drop();
			}
		}

		// set the custom attributes
		irr::io::IAttributes* attribs = GetAttributes(name);
		if (attribs)
		{
			for (irr::u32 i = 0; i < userData->getAttributeCount(); ++i)
			{
				switch (userData->getAttributeType(i))
				{
				case irr::io::EAT_STRING:
					attribs->addString(userData->getAttributeName(i), 
						userData->getAttributeAsString(i).c_str());
					break;
				case irr::io::EAT_VECTOR3D:
					attribs->addVector3d(userData->getAttributeName(i), 
						userData->getAttributeAsVector3d(i));
					break;
				case irr::io::EAT_VECTOR2D:
					attribs->addVector2d(userData->getAttributeName(i), 
						userData->getAttributeAsVector2d(i));
					break;
				case irr::io::EAT_COLOR:
					attribs->addColor(userData->getAttributeName(i), 
						userData->getAttributeAsColor(i));
					break;
				case irr::io::EAT_FLOAT:
					attribs->addFloat(userData->getAttributeName(i), 
						userData->getAttributeAsFloat(i));
					break;
				case irr::io::EAT_INT:
					attribs->addInt(userData->getAttributeName(i), 
						userData->getAttributeAsInt(i));
					break;
				case irr::io::EAT_BOOL:
					attribs->addBool(userData->getAttributeName(i), 
						userData->getAttributeAsBool(i));
					break;
				}
			}
		}

		materials.clear();
		attributes->clear();
	}

	serializer->Finalize();

	explorerPanel->SetMapName(m_FileName.GetFullName());
}

wxString Map::NextName(const wxString& base)
{
	wxString id;
	
	do
	{
		if (m_NextId < 100)
			id = wxString::Format("%03d", m_NextId++);
		else
			id = wxString::Format("%d", m_NextId++);
	} while (m_SceneMgr->getSceneNodeFromName(id.ToStdString().c_str()));

	return wxString(base).Append(id);
}

void Map::AddEntity(const wxString& name, irr::io::IAttributes* attribs)
{
	m_Entities.emplace(name, attribs);
}

void Map::RemoveEntity(const wxString& name)
{
	entities_t::iterator entity = m_Entities.find(name);
	if (entity != m_Entities.end())
	{
		entity->second->drop();
		m_Entities.erase(entity);
	}
}

void Map::RecomputeLighting(bool lighting)
{
	// walk all the map nodes and enable lighting
	for (entities_t::iterator i = m_Entities.begin();
		i != m_Entities.end(); ++i)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName(i->first.c_str());
		if (node)
			node->setMaterialFlag(irr::video::EMF_LIGHTING, lighting);
	}

	m_Lighting = lighting;
}

bool Map::IsLighting(void)
{
	return m_Lighting;
}

irr::io::IAttributes* Map::GetAttributes(const wxString& entityName)
{
	entities_t::iterator entity = m_Entities.find(entityName);
	if (entity == m_Entities.end())
		return nullptr;

	return entity->second;
}
