/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "irrlicht.h"

class UpdatableTerrainSceneNode;

/**
 * Base class for all terrain editing brushes.
 * Defines common brush properties and behavior.
 */
class TerrainBrush
{
public:
	enum BrushType
	{
		BRUSH_RAISE = 0,
		BRUSH_LOWER,
		BRUSH_FLATTEN,
		BRUSH_SMOOTH,
		BRUSH_NOISE,
		BRUSH_PAINT
	};

	enum FalloffType
	{
		FALLOFF_LINEAR = 0,
		FALLOFF_SMOOTH,
		FALLOFF_SHARP,
		FALLOFF_CONSTANT
	};

protected:
	// Brush properties
	irr::f32 m_Size;           // Brush radius in world units
	irr::f32 m_Strength;       // Brush strength (0.0 to 1.0)
	FalloffType m_Falloff;     // How brush strength falls off from center
	BrushType m_Type;          // Type of brush operation
	
	// Position and state
	irr::core::vector3df m_Position;     // Current brush position in world space
	bool m_IsActive;                     // Is brush currently being applied
	bool m_IsVisible;                    // Should brush indicator be shown
	
	// Timing for brush application
	irr::f32 m_CurrentTime;				// updated during apply
	irr::f32 m_LastApplyTime;
	irr::f32 m_ApplyInterval;            // Minimum time between applications (for performance)

public:
	TerrainBrush(BrushType type = BRUSH_RAISE);
	virtual ~TerrainBrush();

	// Brush application - override in derived classes
	virtual bool apply(UpdatableTerrainSceneNode* terrain, irr::f32 deltaTime) = 0;
	
	// Brush preview - override for brush-specific visualization
	virtual void renderPreview(irr::video::IVideoDriver* driver, const irr::core::matrix4& viewMatrix);
	
	// Property setters
	void setSize(irr::f32 size) { m_Size = irr::core::clamp(size, 0.1f, 100.0f); }
	void setStrength(irr::f32 strength) { m_Strength = irr::core::clamp(strength, 0.0f, 1.0f); }
	void setFalloff(FalloffType falloff) { m_Falloff = falloff; }
	void setPosition(const irr::core::vector3df& position) { m_Position = position; }
	void setActive(bool active) { m_IsActive = active; }
	void setVisible(bool visible) { m_IsVisible = visible; }
	
	// Property getters
	irr::f32 getSize() const { return m_Size; }
	irr::f32 getStrength() const { return m_Strength; }
	FalloffType getFalloff() const { return m_Falloff; }
	BrushType getType() const { return m_Type; }
	const irr::core::vector3df& getPosition() const { return m_Position; }
	bool isActive() const { return m_IsActive; }
	bool isVisible() const { return m_IsVisible; }
	
	// Time management
	void setCurrentTime(irr::f32 currentTime) { m_CurrentTime = currentTime; }
	irr::f32 getCurrentTime() const { return m_CurrentTime; }
	
	// Utility functions
	irr::f32 calculateFalloff(irr::f32 distance) const;
	irr::core::aabbox3df getBrushBounds() const;
	
protected:
	// Helper functions for derived classes
	bool isTimeToApply() const;
	void updateApplyTime();
	irr::f32 getEffectiveStrength(irr::f32 distance, irr::f32 deltaTime) const;
};
