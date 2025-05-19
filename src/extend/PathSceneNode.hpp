/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "irrlicht.h"

#define ESNT_PATHNODE MAKE_IRR_ID('p', 'a', 't', 'h')

class PathSceneNode : public irr::scene::ISceneNode
{
private:
	irr::core::aabbox3d<irr::f32> m_Aabb;

	irr::core::stringc m_PathName;
	irr::core::stringc m_PrevNode;
	irr::core::stringc m_NextNode;

	PathSceneNode* m_Next;
	PathSceneNode* m_Prev;

	bool m_DrawLink;

public:
	PathSceneNode(irr::scene::ISceneNode* parent,
		irr::scene::ISceneManager* mgr, irr::s32 id,
		const irr::core::vector3df& position = irr::core::vector3df(0, 0, 0),
		const irr::core::vector3df& rotation = irr::core::vector3df(0, 0, 0),
		const irr::core::vector3df& scale = irr::core::vector3df(1.0f, 1.0f, 1.0f));
	virtual ~PathSceneNode(void);

	virtual void OnRegisterSceneNode(void);

	//! renders the node.
	virtual void render(void);

	//! returns the axis aligned bounding box of this node
	const irr::core::aabbox3d<irr::f32>& getBoundingBox(void) const;

	//! Returns type of the scene node
	virtual irr::scene::ESCENE_NODE_TYPE getType(void) const { return (irr::scene::ESCENE_NODE_TYPE)ESNT_PATHNODE; }

	//! Creates a clone of this scene node and its children.
	virtual irr::scene::ISceneNode* clone(ISceneNode* newParent = 0, irr::scene::ISceneManager* newManager = 0);

	virtual void serializeAttributes(irr::io::IAttributes* out, irr::io::SAttributeReadWriteOptions* options) const;
	virtual void deserializeAttributes(irr::io::IAttributes* in, irr::io::SAttributeReadWriteOptions* options);

	void drawLink(bool draw) { m_DrawLink = draw; }

	void setPathName(const irr::core::stringc& pathName);
	const irr::core::stringc& getPathName(void);

	void setNext(PathSceneNode* pathNode);
	void setPrev(PathSceneNode* pathNode);
	PathSceneNode* getPrev(void);
	PathSceneNode* getNext(void);
};
