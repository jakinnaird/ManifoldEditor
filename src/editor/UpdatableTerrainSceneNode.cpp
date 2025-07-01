/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "UpdatableTerrainSceneNode.hpp"

#include <wx/config.h>
#include <wx/log.h>
#include <wx/string.h>

UpdatableTerrainSceneNode::UpdatableTerrainSceneNode(
	irr::scene::ISceneNode* parent, 
	irr::scene::ISceneManager* mgr,
	irr::io::IFileSystem* fs,
	irr::s32 id,
	irr::s32 maxLOD,
	irr::scene::E_TERRAIN_PATCH_SIZE patchSize,
	const irr::core::vector3df& position,
	const irr::core::vector3df& rotation,
	const irr::core::vector3df& scale)
	: ITerrainSceneNode(parent, mgr, id, position, rotation, scale)
	, m_Mesh(nullptr)
	, m_RenderBuffer(nullptr)
	, m_FileSystem(fs)
	, m_TerrainSize(0)
	, m_MaxLOD(maxLOD)
	, m_PatchSize(patchSize)
	, m_VertexColor(255, 255, 255, 255)
	, m_SmoothFactor(0)
	, m_TerrainScale(scale)
	, m_TerrainPosition(position)
	, m_PatchCount(0)
	, m_CalcPatchSize(0)
	, m_CameraMovementDelta(10.0f)
	, m_ForceRecalculation(true)
	, m_NeedsUpdate(false)
{
#ifdef _DEBUG
	setDebugName("UpdatableTerrainSceneNode");
#endif

	initializeDefaults();

	if (m_FileSystem)
		m_FileSystem->grab();

	// Create mesh
	m_Mesh = new irr::scene::SMesh();
	
	// Render buffer will be created in generateTerrain() with appropriate index type
	m_RenderBuffer = nullptr;

	setAutomaticCulling(irr::scene::EAC_OFF);
}

UpdatableTerrainSceneNode::~UpdatableTerrainSceneNode()
{
	cleanup();
}

void UpdatableTerrainSceneNode::initializeDefaults()
{
	m_BoundingBox.reset(irr::core::aabbox3df(0, 0, 0, 0, 0, 0));
	m_OldCameraPosition.set(0, 0, 0);
}

void UpdatableTerrainSceneNode::cleanup()
{
	if (m_FileSystem)
	{
		m_FileSystem->drop();
		m_FileSystem = nullptr;
	}

	if (m_Mesh)
	{
		m_Mesh->drop();
		m_Mesh = nullptr;
	}

	if (m_RenderBuffer)
	{
		m_RenderBuffer->drop();
		m_RenderBuffer = nullptr;
	}

	m_Patches.clear();
}

// ISceneNode interface implementation
void UpdatableTerrainSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
	{
		// Check if we need to update due to camera movement or heightmap changes
		if (m_NeedsUpdate || m_HeightmapData.isModified())
		{
			updateDirtyPatches();
		}

		SceneManager->registerNodeForRendering(this);
	}

	ISceneNode::OnRegisterSceneNode();
}

void UpdatableTerrainSceneNode::render()
{
	irr::video::IVideoDriver* driver = SceneManager->getVideoDriver();

	if (!driver || !m_RenderBuffer || !m_Mesh)
		return;

	driver->setTransform(irr::video::ETS_WORLD, AbsoluteTransformation);

	if (m_RenderBuffer->getIndexCount() > 0)
	{
		driver->setMaterial(m_RenderBuffer->getMaterial());
		driver->drawMeshBuffer(m_RenderBuffer);
	}

	// Debug bounding box
	if (DebugDataVisible & irr::scene::EDS_BBOX)
	{
		irr::video::SMaterial debugMaterial;
		debugMaterial.Lighting = false;
		driver->setMaterial(debugMaterial);
		driver->draw3DBox(m_BoundingBox, irr::video::SColor(255, 255, 255, 255));
	}
}

const irr::core::aabbox3d<irr::f32>& UpdatableTerrainSceneNode::getBoundingBox() const
{
	return m_BoundingBox;
}

irr::u32 UpdatableTerrainSceneNode::getMaterialCount() const
{
	return m_Mesh ? m_Mesh->getMeshBufferCount() : 0;
}

irr::video::SMaterial& UpdatableTerrainSceneNode::getMaterial(irr::u32 i)
{
	if (m_RenderBuffer && i == 0)
		return m_RenderBuffer->getMaterial();
	
	// Fallback - should not happen
	static irr::video::SMaterial fallback;
	return fallback;
}

irr::scene::ISceneNode* UpdatableTerrainSceneNode::clone(irr::scene::ISceneNode* newParent, irr::scene::ISceneManager* newManager)
{
	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	UpdatableTerrainSceneNode* newNode = new UpdatableTerrainSceneNode(
		newParent, newManager, m_FileSystem, ID, m_MaxLOD, m_PatchSize,
		getPosition(), getRotation(), getScale());

	// Copy heightmap data
	if (m_HeightmapData.isValid())
	{
		newNode->m_HeightmapData = m_HeightmapData;
		newNode->generateTerrain();
	}

	newNode->cloneMembers(this, newManager);

	if (newParent)
		newNode->drop();

	return newNode;
}

// Position override to keep terrain coordinate system synchronized
void UpdatableTerrainSceneNode::setPosition(const irr::core::vector3df& newpos)
{
	// Update scene node position
	ISceneNode::setPosition(newpos);
	
	// Keep internal terrain position synchronized for serialization
	m_TerrainPosition = newpos;
	
	// No need to update bounding box - it's now in local coordinates
	// and Irrlicht handles the transformation automatically
}

// Material overrides to work with render buffer
void UpdatableTerrainSceneNode::setMaterialFlag(irr::video::E_MATERIAL_FLAG flag, bool newvalue)
{
	if (m_RenderBuffer)
	{
		m_RenderBuffer->getMaterial().setFlag(flag, newvalue);
	}
}

void UpdatableTerrainSceneNode::setMaterialTexture(irr::u32 textureLayer, irr::video::ITexture* texture)
{
	if (m_RenderBuffer && textureLayer < irr::video::MATERIAL_MAX_TEXTURES)
	{
		m_RenderBuffer->getMaterial().setTexture(textureLayer, texture);
	}
}

void UpdatableTerrainSceneNode::setMaterialType(irr::video::E_MATERIAL_TYPE newType)
{
	if (m_RenderBuffer)
	{
		m_RenderBuffer->getMaterial().MaterialType = newType;
	}
}

// ITerrainSceneNode interface implementation
const irr::core::aabbox3d<irr::f32>& UpdatableTerrainSceneNode::getBoundingBox(irr::s32 patchX, irr::s32 patchZ) const
{
	if (patchX >= 0 && patchZ >= 0 && (irr::u32)patchX < m_PatchCount && (irr::u32)patchZ < m_PatchCount)
	{
		irr::u32 index = getPatchIndex((irr::u32)patchX, (irr::u32)patchZ);
		if (index < m_Patches.size())
			return m_Patches[index].BoundingBox;
	}
	
	return m_BoundingBox;
}

irr::u32 UpdatableTerrainSceneNode::getIndexCount() const
{
	return m_RenderBuffer ? m_RenderBuffer->getIndexCount() : 0;
}

irr::scene::IMesh* UpdatableTerrainSceneNode::getMesh()
{
	return m_Mesh;
}

irr::scene::IMeshBuffer* UpdatableTerrainSceneNode::getRenderBuffer()
{
	return m_RenderBuffer;
}

void UpdatableTerrainSceneNode::getMeshBufferForLOD(irr::scene::IDynamicMeshBuffer& mb, irr::s32 LOD) const
{
	// Simple implementation - could be enhanced for different LODs
	if (m_RenderBuffer)
	{
		mb.getVertexBuffer().set_used(m_RenderBuffer->getVertexCount());
		mb.getIndexBuffer().set_used(m_RenderBuffer->getIndexCount());

		for (irr::u32 i = 0; i < m_RenderBuffer->getVertexCount(); ++i)
		{
			mb.getVertexBuffer()[i] = m_RenderBuffer->getVertexBuffer()[i];
		}

		for (irr::u32 i = 0; i < m_RenderBuffer->getIndexCount(); ++i)
		{
			mb.getIndexBuffer().setValue(i, m_RenderBuffer->getIndices()[i]);
		}
	}
}

irr::s32 UpdatableTerrainSceneNode::getIndicesForPatch(irr::core::array<irr::u32>& indices, irr::s32 patchX, irr::s32 patchZ, irr::s32 LOD)
{
	// Simple implementation - could be enhanced for patch-specific indices
	indices.clear();
	if (m_RenderBuffer)
	{
		for (irr::u32 i = 0; i < m_RenderBuffer->getIndexCount(); ++i)
		{
			indices.push_back(m_RenderBuffer->getIndices()[i]);
		}
	}
	return indices.size();
}

irr::s32 UpdatableTerrainSceneNode::getCurrentLODOfPatches(irr::core::array<irr::s32>& LODs) const
{
	LODs.clear();
	for (irr::u32 i = 0; i < m_Patches.size(); ++i)
	{
		LODs.push_back(m_Patches[i].CurrentLOD);
	}
	return LODs.size();
}

void UpdatableTerrainSceneNode::setLODOfPatch(irr::s32 patchX, irr::s32 patchZ, irr::s32 LOD)
{
	if (patchX >= 0 && patchZ >= 0 && (irr::u32)patchX < m_PatchCount && (irr::u32)patchZ < m_PatchCount)
	{
		irr::u32 index = getPatchIndex((irr::u32)patchX, (irr::u32)patchZ);
		if (index < m_Patches.size())
		{
			m_Patches[index].CurrentLOD = LOD;
			m_Patches[index].IsDirty = true;
			m_NeedsUpdate = true;
		}
	}
}

const irr::core::vector3df& UpdatableTerrainSceneNode::getTerrainCenter() const
{
	// Return the actual scene node position, not the internal terrain position
	// This ensures consistency with the coordinate system
	static irr::core::vector3df center;
	center = getPosition();
	center.X += ((irr::f32)(m_TerrainSize - 1) * m_TerrainScale.X) * 0.5f;
	center.Z += ((irr::f32)(m_TerrainSize - 1) * m_TerrainScale.Z) * 0.5f;
	return center;
}

irr::f32 UpdatableTerrainSceneNode::getHeight(irr::f32 x, irr::f32 y) const
{
	// Convert world coordinates to heightmap coordinates
	irr::s32 hx, hz;
	worldToHeightmap(x, y, hx, hz);
	
	// Get interpolated height from heightmap
	irr::f32 height = m_HeightmapData.getInterpolatedHeight((irr::f32)hx, (irr::f32)hz);
	
	// Apply terrain scaling and position offset
	irr::core::vector3df nodePos = getPosition();
	return height * m_TerrainScale.Y + nodePos.Y;
}

void UpdatableTerrainSceneNode::setCameraMovementDelta(irr::f32 delta)
{
	m_CameraMovementDelta = delta;
}

void UpdatableTerrainSceneNode::setCameraRotationDelta(irr::f32 delta)
{
	// Not implemented in this basic version
}

void UpdatableTerrainSceneNode::setDynamicSelectorUpdate(bool bVal)
{
	// Not implemented in this basic version
}

bool UpdatableTerrainSceneNode::overrideLODDistance(irr::s32 LOD, irr::f64 newDistance)
{
	// Not implemented in this basic version
	return false;
}

void UpdatableTerrainSceneNode::scaleTexture(irr::f32 scale, irr::f32 scale2)
{
	if (!m_RenderBuffer)
		return;

	// Update texture coordinates for all vertices
	for (irr::u32 i = 0; i < m_RenderBuffer->getVertexCount(); ++i)
	{
		irr::video::S3DVertex2TCoords& vertex = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[i];
		vertex.TCoords *= scale;
		if (scale2 != 0.0f)
			vertex.TCoords2 *= scale2;
	}

	m_RenderBuffer->setDirty();
}

bool UpdatableTerrainSceneNode::loadHeightMap(irr::io::IReadFile* file, irr::video::SColor vertexColor, irr::s32 smoothFactor)
{
	if (!file)
		return false;

	irr::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (!driver)
		return false;

	irr::video::IImage* image = driver->createImageFromFile(file);
	if (!image)
		return false;

	m_VertexColor = vertexColor;
	m_SmoothFactor = smoothFactor;

	bool result = m_HeightmapData.loadFromImage(image);
	image->drop();

	if (result)
	{
		if (smoothFactor > 0)
			m_HeightmapData.smooth(smoothFactor);

		m_TerrainSize = m_HeightmapData.getSize();
		result = generateTerrain();
	}

	return result;
}

bool UpdatableTerrainSceneNode::loadHeightMapRAW(irr::io::IReadFile* file, irr::s32 bitsPerPixel, bool signedData, bool floatVals, irr::s32 width, irr::video::SColor vertexColor, irr::s32 smoothFactor)
{
	// Basic implementation - could be enhanced to support RAW files
	return loadHeightMap(file, vertexColor, smoothFactor);
}

// Serialization
void UpdatableTerrainSceneNode::serializeAttributes(irr::io::IAttributes* out, irr::io::SAttributeReadWriteOptions* options) const
{
	ISceneNode::serializeAttributes(out, options);

	// Basic terrain properties
	out->addInt("TerrainSize", m_TerrainSize);
	out->addInt("MaxLOD", m_MaxLOD);
	out->addInt("PatchSize", (irr::s32)m_PatchSize);
	out->addColor("VertexColor", m_VertexColor);
	out->addInt("SmoothFactor", m_SmoothFactor);
	out->addVector3d("TerrainScale", m_TerrainScale);
	out->addVector3d("TerrainPosition", m_TerrainPosition);
	out->addFloat("TextureScale1", 1.0f);
	out->addFloat("TextureScale2", 1.0f);
	
	// Heightmap data properties
	out->addFloat("MinHeight", m_HeightmapData.getMinHeight());
	out->addFloat("MaxHeight", m_HeightmapData.getMaxHeight());
	out->addBool("IsHeightmapModified", m_HeightmapData.isModified());
	
	// Add heightmap file reference for external storage
	if (m_HeightmapData.isValid())
	{
		wxString basePath(wxConfig::Get()->Read("Paths/TexturePath", ""));
		wxString terrainId = wxString::Format("terrain_%d_%dx%d.bmp", 
			getID(), m_TerrainSize, m_TerrainSize);
		if (!basePath.IsEmpty())
			terrainId = basePath + "/" + terrainId;

		out->addString("Heightmap", terrainId.ToStdString().c_str());

		if (options) // we are writing to disk
			saveHeightmapToFile(terrainId.ToStdString().c_str());
	}
	else
		out->addString("Heightmap", "");
}

void UpdatableTerrainSceneNode::deserializeAttributes(irr::io::IAttributes* in, irr::io::SAttributeReadWriteOptions* options)
{
	// Read basic terrain properties
	m_TerrainSize = in->getAttributeAsInt("TerrainSize");
	m_MaxLOD = in->getAttributeAsInt("MaxLOD");
	m_PatchSize = (irr::scene::E_TERRAIN_PATCH_SIZE)in->getAttributeAsInt("PatchSize");
	m_VertexColor = in->getAttributeAsColor("VertexColor");
	m_SmoothFactor = in->getAttributeAsInt("SmoothFactor");
	m_TerrainScale = in->getAttributeAsVector3d("TerrainScale");
	m_TerrainPosition = in->getAttributeAsVector3d("TerrainPosition");
	
	// Read heightmap data properties
	irr::f32 minHeight = in->getAttributeAsFloat("MinHeight");
	irr::f32 maxHeight = in->getAttributeAsFloat("MaxHeight");
	bool isModified = in->getAttributeAsBool("IsHeightmapModified");
	
	// Read heightmap file reference
	irr::core::stringc heightmapFile = in->getAttributeAsString("Heightmap");

	// Call parent deserialization
	ISceneNode::deserializeAttributes(in, options);

	// Regenerate terrain if we have valid size
	if (m_TerrainSize > 0)
	{
		// Create empty heightmap first
		if (!m_HeightmapData.isValid())
		{
			m_HeightmapData.create(m_TerrainSize, 0.0f);
		}
		
		// Try to load heightmap file if specified
		if (heightmapFile.size() > 0)
		{
			if (!loadHeightmapFromFile(heightmapFile.c_str()))
				wxLogWarning("Failed to load heightmap file: %s", heightmapFile.c_str());
		}
		else	
			generateTerrain(); // generate some base terrain
	}
}

// Extended heightmap management (new functionality)
bool UpdatableTerrainSceneNode::createHeightmap(irr::u32 size, irr::f32 defaultHeight)
{
	bool result = m_HeightmapData.create(size, defaultHeight);
	if (result)
	{
		m_TerrainSize = size;
		result = generateTerrain();
	}
	return result;
}

bool UpdatableTerrainSceneNode::loadHeightmapFromFile(const irr::io::path& filename)
{
	irr::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (!driver)
		return false;

	bool result = m_HeightmapData.loadFromFile(filename, driver);
	if (result)
	{
		m_TerrainSize = m_HeightmapData.getSize();
		result = generateTerrain();
	}
	return result;
}

bool UpdatableTerrainSceneNode::saveHeightmapToFile(const irr::io::path& filename) const
{
	irr::video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (!driver)
		return false;

	return m_HeightmapData.saveToFile(filename, driver);
}

// Dynamic updates (new functionality)
bool UpdatableTerrainSceneNode::updateHeight(irr::s32 x, irr::s32 z, irr::f32 newHeight)
{
	if (!m_HeightmapData.setHeightSafe(x, z, newHeight))
		return false;

	// Mark affected patches as dirty
	irr::core::aabbox3d<irr::s32> region(x, 0, z, x, 0, z);
	markPatchesDirtyInRegion(region);
	m_NeedsUpdate = true;

	return true;
}

bool UpdatableTerrainSceneNode::updateRegion(irr::s32 x, irr::s32 z, irr::s32 width, irr::s32 height, const irr::f32* heightData)
{
	if (!m_HeightmapData.updateRegion((irr::u32)x, (irr::u32)z, (irr::u32)width, (irr::u32)height, heightData))
		return false;

	// Mark affected patches as dirty
	irr::core::aabbox3d<irr::s32> region(x, 0, z, x + width - 1, 0, z + height - 1);
	markPatchesDirtyInRegion(region);
	m_NeedsUpdate = true;

	return true;
}

void UpdatableTerrainSceneNode::smoothTerrain(irr::u32 iterations)
{
	m_HeightmapData.smooth(iterations);
	m_ForceRecalculation = true;
	m_NeedsUpdate = true;
}

void UpdatableTerrainSceneNode::smoothRegion(irr::s32 x, irr::s32 z, irr::s32 width, irr::s32 height, irr::u32 iterations)
{
	m_HeightmapData.smoothRegion((irr::u32)x, (irr::u32)z, (irr::u32)width, (irr::u32)height, iterations);
	
	// Mark affected patches as dirty
	irr::core::aabbox3d<irr::s32> region(x, 0, z, x + width - 1, 0, z + height - 1);
	markPatchesDirtyInRegion(region);
	m_NeedsUpdate = true;
}

// Information access (new functionality)
irr::u32 UpdatableTerrainSceneNode::getHeightmapSize() const
{
	return m_HeightmapData.getSize();
}

irr::f32 UpdatableTerrainSceneNode::getMinHeight() const
{
	return m_HeightmapData.getMinHeight();
}

irr::f32 UpdatableTerrainSceneNode::getMaxHeight() const
{
	return m_HeightmapData.getMaxHeight();
}

const irr::f32* UpdatableTerrainSceneNode::getHeightmapData() const
{
	return m_HeightmapData.getData();
}

bool UpdatableTerrainSceneNode::isHeightmapModified() const
{
	return m_HeightmapData.isModified();
}

void UpdatableTerrainSceneNode::markHeightmapClean()
{
	m_HeightmapData.markClean();
}

// Terrain editing utilities
const irr::core::vector3df& UpdatableTerrainSceneNode::getTerrainScale() const
{
	return m_TerrainScale;
}

const irr::core::vector3df& UpdatableTerrainSceneNode::getTerrainPosition() const
{
	return m_TerrainPosition;
}

// Private implementation
bool UpdatableTerrainSceneNode::generateTerrain()
{
	if (!m_HeightmapData.isValid())
		return false;

	// Calculate vertex count to determine index buffer type
	irr::u32 vertexCount = m_TerrainSize * m_TerrainSize;
	
	// Create or recreate render buffer with appropriate index type
	if (m_RenderBuffer)
	{
		m_RenderBuffer->drop();
		m_RenderBuffer = nullptr;
	}
	
	if (vertexCount <= 65536)
	{
		// Small enough for 16-bit indices
		m_RenderBuffer = new irr::scene::CDynamicMeshBuffer(irr::video::EVT_2TCOORDS, irr::video::EIT_16BIT);
	}
	else
	{
		// Need 32-bit indices
		m_RenderBuffer = new irr::scene::CDynamicMeshBuffer(irr::video::EVT_2TCOORDS, irr::video::EIT_32BIT);
	}
	
	m_RenderBuffer->setHardwareMappingHint(irr::scene::EHM_STATIC, irr::scene::EBT_VERTEX);
	m_RenderBuffer->setHardwareMappingHint(irr::scene::EHM_DYNAMIC, irr::scene::EBT_INDEX);

	// Add render buffer to mesh
	if (m_Mesh)
	{
		// Clear existing mesh buffers
		for (irr::u32 i = 0; i < m_Mesh->getMeshBufferCount(); ++i)
		{
			m_Mesh->getMeshBuffer(i)->drop();
		}
		m_Mesh->MeshBuffers.clear();
		
		// Add our render buffer to the mesh
		m_RenderBuffer->grab(); // Mesh will hold a reference
		m_Mesh->addMeshBuffer(m_RenderBuffer);
		m_Mesh->recalculateBoundingBox();
	}

	// Calculate patch information
	m_CalcPatchSize = (irr::u32)m_PatchSize;
	m_PatchCount = (m_TerrainSize - 1) / (m_CalcPatchSize - 1);

	// Create patches
	createPatches();

	// Generate mesh from heightmap
	updateMeshFromHeightmap();

	// Calculate normals
	calculateNormals();

	// Update bounding box
	updateBoundingBox();

	m_ForceRecalculation = false;
	m_NeedsUpdate = false;

	return true;
}

void UpdatableTerrainSceneNode::calculateNormals()
{
	if (!m_RenderBuffer || m_RenderBuffer->getVertexCount() == 0)
		return;

	// Reset all normals
	for (irr::u32 i = 0; i < m_RenderBuffer->getVertexCount(); ++i)
	{
		irr::video::S3DVertex2TCoords& vertex = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[i];
		vertex.Normal.set(0, 0, 0);
	}

	// Calculate face normals and accumulate to vertices
	// Handle both 16-bit and 32-bit indices
	if (m_RenderBuffer->getIndexBuffer().getType() == irr::video::EIT_16BIT)
	{
		irr::u16* indices = (irr::u16*)m_RenderBuffer->getIndices();
		for (irr::u32 i = 0; i < m_RenderBuffer->getIndexCount(); i += 3)
		{
			irr::video::S3DVertex2TCoords& v1 = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[indices[i]];
			irr::video::S3DVertex2TCoords& v2 = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[indices[i + 1]];
			irr::video::S3DVertex2TCoords& v3 = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[indices[i + 2]];

			irr::core::vector3df normal = (v2.Pos - v1.Pos).crossProduct(v3.Pos - v1.Pos);
			normal.normalize();

			v1.Normal += normal;
			v2.Normal += normal;
			v3.Normal += normal;
		}
	}
	else // 32-bit indices
	{
		irr::u32* indices = (irr::u32*)m_RenderBuffer->getIndices();
		for (irr::u32 i = 0; i < m_RenderBuffer->getIndexCount(); i += 3)
		{
			irr::video::S3DVertex2TCoords& v1 = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[indices[i]];
			irr::video::S3DVertex2TCoords& v2 = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[indices[i + 1]];
			irr::video::S3DVertex2TCoords& v3 = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[indices[i + 2]];

			irr::core::vector3df normal = (v2.Pos - v1.Pos).crossProduct(v3.Pos - v1.Pos);
			normal.normalize();

			v1.Normal += normal;
			v2.Normal += normal;
			v3.Normal += normal;
		}
	}

	// Normalize all vertex normals
	for (irr::u32 i = 0; i < m_RenderBuffer->getVertexCount(); ++i)
	{
		irr::video::S3DVertex2TCoords& vertex = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[i];
		vertex.Normal.normalize();
	}

	m_RenderBuffer->setDirty();
}

void UpdatableTerrainSceneNode::createPatches()
{
	m_Patches.clear();
	m_Patches.reallocate(m_PatchCount * m_PatchCount);

	for (irr::u32 z = 0; z < m_PatchCount; ++z)
	{
		for (irr::u32 x = 0; x < m_PatchCount; ++x)
		{
			TerrainPatch patch;
			patch.CurrentLOD = 0;
			patch.IsDirty = true;
			patch.BoundingBox.reset(irr::core::aabbox3df(0, 0, 0, 0, 0, 0));
			m_Patches.push_back(patch);
		}
	}

	calculatePatchData();
}

void UpdatableTerrainSceneNode::calculatePatchData()
{
	// Calculate bounding boxes for each patch
	for (irr::u32 z = 0; z < m_PatchCount; ++z)
	{
		for (irr::u32 x = 0; x < m_PatchCount; ++x)
		{
			irr::u32 patchIndex = getPatchIndex(x, z);
			if (patchIndex >= m_Patches.size())
				continue;

			// Calculate patch bounds in heightmap coordinates
			irr::u32 startX = x * (m_CalcPatchSize - 1);
			irr::u32 startZ = z * (m_CalcPatchSize - 1);
			irr::u32 endX = irr::core::min_(startX + m_CalcPatchSize - 1, m_TerrainSize - 1);
			irr::u32 endZ = irr::core::min_(startZ + m_CalcPatchSize - 1, m_TerrainSize - 1);

			// Find min/max heights in this patch
			irr::f32 minHeight = m_HeightmapData.getHeight(startX, startZ);
			irr::f32 maxHeight = minHeight;

			for (irr::u32 pz = startZ; pz <= endZ; ++pz)
			{
				for (irr::u32 px = startX; px <= endX; ++px)
				{
					irr::f32 height = m_HeightmapData.getHeight(px, pz);
					if (height < minHeight) minHeight = height;
					if (height > maxHeight) maxHeight = height;
				}
			}

			// Set patch bounding box in local coordinates
			m_Patches[patchIndex].BoundingBox.reset(irr::core::aabbox3df(
				(irr::f32)startX * m_TerrainScale.X,
				minHeight * m_TerrainScale.Y,
				(irr::f32)startZ * m_TerrainScale.Z,
				(irr::f32)endX * m_TerrainScale.X,
				maxHeight * m_TerrainScale.Y,
				(irr::f32)endZ * m_TerrainScale.Z
			));
		}
	}
}

void UpdatableTerrainSceneNode::updateMeshFromHeightmap()
{
	if (!m_RenderBuffer || !m_HeightmapData.isValid())
		return;

	// Calculate vertex count
	irr::u32 vertexCount = m_TerrainSize * m_TerrainSize;
	m_RenderBuffer->getVertexBuffer().set_used(vertexCount);

	// Generate vertices
	irr::f32 stepSize = 1.0f / (irr::f32)(m_TerrainSize - 1);
	for (irr::u32 z = 0; z < m_TerrainSize; ++z)
	{
		for (irr::u32 x = 0; x < m_TerrainSize; ++x)
		{
			irr::u32 index = getVertexIndex(x, z);
			irr::video::S3DVertex2TCoords& vertex = (irr::video::S3DVertex2TCoords&)m_RenderBuffer->getVertexBuffer()[index];

			// Position in local coordinates (relative to terrain position)
			vertex.Pos.X = (irr::f32)x * m_TerrainScale.X;
			vertex.Pos.Y = m_HeightmapData.getHeight(x, z) * m_TerrainScale.Y;
			vertex.Pos.Z = (irr::f32)z * m_TerrainScale.Z;

			// Texture coordinates
			vertex.TCoords.X = vertex.TCoords2.X = 1.0f - (irr::f32)x * stepSize;
			vertex.TCoords.Y = vertex.TCoords2.Y = (irr::f32)z * stepSize;

			// Color
			vertex.Color = m_VertexColor;

			// Normal (will be calculated later)
			vertex.Normal.set(0, 1, 0);
		}
	}

	// Generate indices
	irr::u32 indexCount = (m_TerrainSize - 1) * (m_TerrainSize - 1) * 6;
	m_RenderBuffer->getIndexBuffer().set_used(indexCount);

	irr::u32 currentIndex = 0;
	for (irr::u32 z = 0; z < m_TerrainSize - 1; ++z)
	{
		for (irr::u32 x = 0; x < m_TerrainSize - 1; ++x)
		{
			irr::u32 i1 = getVertexIndex(x, z);
			irr::u32 i2 = getVertexIndex(x + 1, z);
			irr::u32 i3 = getVertexIndex(x, z + 1);
			irr::u32 i4 = getVertexIndex(x + 1, z + 1);

			// First triangle
			m_RenderBuffer->getIndexBuffer().setValue(currentIndex++, i1);
			m_RenderBuffer->getIndexBuffer().setValue(currentIndex++, i3);
			m_RenderBuffer->getIndexBuffer().setValue(currentIndex++, i2);

			// Second triangle
			m_RenderBuffer->getIndexBuffer().setValue(currentIndex++, i2);
			m_RenderBuffer->getIndexBuffer().setValue(currentIndex++, i3);
			m_RenderBuffer->getIndexBuffer().setValue(currentIndex++, i4);
		}
	}

	m_RenderBuffer->setDirty();
}

void UpdatableTerrainSceneNode::updatePatchesFromHeightmap()
{
	// Mark all patches as needing recalculation
	for (irr::u32 i = 0; i < m_Patches.size(); ++i)
	{
		m_Patches[i].IsDirty = true;
	}
	calculatePatchData();
}

void UpdatableTerrainSceneNode::markPatchDirty(irr::u32 patchX, irr::u32 patchZ)
{
	if (patchX < m_PatchCount && patchZ < m_PatchCount)
	{
		irr::u32 index = getPatchIndex(patchX, patchZ);
		if (index < m_Patches.size())
		{
			m_Patches[index].IsDirty = true;
		}
	}
}

void UpdatableTerrainSceneNode::markPatchesDirtyInRegion(const irr::core::aabbox3d<irr::s32>& region)
{
	if (m_PatchCount == 0 || m_CalcPatchSize == 0)
		return;

	// Convert heightmap coordinates to patch coordinates
	irr::u32 startPatchX = (irr::u32)irr::core::max_(0, region.MinEdge.X) / (m_CalcPatchSize - 1);
	irr::u32 startPatchZ = (irr::u32)irr::core::max_(0, region.MinEdge.Z) / (m_CalcPatchSize - 1);
	irr::u32 endPatchX = irr::core::min_((irr::u32)region.MaxEdge.X / (m_CalcPatchSize - 1), m_PatchCount - 1);
	irr::u32 endPatchZ = irr::core::min_((irr::u32)region.MaxEdge.Z / (m_CalcPatchSize - 1), m_PatchCount - 1);

	// Mark patches as dirty
	for (irr::u32 z = startPatchZ; z <= endPatchZ; ++z)
	{
		for (irr::u32 x = startPatchX; x <= endPatchX; ++x)
		{
			markPatchDirty(x, z);
		}
	}
}

void UpdatableTerrainSceneNode::updateDirtyPatches()
{
	if (m_ForceRecalculation)
	{
		generateTerrain();
		return;
	}

	bool anyPatchDirty = false;
	for (irr::u32 i = 0; i < m_Patches.size(); ++i)
	{
		if (m_Patches[i].IsDirty)
		{
			anyPatchDirty = true;
			m_Patches[i].IsDirty = false;
		}
	}

	if (anyPatchDirty || m_HeightmapData.isModified())
	{
		updateMeshFromHeightmap();
		calculateNormals();
		updateBoundingBox();
		calculatePatchData();
		m_HeightmapData.markClean();
		m_NeedsUpdate = false;
	}
}

void UpdatableTerrainSceneNode::updateBoundingBox()
{
	if (!m_HeightmapData.isValid())
	{
		m_BoundingBox.reset(irr::core::aabbox3df(0, 0, 0, 0, 0, 0));
		return;
	}

	// Calculate bounding box in local coordinates (relative to terrain position)
	// Irrlicht will transform this to world space using the node's transformation matrix
	irr::f32 minHeight = m_HeightmapData.getMinHeight() * m_TerrainScale.Y;
	irr::f32 maxHeight = m_HeightmapData.getMaxHeight() * m_TerrainScale.Y;

	m_BoundingBox.reset(irr::core::aabbox3df(
		0.0f, // Local X starts at 0
		minHeight,
		0.0f, // Local Z starts at 0
		(irr::f32)(m_TerrainSize - 1) * m_TerrainScale.X,
		maxHeight,
		(irr::f32)(m_TerrainSize - 1) * m_TerrainScale.Z
	));

	// Update mesh bounding box as well
	if (m_Mesh)
	{
		m_Mesh->setBoundingBox(m_BoundingBox);
	}
}

void UpdatableTerrainSceneNode::worldToHeightmap(irr::f32 worldX, irr::f32 worldZ, irr::s32& heightmapX, irr::s32& heightmapZ) const
{
	// Convert world coordinates to heightmap coordinates
	// Account for terrain position (scene node position)
	irr::core::vector3df nodePos = getPosition();
	heightmapX = (irr::s32)((worldX - nodePos.X) / m_TerrainScale.X);
	heightmapZ = (irr::s32)((worldZ - nodePos.Z) / m_TerrainScale.Z);
}

void UpdatableTerrainSceneNode::heightmapToWorld(irr::s32 heightmapX, irr::s32 heightmapZ, irr::f32& worldX, irr::f32& worldZ) const
{
	// Convert heightmap coordinates to world coordinates
	// Account for terrain position (scene node position)
	irr::core::vector3df nodePos = getPosition();
	worldX = (irr::f32)heightmapX * m_TerrainScale.X + nodePos.X;
	worldZ = (irr::f32)heightmapZ * m_TerrainScale.Z + nodePos.Z;
}

 