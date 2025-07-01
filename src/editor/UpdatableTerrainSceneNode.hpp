/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "irrlicht.h"
#include "HeightmapData.hpp"

/**
 * An updatable terrain scene node that allows dynamic heightmap modifications.
 * Extends Irrlicht's ITerrainSceneNode with real-time editing capabilities.
 */
class UpdatableTerrainSceneNode : public irr::scene::ITerrainSceneNode
{
private:
	// Terrain data
	HeightmapData m_HeightmapData;
	irr::scene::SMesh* m_Mesh;
	irr::scene::IDynamicMeshBuffer* m_RenderBuffer;
	irr::io::IFileSystem* m_FileSystem;
	
	// Terrain properties
	irr::u32 m_TerrainSize;
	irr::s32 m_MaxLOD;
	irr::scene::E_TERRAIN_PATCH_SIZE m_PatchSize;
	irr::video::SColor m_VertexColor;
	irr::s32 m_SmoothFactor;
	
	// Scale and position
	irr::core::vector3df m_TerrainScale;
	irr::core::vector3df m_TerrainPosition;
	
	// Patch management
	struct TerrainPatch
	{
		irr::core::aabbox3d<irr::f32> BoundingBox;
		irr::s32 CurrentLOD;
		bool IsDirty;
	};
	
	irr::core::array<TerrainPatch> m_Patches;
	irr::u32 m_PatchCount;
	irr::u32 m_CalcPatchSize;
	
	// Bounding box
	irr::core::aabbox3d<irr::f32> m_BoundingBox;
	
	// Camera tracking for LOD
	irr::core::vector3df m_OldCameraPosition;
	irr::f32 m_CameraMovementDelta;
	
	// Update flags
	bool m_ForceRecalculation;
	bool m_NeedsUpdate;

public:
	// Constructor
	UpdatableTerrainSceneNode(
		irr::scene::ISceneNode* parent, 
		irr::scene::ISceneManager* mgr,
		irr::io::IFileSystem* fs,
		irr::s32 id,
		irr::s32 maxLOD = 5,
		irr::scene::E_TERRAIN_PATCH_SIZE patchSize = irr::scene::ETPS_17,
		const irr::core::vector3df& position = irr::core::vector3df(0,0,0),
		const irr::core::vector3df& rotation = irr::core::vector3df(0,0,0),
		const irr::core::vector3df& scale = irr::core::vector3df(1,1,1)
	);
	
	virtual ~UpdatableTerrainSceneNode();

	// ISceneNode interface
	virtual void OnRegisterSceneNode();
	virtual void render();
	virtual const irr::core::aabbox3d<irr::f32>& getBoundingBox() const;
	virtual irr::u32 getMaterialCount() const;
	virtual irr::video::SMaterial& getMaterial(irr::u32 i);
	virtual irr::scene::ESCENE_NODE_TYPE getType() const { return /*(irr::scene::ESCENE_NODE_TYPE)ESNT_UPDATABLE_TERRAIN;*/ irr::scene::ESNT_TERRAIN; }
	virtual irr::scene::ISceneNode* clone(irr::scene::ISceneNode* newParent = 0, irr::scene::ISceneManager* newManager = 0);
	virtual void setPosition(const irr::core::vector3df& newpos);

	// Material overrides to work with render buffer
	virtual void setMaterialFlag(irr::video::E_MATERIAL_FLAG flag, bool newvalue);
	virtual void setMaterialTexture(irr::u32 textureLayer, irr::video::ITexture* texture);
	virtual void setMaterialType(irr::video::E_MATERIAL_TYPE newType);

	// ITerrainSceneNode interface
	virtual const irr::core::aabbox3d<irr::f32>& getBoundingBox(irr::s32 patchX, irr::s32 patchZ) const;
	virtual irr::u32 getIndexCount() const;
	virtual irr::scene::IMesh* getMesh();
	virtual irr::scene::IMeshBuffer* getRenderBuffer();
	virtual void getMeshBufferForLOD(irr::scene::IDynamicMeshBuffer& mb, irr::s32 LOD = 0) const;
	virtual irr::s32 getIndicesForPatch(irr::core::array<irr::u32>& indices, irr::s32 patchX, irr::s32 patchZ, irr::s32 LOD = 0);
	virtual irr::s32 getCurrentLODOfPatches(irr::core::array<irr::s32>& LODs) const;
	virtual void setLODOfPatch(irr::s32 patchX, irr::s32 patchZ, irr::s32 LOD = 0);
	virtual const irr::core::vector3df& getTerrainCenter() const;
	virtual irr::f32 getHeight(irr::f32 x, irr::f32 y) const;
	virtual void setCameraMovementDelta(irr::f32 delta);
	virtual void setCameraRotationDelta(irr::f32 delta);
	virtual void setDynamicSelectorUpdate(bool bVal);
	virtual bool overrideLODDistance(irr::s32 LOD, irr::f64 newDistance);
	virtual void scaleTexture(irr::f32 scale = 1.0f, irr::f32 scale2 = 0.0f);
	virtual bool loadHeightMap(irr::io::IReadFile* file, irr::video::SColor vertexColor = irr::video::SColor(255,255,255,255), irr::s32 smoothFactor = 0);
	virtual bool loadHeightMapRAW(irr::io::IReadFile* file, irr::s32 bitsPerPixel = 16, bool signedData = false, bool floatVals = false, irr::s32 width = 0, irr::video::SColor vertexColor = irr::video::SColor(255,255,255,255), irr::s32 smoothFactor = 0);

	// Serialization
	virtual void serializeAttributes(irr::io::IAttributes* out, irr::io::SAttributeReadWriteOptions* options = 0) const;
	virtual void deserializeAttributes(irr::io::IAttributes* in, irr::io::SAttributeReadWriteOptions* options = 0);

	// Extended heightmap management (new functionality)
	virtual bool createHeightmap(irr::u32 size, irr::f32 defaultHeight = 0.0f);
	virtual bool loadHeightmapFromFile(const irr::io::path& filename);
	virtual bool saveHeightmapToFile(const irr::io::path& filename) const;
	
	// Dynamic updates (new functionality)
	virtual bool updateHeight(irr::s32 x, irr::s32 z, irr::f32 newHeight);
	virtual bool updateRegion(irr::s32 x, irr::s32 z, irr::s32 width, irr::s32 height, const irr::f32* heightData);
	virtual void smoothTerrain(irr::u32 iterations = 1);
	virtual void smoothRegion(irr::s32 x, irr::s32 z, irr::s32 width, irr::s32 height, irr::u32 iterations = 1);
	
	// Information access (new functionality)
	virtual irr::u32 getHeightmapSize() const;
	virtual irr::f32 getMinHeight() const;
	virtual irr::f32 getMaxHeight() const;
	virtual const irr::f32* getHeightmapData() const;
	virtual bool isHeightmapModified() const;
	virtual void markHeightmapClean();
	
	// Terrain editing utilities
	virtual const irr::core::vector3df& getTerrainScale() const;
	virtual const irr::core::vector3df& getTerrainPosition() const;
	virtual void worldToHeightmap(irr::f32 worldX, irr::f32 worldZ, irr::s32& heightmapX, irr::s32& heightmapZ) const;
	virtual void heightmapToWorld(irr::s32 heightmapX, irr::s32 heightmapZ, irr::f32& worldX, irr::f32& worldZ) const;

private:
	// Internal terrain generation
	bool generateTerrain();
	void calculateNormals();
	void createPatches();
	void calculatePatchData();
	void updateMeshFromHeightmap();
	void updatePatchesFromHeightmap();
	
	// Patch management
	void markPatchDirty(irr::u32 patchX, irr::u32 patchZ);
	void markPatchesDirtyInRegion(const irr::core::aabbox3d<irr::s32>& region);
	void updateDirtyPatches();
	irr::u32 getPatchIndex(irr::u32 patchX, irr::u32 patchZ) const { return patchZ * m_PatchCount + patchX; }
	
	// Utility functions
	void updateBoundingBox();
	irr::u32 getVertexIndex(irr::u32 x, irr::u32 z) const { return z * m_TerrainSize + x; }
	
	// Initialization helpers
	void initializeDefaults();
	void cleanup();
};
