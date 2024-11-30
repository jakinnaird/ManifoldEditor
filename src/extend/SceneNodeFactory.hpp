/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "irrlicht.h"

// creates the following scene node types
// - CylinderSceneNode
// - PlaneSceneNode
// - PlayerStartNode
// - PathSceneNode

class SceneNodeFactory : public irr::scene::ISceneNodeFactory
{
private:
	irr::scene::ISceneManager* m_SceneMgr;

	struct SceneNodeType
	{
		SceneNodeType(irr::scene::ESCENE_NODE_TYPE type, const irr::c8* name)
			: Type(type), TypeName(name)
		{}

		irr::scene::ESCENE_NODE_TYPE Type;
		irr::core::stringc TypeName;
	};

	irr::core::array<SceneNodeType> m_SupportedSceneNodeTypes;

public:
	SceneNodeFactory(irr::scene::ISceneManager* sceneMgr);
	~SceneNodeFactory(void);

	irr::scene::ISceneNode* addSceneNode(irr::scene::ESCENE_NODE_TYPE type, irr::scene::ISceneNode* parent = 0);
	irr::scene::ISceneNode* addSceneNode(const irr::c8* typeName, irr::scene::ISceneNode* parent = 0);
	irr::u32 getCreatableSceneNodeTypeCount() const;
	irr::scene::ESCENE_NODE_TYPE getCreateableSceneNodeType(irr::u32 idx) const;
	const irr::c8* getCreateableSceneNodeTypeName(irr::u32 idx) const;
	const irr::c8* getCreateableSceneNodeTypeName(irr::scene::ESCENE_NODE_TYPE type) const;

private:
	irr::scene::ESCENE_NODE_TYPE getTypeFromName(const irr::c8* name) const;
};
