/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "TerrainBrush.hpp"

/**
 * Height modification brush for raising and lowering terrain.
 * Can be configured for different height modification modes.
 */
class HeightBrush : public TerrainBrush
{
public:
	enum HeightMode
	{
		HEIGHT_RAISE = 0,    // Add height
		HEIGHT_LOWER,        // Subtract height
		HEIGHT_FLATTEN,      // Flatten to target height
		HEIGHT_SET           // Set to specific height
	};

private:
	HeightMode m_HeightMode;
	irr::f32 m_TargetHeight;     // For flatten and set modes
	irr::f32 m_MaxDelta;         // Maximum height change per application
	bool m_AdaptiveStrength;     // Adjust strength based on terrain slope

public:
	HeightBrush(HeightMode mode = HEIGHT_RAISE);
	virtual ~HeightBrush();

	// TerrainBrush interface
	virtual bool apply(UpdatableTerrainSceneNode* terrain, irr::f32 deltaTime) override;
	virtual void renderPreview(irr::video::IVideoDriver* driver, const irr::core::matrix4& viewMatrix) override;

	// Height brush specific methods
	void setHeightMode(HeightMode mode) { m_HeightMode = mode; }
	HeightMode getHeightMode() const { return m_HeightMode; }
	
	void setTargetHeight(irr::f32 height) { m_TargetHeight = height; }
	irr::f32 getTargetHeight() const { return m_TargetHeight; }
	
	void setMaxDelta(irr::f32 maxDelta) { m_MaxDelta = irr::core::max_(0.1f, maxDelta); }
	irr::f32 getMaxDelta() const { return m_MaxDelta; }
	
	void setAdaptiveStrength(bool adaptive) { m_AdaptiveStrength = adaptive; }
	bool getAdaptiveStrength() const { return m_AdaptiveStrength; }

private:
	// Internal height modification
	irr::f32 calculateHeightDelta(irr::f32 currentHeight, irr::f32 distance, irr::f32 deltaTime) const;
	void applyHeightToRegion(UpdatableTerrainSceneNode* terrain, 
		irr::s32 centerX, irr::s32 centerZ, irr::f32 deltaTime);
	irr::f32 getAdaptiveStrengthMultiplier(irr::f32 currentHeight, irr::f32 avgHeight) const;
};
