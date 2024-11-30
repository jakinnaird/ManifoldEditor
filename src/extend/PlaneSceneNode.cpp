/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "PlaneSceneNode.hpp"
#include "../source/Irrlicht/CShadowVolumeSceneNode.h"

PlaneSceneNode::PlaneSceneNode(const irr::core::dimension2df& tileSize,
	const irr::core::dimension2du& tileCount,
	irr::scene::ISceneNode* parent,
	irr::scene::ISceneManager* mgr, irr::s32 id,
	const irr::core::vector3df& position,
	const irr::core::vector3df& rotation,
	const irr::core::vector3df& scale)
	: irr::scene::IMeshSceneNode(parent, mgr, id, position, rotation, scale),
	m_Mesh(nullptr), m_Shadow(nullptr), m_TileSize(tileSize), m_TileCount(tileCount)
{
#ifdef _DEBUG
	setDebugName("PlaneSceneNode");
#endif

	m_Mesh = mgr->getGeometryCreator()->createPlaneMesh(tileSize, tileCount);
}

PlaneSceneNode::~PlaneSceneNode(void)
{
	if (m_Shadow)
		m_Shadow->drop();
	if (m_Mesh)
		m_Mesh->drop();
}

void PlaneSceneNode::OnRegisterSceneNode(void)
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}

void PlaneSceneNode::render(void)
{
	irr::video::IVideoDriver* driver = SceneManager->getVideoDriver();

	if (m_Mesh && driver)
	{
		driver->setMaterial(m_Mesh->getMeshBuffer(0)->getMaterial());
		driver->setTransform(irr::video::ETS_WORLD, AbsoluteTransformation);
		if (m_Shadow)
			m_Shadow->updateShadowVolumes();

		driver->drawMeshBuffer(m_Mesh->getMeshBuffer(0));
		if (DebugDataVisible & irr::scene::EDS_BBOX)
		{
			irr::video::SMaterial m;
			m.Lighting = false;
			driver->setMaterial(m);
			driver->draw3DBox(m_Mesh->getMeshBuffer(0)->getBoundingBox(), irr::video::SColor(255, 255, 255, 255));
		}
	}
}

const irr::core::aabbox3d<irr::f32>& PlaneSceneNode::getBoundingBox(void) const
{
	return m_Mesh->getMeshBuffer(0)->getBoundingBox();
}

irr::video::SMaterial& PlaneSceneNode::getMaterial(irr::u32 i)
{
	return m_Mesh->getMeshBuffer(0)->getMaterial();
}

irr::u32 PlaneSceneNode::getMaterialCount(void) const
{
	return 1;
}

irr::scene::IShadowVolumeSceneNode* PlaneSceneNode::addShadowVolumeSceneNode(const irr::scene::IMesh* shadowMesh,
	irr::s32 id, bool zfailmethod, irr::f32 infinity)
{
	if (!SceneManager->getVideoDriver()->queryFeature(irr::video::EVDF_STENCIL_BUFFER))
		return 0;

	if (!shadowMesh)
		shadowMesh = m_Mesh; // if null is given, use the mesh of node

	if (m_Shadow)
		m_Shadow->drop();

	m_Shadow = addShadowVolumeSceneNode(shadowMesh, id, zfailmethod, infinity);
	return m_Shadow;
}

void PlaneSceneNode::serializeAttributes(irr::io::IAttributes* out, irr::io::SAttributeReadWriteOptions* options) const
{
	ISceneNode::serializeAttributes(out, options);

	out->addVector2d("TileSize", irr::core::vector2df(m_TileSize.Width, m_TileSize.Height));
	out->addDimension2d("TileCount", m_TileCount);
}

void PlaneSceneNode::deserializeAttributes(irr::io::IAttributes* in, irr::io::SAttributeReadWriteOptions* options)
{
	irr::core::dimension2df oldSize(m_TileSize);
	irr::core::dimension2du oldCount(m_TileCount);

	irr::core::vector2df size = in->getAttributeAsVector2d("TileSize");
	m_TileCount = in->getAttributeAsDimension2d("TileCount");

	m_TileSize.Width = irr::core::max_(size.X, 0.0001f);
	m_TileSize.Height = irr::core::max_(size.Y, 0.0001f);
	if (!irr::core::equals(m_TileSize.Width, oldSize.Width) || !irr::core::equals(m_TileSize.Height, oldSize.Height) ||
		oldCount != m_TileCount)
	{
		if (m_Mesh)
			m_Mesh->drop();

		m_Mesh = SceneManager->getGeometryCreator()->createPlaneMesh(m_TileSize, m_TileCount);
	}

	ISceneNode::deserializeAttributes(in, options);
}

irr::scene::ISceneNode* PlaneSceneNode::clone(ISceneNode* newParent, irr::scene::ISceneManager* newManager)
{
	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	PlaneSceneNode* nb = new PlaneSceneNode(m_TileSize, m_TileCount, newParent, newManager, ID, 
		RelativeTranslation, RelativeRotation, RelativeScale);

	nb->cloneMembers(this, newManager);
	nb->getMaterial(0) = getMaterial(0);
	nb->m_Shadow = m_Shadow;
	if (nb->m_Shadow)
		nb->m_Shadow->grab();

	if (newParent)
		nb->drop();

	return nb;
}

bool PlaneSceneNode::removeChild(irr::scene::ISceneNode* child)
{
	if (child && m_Shadow == child)
	{
		m_Shadow->drop();
		m_Shadow = 0;
	}

	return ISceneNode::removeChild(child);
}
