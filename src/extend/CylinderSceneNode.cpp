/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "CylinderSceneNode.hpp"
#include "../source/Irrlicht/CShadowVolumeSceneNode.h"

CylinderSceneNode::CylinderSceneNode(irr::f32 radius, irr::f32 length, irr::u32 tesselation,
	bool closeTop, irr::f32 oblique, irr::scene::ISceneNode* parent,
	irr::scene::ISceneManager* mgr, irr::s32 id,
	const irr::core::vector3df& position, const irr::core::vector3df& rotation,
	const irr::core::vector3df& scale)
	: irr::scene::IMeshSceneNode(parent, mgr, id, position, rotation, scale),
	m_Mesh(nullptr), m_Shadow(nullptr), m_Radius(radius), m_Length(length),
	m_Tesselation(tesselation), m_CloseTop(closeTop), m_Oblique(oblique)
{
#ifdef _DEBUG
	setDebugName("CylinderSceneNode");
#endif

	m_Mesh = mgr->getGeometryCreator()->createCylinderMesh(radius, length, tesselation,
		irr::video::SColor(0xffffffff), closeTop, oblique);
}

CylinderSceneNode::~CylinderSceneNode(void)
{
	if (m_Shadow)
		m_Shadow->drop();
	if (m_Mesh)
		m_Mesh->drop();
}

void CylinderSceneNode::OnRegisterSceneNode(void)
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}

void CylinderSceneNode::render(void)
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

const irr::core::aabbox3d<irr::f32>& CylinderSceneNode::getBoundingBox(void) const
{
	return m_Mesh->getMeshBuffer(0)->getBoundingBox();
}

irr::video::SMaterial& CylinderSceneNode::getMaterial(irr::u32 i)
{
	return m_Mesh->getMeshBuffer(0)->getMaterial();
}

irr::u32 CylinderSceneNode::getMaterialCount(void) const
{
	return 1;
}

irr::scene::IShadowVolumeSceneNode* CylinderSceneNode::addShadowVolumeSceneNode(const irr::scene::IMesh* shadowMesh,
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

void CylinderSceneNode::serializeAttributes(irr::io::IAttributes* out, irr::io::SAttributeReadWriteOptions* options) const
{
	ISceneNode::serializeAttributes(out, options);

	out->addFloat("Radius", m_Radius);
	out->addFloat("Length", m_Length);
	out->addInt("Tesselation", m_Tesselation);
	out->addBool("CloseTop", m_CloseTop);
	out->addFloat("Oblique", m_Oblique);
}

void CylinderSceneNode::deserializeAttributes(irr::io::IAttributes* in, irr::io::SAttributeReadWriteOptions* options)
{
	irr::f32 oldRadius = m_Radius;
	irr::f32 oldLength = m_Length;
	irr::u32 oldTesselation = m_Tesselation;
	bool oldCloseTop = m_CloseTop;
	irr::f32 oldOblique = m_Oblique;

	m_Radius = in->getAttributeAsFloat("Radius");
	m_Length = in->getAttributeAsFloat("Length");
	m_Tesselation = in->getAttributeAsInt("Tesselation");
	m_CloseTop = in->getAttributeAsBool("CloseTop");
	m_Oblique = in->getAttributeAsFloat("Oblique");

	m_Radius = irr::core::max_(m_Radius, 0.0001f);
	if (!irr::core::equals(m_Radius, oldRadius) || !irr::core::equals(m_Length, oldLength) || m_Tesselation != oldTesselation ||
		m_CloseTop != oldCloseTop || !irr::core::equals(m_Oblique, oldOblique))
	{
		if (m_Mesh)
			m_Mesh->drop();

		m_Mesh = SceneManager->getGeometryCreator()->createCylinderMesh(m_Radius, m_Length, m_Tesselation,
			irr::video::SColor(0xffffffff), m_CloseTop, m_Oblique);
	}

	ISceneNode::deserializeAttributes(in, options);
}

irr::scene::ISceneNode* CylinderSceneNode::clone(ISceneNode* newParent, irr::scene::ISceneManager* newManager)
{
	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	CylinderSceneNode* nb = new CylinderSceneNode(m_Radius, m_Length, m_Tesselation, m_CloseTop,
		m_Oblique, newParent, newManager, ID, RelativeTranslation, RelativeRotation, RelativeScale);

	nb->cloneMembers(this, newManager);
	nb->getMaterial(0) = getMaterial(0);
	nb->m_Shadow = m_Shadow;
	if (nb->m_Shadow)
		nb->m_Shadow->grab();

	if (newParent)
		nb->drop();

	return nb;
}

bool CylinderSceneNode::removeChild(irr::scene::ISceneNode* child)
{
	if (child && m_Shadow == child)
	{
		m_Shadow->drop();
		m_Shadow = 0;
	}

	return ISceneNode::removeChild(child);
}
