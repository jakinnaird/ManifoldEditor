/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "irrlicht.h"

/**
 * Helper class for managing heightmap data used by UpdatableTerrainSceneNode.
 * Provides efficient storage and manipulation of height values.
 */
class HeightmapData
{
private:
	irr::core::array<irr::f32> m_HeightData;
	irr::u32 m_Size;
	irr::f32 m_MinHeight;
	irr::f32 m_MaxHeight;
	bool m_IsModified;
	irr::core::aabbox3d<irr::s32> m_DirtyRegion;

public:
	HeightmapData();
	~HeightmapData();

	// Initialization
	bool create(irr::u32 size, irr::f32 defaultHeight = 0.0f);
	bool loadFromImage(irr::video::IImage* image);
	bool loadFromFile(const irr::io::path& filename, irr::video::IVideoDriver* driver);
	bool saveToFile(const irr::io::path& filename, irr::video::IVideoDriver* driver) const;
	void clear();

	// Data access
	irr::f32 getHeight(irr::u32 x, irr::u32 z) const;
	irr::f32 getHeightSafe(irr::s32 x, irr::s32 z) const; // Returns 0 if out of bounds
	irr::f32 getInterpolatedHeight(irr::f32 x, irr::f32 z) const; // Bilinear interpolation
	void setHeight(irr::u32 x, irr::u32 z, irr::f32 height);
	bool setHeightSafe(irr::s32 x, irr::s32 z, irr::f32 height); // Returns false if out of bounds

	// Bulk operations
	bool updateRegion(irr::u32 startX, irr::u32 startZ, irr::u32 width, irr::u32 height, const irr::f32* heightData);
	void getRegion(irr::u32 startX, irr::u32 startZ, irr::u32 width, irr::u32 height, irr::f32* heightData) const;

	// Information
	irr::u32 getSize() const { return m_Size; }
	irr::f32 getMinHeight() const { return m_MinHeight; }
	irr::f32 getMaxHeight() const { return m_MaxHeight; }
	const irr::f32* getData() const { return m_HeightData.const_pointer(); }
	irr::f32* getData() { return m_HeightData.pointer(); }
	bool isValid() const { return m_Size > 0 && m_HeightData.size() > 0; }

	// Modification tracking
	bool isModified() const { return m_IsModified; }
	void markClean() { m_IsModified = false; m_DirtyRegion.reset(irr::core::vector3di(0, 0, 0)); }
	void markDirty() { m_IsModified = true; }
	void markRegionDirty(irr::u32 x, irr::u32 z, irr::u32 width = 1, irr::u32 height = 1);
	const irr::core::aabbox3d<irr::s32>& getDirtyRegion() const { return m_DirtyRegion; }

	// Utility operations
	void smooth(irr::u32 iterations = 1);
	void smoothRegion(irr::u32 startX, irr::u32 startZ, irr::u32 width, irr::u32 height, irr::u32 iterations = 1);
	void normalizeHeights();
	void scaleHeights(irr::f32 scale);

private:
	void updateMinMaxHeight();
	irr::u32 getIndex(irr::u32 x, irr::u32 z) const { return z * m_Size + x; }
	bool isValidCoordinate(irr::u32 x, irr::u32 z) const { return x < m_Size && z < m_Size; }
};
