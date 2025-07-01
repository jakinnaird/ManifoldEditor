/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "SceneNodeFactory.hpp"
#include "CylinderSceneNode.hpp"
#include "PlaneSceneNode.hpp"
#include "PlayerStartNode.hpp"
#include "PathSceneNode.hpp"
#include "../editor/UpdatableTerrainSceneNode.hpp"

SceneNodeFactory::SceneNodeFactory(irr::scene::ISceneManager* sceneMgr)
	: m_SceneMgr(sceneMgr)
{
#ifdef _DEBUG
	setDebugName("SceneNodeFactory");
#endif

	m_SupportedSceneNodeTypes.push_back(SceneNodeType((irr::scene::ESCENE_NODE_TYPE)ESNT_CYLINDER, "cylinder"));
	m_SupportedSceneNodeTypes.push_back(SceneNodeType((irr::scene::ESCENE_NODE_TYPE)ESNT_PLANE, "plane"));
	m_SupportedSceneNodeTypes.push_back(SceneNodeType((irr::scene::ESCENE_NODE_TYPE)ESNT_PLAYERSTART, "playerstart"));
	m_SupportedSceneNodeTypes.push_back(SceneNodeType((irr::scene::ESCENE_NODE_TYPE)ESNT_PATHNODE, "pathnode"));
	
	// override the terrain node type
	m_SupportedSceneNodeTypes.push_back(SceneNodeType(irr::scene::ESNT_TERRAIN, "terrain"));
}

SceneNodeFactory::~SceneNodeFactory(void)
{
}

irr::scene::ISceneNode* SceneNodeFactory::addSceneNode(irr::scene::ESCENE_NODE_TYPE type, 
	irr::scene::ISceneNode* parent)
{
	if (!parent)
		parent = m_SceneMgr->getRootSceneNode();

	irr::scene::ISceneNode* node = nullptr;

	switch (type)
	{
	case ESNT_CYLINDER:
		node = new CylinderSceneNode(5, 10, 8, true, 0.0f, parent, m_SceneMgr, -1);
		break;
	case ESNT_PLANE:
		node = new PlaneSceneNode(irr::core::dimension2df(10, 10), irr::core::dimension2du(1, 1),
			parent, m_SceneMgr, -1);
		break;
	case ESNT_PLAYERSTART:
		node = new PlayerStartNode(parent, m_SceneMgr, -1);
		break;
	case ESNT_PATHNODE:
		node = new PathSceneNode(parent, m_SceneMgr, -1);
		break;
	// case ESNT_UPDATABLE_TERRAIN:
	case irr::scene::ESNT_TERRAIN:
		{
			UpdatableTerrainSceneNode* terrain = new UpdatableTerrainSceneNode(
				parent, m_SceneMgr, m_SceneMgr->getFileSystem(), -1, 5, irr::scene::ETPS_17);
			// Create a default 257x257 heightmap (good size for terrain)
			terrain->createHeightmap(257, 0.0f);
			// Set some basic material properties
			terrain->getMaterial(0).setTexture(0, m_SceneMgr->getVideoDriver()->getTexture("editor.mpk:textures/default.jpg"));
			terrain->getMaterial(0).Lighting = false;
			node = terrain;
		}
		break;
	}

	if (node)
		node->drop();

	return node;
}

irr::scene::ISceneNode* SceneNodeFactory::addSceneNode(const irr::c8* typeName,
	irr::scene::ISceneNode* parent)
{
	return addSceneNode(getTypeFromName(typeName), parent);
}

irr::u32 SceneNodeFactory::getCreatableSceneNodeTypeCount() const
{
	return m_SupportedSceneNodeTypes.size();
}

irr::scene::ESCENE_NODE_TYPE SceneNodeFactory::getCreateableSceneNodeType(irr::u32 idx) const
{
	if (idx < m_SupportedSceneNodeTypes.size())
		return m_SupportedSceneNodeTypes[idx].Type;

	return irr::scene::ESNT_UNKNOWN;
}

const irr::c8* SceneNodeFactory::getCreateableSceneNodeTypeName(irr::u32 idx) const
{
	if (idx < m_SupportedSceneNodeTypes.size())
		return m_SupportedSceneNodeTypes[idx].TypeName.c_str();

	return nullptr;
}

const irr::c8* SceneNodeFactory::getCreateableSceneNodeTypeName(irr::scene::ESCENE_NODE_TYPE type) const
{
	for (irr::u32 i = 0; i < m_SupportedSceneNodeTypes.size(); ++i)
	{
		if (m_SupportedSceneNodeTypes[i].Type == type)
			return m_SupportedSceneNodeTypes[i].TypeName.c_str();
	}

	return nullptr;
}

irr::scene::ESCENE_NODE_TYPE SceneNodeFactory::getTypeFromName(const irr::c8* name) const
{
	for (irr::u32 i = 0; i < m_SupportedSceneNodeTypes.size(); ++i)
	{
		if (m_SupportedSceneNodeTypes[i].TypeName == name)
			return m_SupportedSceneNodeTypes[i].Type;
	}

	return irr::scene::ESNT_UNKNOWN;
}
