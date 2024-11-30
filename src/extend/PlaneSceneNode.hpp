/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "irrlicht.h"

#define ESNT_PLANE	MAKE_IRR_ID('p', 'l', 'a', 'n')

class PlaneSceneNode : public irr::scene::IMeshSceneNode
{
private:
	irr::scene::IMesh* m_Mesh;
	irr::scene::IShadowVolumeSceneNode* m_Shadow;
	irr::core::dimension2df m_TileSize;
	irr::core::dimension2du m_TileCount;

public:
	PlaneSceneNode(const irr::core::dimension2df& tileSize,
		const irr::core::dimension2du& tileCount,
		irr::scene::ISceneNode* parent,
		irr::scene::ISceneManager* mgr, irr::s32 id,
		const irr::core::vector3df& position = irr::core::vector3df(0, 0, 0),
		const irr::core::vector3df& rotation = irr::core::vector3df(0, 0, 0),
		const irr::core::vector3df& scale = irr::core::vector3df(1.0f, 1.0f, 1.0f));
	virtual ~PlaneSceneNode(void);

	virtual void OnRegisterSceneNode(void);

	//! renders the node.
	virtual void render(void);

	//! returns the axis aligned bounding box of this node
	virtual const irr::core::aabbox3d<irr::f32>& getBoundingBox(void) const;

	//! returns the material based on the zero based index i. To get the amount
	//! of materials used by this scene node, use getMaterialCount().
	//! This function is needed for inserting the node into the scene hirachy on a
	//! optimal position for minimizing renderstate changes, but can also be used
	//! to directly modify the material of a scene node.
	virtual irr::video::SMaterial& getMaterial(irr::u32 i);

	//! returns amount of materials used by this scene node.
	virtual irr::u32 getMaterialCount(void) const;

	//! Returns type of the scene node
	virtual irr::scene::ESCENE_NODE_TYPE getType(void) const { return (irr::scene::ESCENE_NODE_TYPE)ESNT_PLANE; }

	//! Creates shadow volume scene node as child of this node
	//! and returns a pointer to it.
	virtual irr::scene::IShadowVolumeSceneNode* addShadowVolumeSceneNode(const irr::scene::IMesh* shadowMesh,
		irr::s32 id, bool zfailmethod = true, irr::f32 infinity = 10000.0f);

	//! Writes attributes of the scene node.
	virtual void serializeAttributes(irr::io::IAttributes* out, irr::io::SAttributeReadWriteOptions* options = 0) const;

	//! Reads attributes of the scene node.
	virtual void deserializeAttributes(irr::io::IAttributes* in, irr::io::SAttributeReadWriteOptions* options = 0);

	//! Creates a clone of this scene node and its children.
	virtual irr::scene::ISceneNode* clone(ISceneNode* newParent = 0, irr::scene::ISceneManager* newManager = 0);

	//! Sets a new mesh to display
	virtual void setMesh(irr::scene::IMesh* mesh) {}

	//! Returns the current mesh
	virtual irr::scene::IMesh* getMesh(void) { return m_Mesh; }

	//! Sets if the scene node should not copy the materials of the mesh but use them in a read only style.
	/* In this way it is possible to change the materials a mesh causing all mesh scene nodes
	referencing this mesh to change too. */
	virtual void setReadOnlyMaterials(bool readonly) {}

	//! Returns if the scene node should not copy the materials of the mesh but use them in a read only style
	virtual bool isReadOnlyMaterials(void) const { return false; }

	//! Removes a child from this scene node.
	//! Implemented here, to be able to remove the shadow properly, if there is one,
	//! or to remove attached childs.
	virtual bool removeChild(irr::scene::ISceneNode* child);
};
