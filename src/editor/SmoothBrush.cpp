/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "SmoothBrush.hpp"
#include "UpdatableTerrainSceneNode.hpp"

SmoothBrush::SmoothBrush(SmoothMode mode)
	: TerrainBrush(BRUSH_SMOOTH)
	, m_SmoothMode(mode)
	, m_PreserveThreshold(0.5f)
	, m_Iterations(1)
{
}

SmoothBrush::~SmoothBrush()
{
}

bool SmoothBrush::apply(UpdatableTerrainSceneNode* terrain, irr::f32 deltaTime)
{
	if (!terrain || !m_IsActive || !isTimeToApply())
		return false;

	// Convert world position to heightmap coordinates
	irr::s32 centerX, centerZ;
	terrain->worldToHeightmap(m_Position.X, m_Position.Z, centerX, centerZ);
	
	// Validate coordinates
	if (centerX < 0 || centerZ < 0 || 
		(irr::u32)centerX >= terrain->getHeightmapSize() || 
		(irr::u32)centerZ >= terrain->getHeightmapSize())
		return false;

	// Apply smoothing
	applySmoothingToRegion(terrain, centerX, centerZ, deltaTime);
	
	updateApplyTime();
	return true;
}

void SmoothBrush::applySmoothingToRegion(UpdatableTerrainSceneNode* terrain, 
	irr::s32 centerX, irr::s32 centerZ, irr::f32 deltaTime)
{
	irr::u32 terrainSize = terrain->getHeightmapSize();
	const irr::f32* heightData = terrain->getHeightmapData();
	
	// Calculate affected region in heightmap coordinates
	irr::s32 radius = (irr::s32)(m_Size / terrain->getTerrainScale().X) + 1;
	irr::s32 minX = irr::core::max_(0, centerX - radius);
	irr::s32 maxX = irr::core::min_((irr::s32)terrainSize - 1, centerX + radius);
	irr::s32 minZ = irr::core::max_(0, centerZ - radius);
	irr::s32 maxZ = irr::core::min_((irr::s32)terrainSize - 1, centerZ + radius);
	
	// Store smoothed heights for batch update
	irr::core::array<irr::f32> smoothedHeights;
	irr::core::array<irr::s32> modifiedX;
	irr::core::array<irr::s32> modifiedZ;
	
	// Apply multiple iterations if requested
	for (irr::u32 iteration = 0; iteration < m_Iterations; ++iteration)
	{
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
					irr::f32 currentHeight = heightData[index];
					
					// Calculate smoothed height based on mode
					irr::f32 smoothedHeight;
					switch (m_SmoothMode)
					{
						case SMOOTH_AVERAGE:
							smoothedHeight = calculateAverageSmoothing(heightData, terrainSize, x, z, distance);
							break;
						case SMOOTH_GAUSSIAN:
							smoothedHeight = calculateGaussianSmoothing(heightData, terrainSize, x, z, distance);
							break;
						case SMOOTH_PRESERVE_DETAIL:
							smoothedHeight = calculateDetailPreservingSmoothing(heightData, terrainSize, x, z, distance, currentHeight);
							break;
						default:
							smoothedHeight = currentHeight;
							break;
					}
					
					// Apply strength-based blending
					irr::f32 effectiveStrength = getEffectiveStrength(distance, deltaTime);
					irr::f32 newHeight = currentHeight + (smoothedHeight - currentHeight) * effectiveStrength;
					
					// Store for batch update
					if (iteration == 0) // Only store coordinates once
					{
						modifiedX.push_back(x);
						modifiedZ.push_back(z);
					}
					smoothedHeights.push_back(newHeight);
				}
			}
		}
		
		// Update heightmap data for next iteration if there are more iterations
		if (iteration < m_Iterations - 1)
		{
			for (irr::u32 i = 0; i < modifiedX.size(); ++i)
			{
				terrain->updateHeight(modifiedX[i], modifiedZ[i], smoothedHeights[i]);
			}
			heightData = terrain->getHeightmapData(); // Refresh data pointer
			smoothedHeights.clear();
		}
	}
	
	// Final update to terrain
	for (irr::u32 i = 0; i < modifiedX.size(); ++i)
	{
		terrain->updateHeight(modifiedX[i], modifiedZ[i], smoothedHeights[i]);
	}
}

irr::f32 SmoothBrush::calculateAverageSmoothing(const irr::f32* heightData, irr::u32 terrainSize,
	irr::s32 x, irr::s32 z, irr::f32 distance) const
{
	// Simple box filter - average heights in a small neighborhood
	irr::s32 radius = 1; // 3x3 kernel
	irr::f32 totalHeight = 0.0f;
	irr::u32 sampleCount = 0;
	
	for (irr::s32 dz = -radius; dz <= radius; ++dz)
	{
		for (irr::s32 dx = -radius; dx <= radius; ++dx)
		{
			irr::s32 sampleX = x + dx;
			irr::s32 sampleZ = z + dz;
			
			if (sampleX >= 0 && sampleX < (irr::s32)terrainSize && 
				sampleZ >= 0 && sampleZ < (irr::s32)terrainSize)
			{
				totalHeight += heightData[sampleZ * terrainSize + sampleX];
				sampleCount++;
			}
		}
	}
	
	return sampleCount > 0 ? totalHeight / (irr::f32)sampleCount : heightData[z * terrainSize + x];
}

irr::f32 SmoothBrush::calculateGaussianSmoothing(const irr::f32* heightData, irr::u32 terrainSize,
	irr::s32 x, irr::s32 z, irr::f32 distance) const
{
	// Gaussian blur with variable kernel size based on brush size
	irr::s32 radius = irr::core::max_(1, (irr::s32)(m_Size / 10.0f)); // Adaptive kernel size
	irr::f32 sigma = (irr::f32)radius / 3.0f; // Standard deviation
	
	irr::f32 totalWeight = 0.0f;
	irr::f32 weightedHeight = 0.0f;
	
	for (irr::s32 dz = -radius; dz <= radius; ++dz)
	{
		for (irr::s32 dx = -radius; dx <= radius; ++dx)
		{
			irr::s32 sampleX = x + dx;
			irr::s32 sampleZ = z + dz;
			
			if (sampleX >= 0 && sampleX < (irr::s32)terrainSize && 
				sampleZ >= 0 && sampleZ < (irr::s32)terrainSize)
			{
				irr::f32 sampleDistance = sqrtf((irr::f32)(dx * dx + dz * dz));
				irr::f32 weight = getGaussianWeight(sampleDistance, sigma);
				
				weightedHeight += heightData[sampleZ * terrainSize + sampleX] * weight;
				totalWeight += weight;
			}
		}
	}
	
	return totalWeight > 0.0f ? weightedHeight / totalWeight : heightData[z * terrainSize + x];
}

irr::f32 SmoothBrush::calculateDetailPreservingSmoothing(const irr::f32* heightData, irr::u32 terrainSize,
	irr::s32 x, irr::s32 z, irr::f32 distance, irr::f32 originalHeight) const
{
	// Calculate standard average smoothing first
	irr::f32 smoothedHeight = calculateAverageSmoothing(heightData, terrainSize, x, z, distance);
	
	// Check if the difference is significant enough to preserve detail
	irr::f32 heightDifference = fabsf(smoothedHeight - originalHeight);
	
	if (heightDifference < m_PreserveThreshold)
	{
		// Small difference - apply full smoothing
		return smoothedHeight;
	}
	else
	{
		// Large difference - preserve some of the original detail
		irr::f32 preservationFactor = irr::core::clamp(
			(heightDifference - m_PreserveThreshold) / m_PreserveThreshold, 
			0.0f, 0.7f);
		
		return smoothedHeight + (originalHeight - smoothedHeight) * preservationFactor;
	}
}

irr::f32 SmoothBrush::getGaussianWeight(irr::f32 distance, irr::f32 sigma) const
{
	if (sigma <= 0.0f)
		return distance == 0.0f ? 1.0f : 0.0f;
		
	irr::f32 sigmaSquared = sigma * sigma;
	return expf(-(distance * distance) / (2.0f * sigmaSquared)) / (2.0f * irr::core::PI * sigmaSquared);
}
