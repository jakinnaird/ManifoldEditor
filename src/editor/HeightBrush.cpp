/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "HeightBrush.hpp"
#include "UpdatableTerrainSceneNode.hpp"

HeightBrush::HeightBrush(HeightMode mode)
	: TerrainBrush(mode == HEIGHT_RAISE ? BRUSH_RAISE : BRUSH_LOWER)
	, m_HeightMode(mode)
	, m_TargetHeight(0.0f)
	, m_MaxDelta(2.0f) // Reduced from 10.0f for more gradual height changes
	, m_AdaptiveStrength(false)
{
	// Update brush type based on height mode
	switch (m_HeightMode)
	{
		case HEIGHT_FLATTEN:
		case HEIGHT_SET:
			m_Type = BRUSH_FLATTEN;
			break;
		case HEIGHT_LOWER:
			m_Type = BRUSH_LOWER;
			break;
		case HEIGHT_RAISE:
		default:
			m_Type = BRUSH_RAISE;
			break;
	}
}

HeightBrush::~HeightBrush()
{
}

bool HeightBrush::apply(UpdatableTerrainSceneNode* terrain, irr::f32 deltaTime)
{
	// update the current time
	setCurrentTime(deltaTime);

	if (!terrain || !m_IsActive || !isTimeToApply())
		return false;

	// Convert world position to heightmap coordinates
	irr::s32 centerX, centerZ;
	//irr::f32 worldX, worldZ;
	terrain->worldToHeightmap(m_Position.X, m_Position.Z, centerX, centerZ);
	
	// Validate coordinates
	if (centerX < 0 || centerZ < 0 || 
		(irr::u32)centerX >= terrain->getHeightmapSize() || 
		(irr::u32)centerZ >= terrain->getHeightmapSize())
		return false;

	// Apply height modification
	applyHeightToRegion(terrain, centerX, centerZ, deltaTime);
	
	updateApplyTime();
	return true;
}

void HeightBrush::renderPreview(irr::video::IVideoDriver* driver, const irr::core::matrix4& viewMatrix)
{
	// Call base class preview first
	TerrainBrush::renderPreview(driver, viewMatrix);
	
	if (!m_IsVisible || !driver)
		return;

	// Add height-specific visualization
	if (m_HeightMode == HEIGHT_FLATTEN || m_HeightMode == HEIGHT_SET)
	{
		// Draw a horizontal line indicating target height
		irr::video::SMaterial lineMaterial;
		lineMaterial.Lighting = false;
		lineMaterial.ZBuffer = irr::video::ECFN_LESSEQUAL;
		lineMaterial.ZWriteEnable = false;
		
		driver->setMaterial(lineMaterial);
		
		// Draw cross-hairs at target height
		irr::core::vector3df center(m_Position.X, m_TargetHeight, m_Position.Z);
		irr::core::vector3df lineSize(m_Size * 0.5f, 0, 0);
		irr::core::vector3df lineSize2(0, 0, m_Size * 0.5f);
		
		irr::video::SColor targetColor(128, 255, 255, 0); // Yellow
		driver->draw3DLine(center - lineSize, center + lineSize, targetColor);
		driver->draw3DLine(center - lineSize2, center + lineSize2, targetColor);
	}
}

void HeightBrush::applyHeightToRegion(UpdatableTerrainSceneNode* terrain, 
	irr::s32 centerX, irr::s32 centerZ, irr::f32 deltaTime)
{
	irr::u32 terrainSize = terrain->getHeightmapSize();
	
	// Calculate affected region in heightmap coordinates
	// Use a more precise radius calculation
	irr::f32 radiusFloat = m_Size / terrain->getTerrainScale().X;
	irr::s32 radius = (irr::s32)ceilf(radiusFloat) + 1;
	irr::s32 minX = irr::core::max_(0, centerX - radius);
	irr::s32 maxX = irr::core::min_((irr::s32)terrainSize - 1, centerX + radius);
	irr::s32 minZ = irr::core::max_(0, centerZ - radius);
	irr::s32 maxZ = irr::core::min_((irr::s32)terrainSize - 1, centerZ + radius);
	
	// Calculate average height for adaptive strength (if enabled)
	irr::f32 avgHeight = 0.0f;
	if (m_AdaptiveStrength)
	{
		irr::u32 sampleCount = 0;
		for (irr::s32 z = minZ; z <= maxZ; z += 2)
		{
			for (irr::s32 x = minX; x <= maxX; x += 2)
			{
				avgHeight += terrain->getHeightmapData()[z * terrainSize + x];
				sampleCount++;
			}
		}
		if (sampleCount > 0)
			avgHeight /= (irr::f32)sampleCount;
	}
	
	// Apply height changes
	irr::core::array<irr::f32> modifiedHeights;
	modifiedHeights.reallocate((maxX - minX + 1) * (maxZ - minZ + 1));
	
	for (irr::s32 z = minZ; z <= maxZ; ++z)
	{
		for (irr::s32 x = minX; x <= maxX; ++x)
		{
			// Calculate distance from brush center
			irr::f32 worldX, worldZ;
			terrain->heightmapToWorld(x, z, worldX, worldZ);
			irr::f32 distance = sqrtf((worldX - m_Position.X) * (worldX - m_Position.X) + 
									  (worldZ - m_Position.Z) * (worldZ - m_Position.Z));
			
			if (distance < m_Size)
			{
				// Get current height
				irr::u32 index = z * terrainSize + x;
				irr::f32 currentHeight = terrain->getHeightmapData()[index];
				
				// Calculate height delta
				irr::f32 heightDelta = calculateHeightDelta(currentHeight, distance, deltaTime);
				
				// Apply adaptive strength if enabled
				if (m_AdaptiveStrength)
				{
					irr::f32 adaptiveMultiplier = getAdaptiveStrengthMultiplier(currentHeight, avgHeight);
					heightDelta *= adaptiveMultiplier;
				}
				
				// Clamp delta to maximum change
				heightDelta = irr::core::clamp(heightDelta, -m_MaxDelta, m_MaxDelta);
				
				// Calculate new height
				irr::f32 newHeight = currentHeight + heightDelta;
				
				// Store the modified height
				modifiedHeights.push_back(newHeight);
				
				// Update terrain
				terrain->updateHeight(x, z, newHeight);
			}
		}
	}
}

irr::f32 HeightBrush::calculateHeightDelta(irr::f32 currentHeight, irr::f32 distance, irr::f32 deltaTime) const
{
	irr::f32 effectiveStrength = getEffectiveStrength(distance, deltaTime);
	
	switch (m_HeightMode)
	{
		case HEIGHT_RAISE:
			return effectiveStrength;
			
		case HEIGHT_LOWER:
			return -effectiveStrength;
			
		case HEIGHT_FLATTEN:
		case HEIGHT_SET:
		{
			irr::f32 heightDiff = m_TargetHeight - currentHeight;
			irr::f32 maxChange = effectiveStrength;
			
			// Move towards target height, but don't overshoot
			if (fabsf(heightDiff) <= maxChange)
				return heightDiff;
			else
				return heightDiff > 0 ? maxChange : -maxChange;
		}
		
		default:
			return 0.0f;
	}
}

irr::f32 HeightBrush::getAdaptiveStrengthMultiplier(irr::f32 currentHeight, irr::f32 avgHeight) const
{
	if (!m_AdaptiveStrength)
		return 1.0f;
		
	// Reduce strength on steep areas (high difference from average)
	irr::f32 heightDiff = fabsf(currentHeight - avgHeight);
	irr::f32 threshold = m_Size * 0.1f; // 10% of brush size
	
	if (heightDiff < threshold)
		return 1.0f;
	else
	{
		// Linear falloff for steep areas
		irr::f32 falloff = 1.0f - irr::core::clamp((heightDiff - threshold) / threshold, 0.0f, 0.5f);
		return falloff;
	}
}
