/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "PlayerStartNode.hpp"

PlayerStartNode::PlayerStartNode(irr::scene::ISceneNode* parent,
	irr::scene::ISceneManager* mgr, irr::s32 id,
	const irr::core::vector3df& position,
	const irr::core::vector3df& rotation,
	const irr::core::vector3df& scale)
	: irr::scene::ISceneNode(parent, mgr, id, position, rotation, scale)
{
#ifdef _DEBUG
	setDebugName("PlayerStartNode");
#endif

	m_Aabb.MinEdge.set(-5, -5, -5);
	m_Aabb.MaxEdge.set(5, 5, 5);
}

PlayerStartNode::~PlayerStartNode(void)
{
}

void PlayerStartNode::OnRegisterSceneNode(void)
{
	ISceneNode::OnRegisterSceneNode();
}

void PlayerStartNode::render(void)
{
	// no-op
}

const irr::core::aabbox3d<irr::f32>& PlayerStartNode::getBoundingBox(void) const
{
	return m_Aabb;
}

irr::scene::ISceneNode* PlayerStartNode::clone(ISceneNode* newParent, 
	irr::scene::ISceneManager* newManager)
{
	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	PlayerStartNode* nb = new PlayerStartNode(newParent, newManager, ID, RelativeTranslation, 
		RelativeRotation, RelativeScale);

	nb->cloneMembers(this, newManager);

	if (newParent)
		nb->drop();

	return nb;
}
