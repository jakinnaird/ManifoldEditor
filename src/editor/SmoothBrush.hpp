/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "TerrainBrush.hpp"

/**
 * Smoothing brush for creating natural terrain transitions.
 * Reduces sharp edges and creates gentle slopes.
 */
class SmoothBrush : public TerrainBrush
{
public:
	enum SmoothMode
	{
		SMOOTH_AVERAGE = 0,    // Average heights in brush area
		SMOOTH_GAUSSIAN,       // Gaussian blur smoothing
		SMOOTH_PRESERVE_DETAIL // Smooth while preserving fine details
	};

private:
	SmoothMode m_SmoothMode;
	irr::f32 m_PreserveThreshold;  // For detail preserving mode
	irr::u32 m_Iterations;         // Number of smoothing passes per application

public:
	SmoothBrush(SmoothMode mode = SMOOTH_AVERAGE);
	virtual ~SmoothBrush();

	// TerrainBrush interface
	virtual bool apply(UpdatableTerrainSceneNode* terrain, irr::f32 deltaTime) override;

	// Smooth brush specific methods
	void setSmoothMode(SmoothMode mode) { m_SmoothMode = mode; }
	SmoothMode getSmoothMode() const { return m_SmoothMode; }
	
	void setPreserveThreshold(irr::f32 threshold) { m_PreserveThreshold = irr::core::max_(0.01f, threshold); }
	irr::f32 getPreserveThreshold() const { return m_PreserveThreshold; }
	
	void setIterations(irr::u32 iterations) { m_Iterations = irr::core::clamp(iterations, 1u, 10u); }
	irr::u32 getIterations() const { return m_Iterations; }

private:
	// Smoothing algorithms
	void applySmoothingToRegion(UpdatableTerrainSceneNode* terrain, 
		irr::s32 centerX, irr::s32 centerZ, irr::f32 deltaTime);
	irr::f32 calculateAverageSmoothing(const irr::f32* heightData, irr::u32 terrainSize,
		irr::s32 x, irr::s32 z, irr::f32 distance) const;
	irr::f32 calculateGaussianSmoothing(const irr::f32* heightData, irr::u32 terrainSize,
		irr::s32 x, irr::s32 z, irr::f32 distance) const;
	irr::f32 calculateDetailPreservingSmoothing(const irr::f32* heightData, irr::u32 terrainSize,
		irr::s32 x, irr::s32 z, irr::f32 distance, irr::f32 originalHeight) const;
	irr::f32 getGaussianWeight(irr::f32 distance, irr::f32 sigma) const;
};
 