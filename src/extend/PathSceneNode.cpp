/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "PathSceneNode.hpp"

PathSceneNode::PathSceneNode(irr::scene::ISceneNode* parent,
	irr::scene::ISceneManager* mgr, irr::s32 id,
	const irr::core::vector3df& position,
	const irr::core::vector3df& rotation,
	const irr::core::vector3df& scale)
	: irr::scene::ISceneNode(parent, mgr, id, position, rotation, scale)
{
#ifdef _DEBUG
	setDebugName("PathSceneNode");
#endif

	m_Aabb.MinEdge.set(-5, -5, -5);
	m_Aabb.MaxEdge.set(5, 5, 5);

	m_Next = nullptr;
	m_Prev = nullptr;

	m_DrawLink = false;
}

PathSceneNode::~PathSceneNode(void)
{
}

void PathSceneNode::OnRegisterSceneNode(void)
{
	if (IsVisible && m_DrawLink)
		SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}

void PathSceneNode::render(void)
{
	if (m_DrawLink)
	{
		PathSceneNode* next = getNext();
		if (next)
		{
			SceneManager->getVideoDriver()->draw3DLine(getPosition(),
				next->getPosition());
		}
	}
}

const irr::core::aabbox3d<irr::f32>& PathSceneNode::getBoundingBox(void) const
{
	return m_Aabb;
}

irr::scene::ISceneNode* PathSceneNode::clone(ISceneNode* newParent,
	irr::scene::ISceneManager* newManager)
{
	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	PathSceneNode* nb = new PathSceneNode(newParent, newManager, ID, RelativeTranslation,
		RelativeRotation, RelativeScale);

	nb->cloneMembers(this, newManager);

	if (newParent)
		nb->drop();

	return nb;
}

void PathSceneNode::serializeAttributes(irr::io::IAttributes* out, irr::io::SAttributeReadWriteOptions* options) const
{
	ISceneNode::serializeAttributes(out, options);

	out->addString("PathName", m_PathName.c_str());
	out->addString("NextNode", m_NextNode.c_str());
	out->addString("PrevNode", m_PrevNode.c_str());
}

void PathSceneNode::deserializeAttributes(irr::io::IAttributes* in, irr::io::SAttributeReadWriteOptions* options)
{
	m_PathName = in->getAttributeAsString("PathName");
	m_NextNode = in->getAttributeAsString("NextNode");
	m_PrevNode = in->getAttributeAsString("PrevNode");

	ISceneNode::deserializeAttributes(in, options);
}

void PathSceneNode::setPathName(const irr::core::stringc& pathName)
{
	m_PathName = pathName;
}

const irr::core::stringc& PathSceneNode::getPathName(void)
{
	return m_PathName;
}

void PathSceneNode::setNext(PathSceneNode* pathNode)
{
	if (m_Next)
	{
		// clear out the next node's config
		m_Next->m_PrevNode = "";
		m_Next->m_Prev = nullptr;
	}

	m_NextNode = "";
	m_Next = nullptr;

	if (pathNode)
	{
		m_NextNode = pathNode->getName();
		m_Next = pathNode;

		pathNode->m_PrevNode = getName();
		pathNode->m_Prev = this;
	}
}

void PathSceneNode::setPrev(PathSceneNode* pathNode)
{
	if (m_Prev)
	{
		// clear out the prev node's config
		m_Prev->m_NextNode = "";
		m_Prev->m_Next = nullptr;
	}

	m_PrevNode = "";
	m_Prev = nullptr;

	if (pathNode)
	{
		m_PrevNode = pathNode->getName();
		m_Prev = pathNode;

		pathNode->m_NextNode = getName();
		pathNode->m_Next = this;
	}
}

PathSceneNode* PathSceneNode::getPrev(void)
{
	if (m_Prev)
		return m_Prev;

	if (m_PrevNode.empty())
		return nullptr;

	m_Prev = (PathSceneNode*)SceneManager->getSceneNodeFromName(m_PrevNode.c_str());
	return m_Prev;
}

PathSceneNode* PathSceneNode::getNext(void)
{
	if (m_Next)
		return m_Next;

	if (m_NextNode.empty())
		return nullptr;

	m_Next = (PathSceneNode*)SceneManager->getSceneNodeFromName(m_NextNode.c_str());
	return m_Next;
}
