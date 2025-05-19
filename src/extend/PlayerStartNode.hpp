/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "irrlicht.h"

#define ESNT_PLAYERSTART MAKE_IRR_ID('s', 't', 'r', 't')

class PlayerStartNode : public irr::scene::ISceneNode
{
private:
	irr::core::aabbox3d<irr::f32> m_Aabb;

public:
	PlayerStartNode(irr::scene::ISceneNode* parent,
		irr::scene::ISceneManager* mgr, irr::s32 id,
		const irr::core::vector3df& position = irr::core::vector3df(0, 0, 0),
		const irr::core::vector3df& rotation = irr::core::vector3df(0, 0, 0),
		const irr::core::vector3df& scale = irr::core::vector3df(1.0f, 1.0f, 1.0f));
	virtual ~PlayerStartNode(void);

	virtual void OnRegisterSceneNode(void);

	//! renders the node.
	virtual void render(void);

	//! returns the axis aligned bounding box of this node
	const irr::core::aabbox3d<irr::f32>& getBoundingBox(void) const;

	//! Returns type of the scene node
	virtual irr::scene::ESCENE_NODE_TYPE getType(void) const { return (irr::scene::ESCENE_NODE_TYPE)ESNT_PLAYERSTART; }

	//! Creates a clone of this scene node and its children.
	virtual irr::scene::ISceneNode* clone(ISceneNode* newParent = 0, irr::scene::ISceneManager* newManager = 0);
};
