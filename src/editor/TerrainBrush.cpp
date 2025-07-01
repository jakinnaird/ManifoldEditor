/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "TerrainBrush.hpp"
#include "UpdatableTerrainSceneNode.hpp"

TerrainBrush::TerrainBrush(BrushType type)
	: m_Size(5.0f)
	, m_Strength(0.1f) // Reduced from 0.5f for slower, more controlled changes
	, m_Falloff(FALLOFF_SMOOTH)
	, m_Type(type)
	, m_Position(0, 0, 0)
	, m_IsActive(false)
	, m_IsVisible(true)
	, m_CurrentTime(0.0f)
	, m_LastApplyTime(0.0f)
	, m_ApplyInterval(1.0f / 15.0f) // 15 FPS max application rate for slower, more controlled editing
{
}

TerrainBrush::~TerrainBrush()
{
}

void TerrainBrush::renderPreview(irr::video::IVideoDriver* driver, const irr::core::matrix4& viewMatrix)
{
	if (!m_IsVisible || !driver)
		return;

	// Set up material for brush preview - optimized for TOP_VIEW
	irr::video::SMaterial previewMaterial;
	previewMaterial.Lighting = false;
	previewMaterial.ZBuffer = irr::video::ECFN_LESSEQUAL;
	previewMaterial.ZWriteEnable = false;
	previewMaterial.MaterialType = irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	
	// Color based on brush type and state - enhanced for TOP_VIEW visibility
	irr::video::SColor brushColor;
	if (m_IsActive)
		brushColor = irr::video::SColor(255, 255, 255, 0); // Bright yellow when active
	else
	{
		switch (m_Type)
		{
			case BRUSH_RAISE:
				brushColor = irr::video::SColor(200, 0, 255, 0); // Bright green for raise
				break;
			case BRUSH_LOWER:
				brushColor = irr::video::SColor(200, 255, 0, 0); // Bright red for lower
				break;
			case BRUSH_FLATTEN:
				brushColor = irr::video::SColor(200, 0, 128, 255); // Bright blue for flatten
				break;
			case BRUSH_SMOOTH:
				brushColor = irr::video::SColor(200, 255, 128, 255); // Bright magenta for smooth
				break;
			default:
				brushColor = irr::video::SColor(200, 192, 192, 192); // Bright gray for others
				break;
		}
	}
	
	driver->setMaterial(previewMaterial);
	driver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
	
	// Draw brush circle at terrain height
	const irr::u32 segments = 32;
	const irr::f32 angleStep = irr::core::PI * 2.0f / segments;
	
	for (irr::u32 i = 0; i < segments; ++i)
	{
		irr::f32 angle1 = i * angleStep;
		irr::f32 angle2 = (i + 1) * angleStep;
		
		irr::core::vector3df pos1(
			m_Position.X + cosf(angle1) * m_Size,
			m_Position.Y + 2.0f, // Higher above terrain for better TOP_VIEW visibility
			m_Position.Z + sinf(angle1) * m_Size
		);
		
		irr::core::vector3df pos2(
			m_Position.X + cosf(angle2) * m_Size,
			m_Position.Y + 2.0f, // Higher above terrain for better TOP_VIEW visibility
			m_Position.Z + sinf(angle2) * m_Size
		);
		
		driver->draw3DLine(pos1, pos2, brushColor);
	}
	
	// Draw center indicator for precise positioning in TOP_VIEW
	irr::f32 centerSize = m_Size * 0.1f;
	irr::core::vector3df centerPos(m_Position.X, m_Position.Y + 2.5f, m_Position.Z);
	irr::video::SColor centerColor(255, 255, 255, 255); // Bright white
	
	// Draw cross at center
	driver->draw3DLine(
		centerPos + irr::core::vector3df(-centerSize, 0, 0),
		centerPos + irr::core::vector3df(centerSize, 0, 0),
		centerColor
	);
	driver->draw3DLine(
		centerPos + irr::core::vector3df(0, 0, -centerSize),
		centerPos + irr::core::vector3df(0, 0, centerSize),
		centerColor
	);
	
	// Draw falloff indicator (inner circle for strength visualization)
	if (m_Falloff != FALLOFF_CONSTANT)
	{
		irr::f32 innerRadius = m_Size * 0.5f;
		irr::video::SColor innerColor = brushColor;
		innerColor.setAlpha(64);
		
		for (irr::u32 i = 0; i < segments; ++i)
		{
			irr::f32 angle1 = i * angleStep;
			irr::f32 angle2 = (i + 1) * angleStep;
			
			irr::core::vector3df pos1(
				m_Position.X + cosf(angle1) * innerRadius,
				m_Position.Y + 2.0f, // Higher above terrain for better TOP_VIEW visibility
				m_Position.Z + sinf(angle1) * innerRadius
			);
			
			irr::core::vector3df pos2(
				m_Position.X + cosf(angle2) * innerRadius,
				m_Position.Y + 2.0f, // Higher above terrain for better TOP_VIEW visibility
				m_Position.Z + sinf(angle2) * innerRadius
			);
			
			driver->draw3DLine(pos1, pos2, innerColor);
		}
	}
}

irr::f32 TerrainBrush::calculateFalloff(irr::f32 distance) const
{
	if (distance >= m_Size)
		return 0.0f;
	
	irr::f32 normalizedDistance = distance / m_Size;
	
	switch (m_Falloff)
	{
		case FALLOFF_LINEAR:
			return 1.0f - normalizedDistance;
			
		case FALLOFF_SMOOTH:
			// Smooth falloff using smoothstep function
			return 1.0f - (normalizedDistance * normalizedDistance * (3.0f - 2.0f * normalizedDistance));
			
		case FALLOFF_SHARP:
			// Sharp falloff - more strength near center
			return 1.0f - (normalizedDistance * normalizedDistance);
			
		case FALLOFF_CONSTANT:
			return 1.0f;
			
		default:
			return 1.0f - normalizedDistance;
	}
}

irr::core::aabbox3df TerrainBrush::getBrushBounds() const
{
	return irr::core::aabbox3df(
		m_Position.X - m_Size, m_Position.Y - m_Size, m_Position.Z - m_Size,
		m_Position.X + m_Size, m_Position.Y + m_Size, m_Position.Z + m_Size
	);
}

bool TerrainBrush::isTimeToApply() const
{
	//irr::ITimer* timer = irr::getTimer();
	//if (!timer)
	//	return true;
	//	
	//irr::f32 currentTime = timer->getTime() / 1000.0f;
	return (m_CurrentTime - m_LastApplyTime) >= m_ApplyInterval;
}

void TerrainBrush::updateApplyTime()
{
	//irr::ITimer* timer = irr::getTimer();
	//if (timer)
	//{
	//	m_LastApplyTime = timer->getTime() / 1000.0f;
	//}

	m_LastApplyTime = m_CurrentTime;
}

irr::f32 TerrainBrush::getEffectiveStrength(irr::f32 distance, irr::f32 deltaTime) const
{
	irr::f32 falloffMultiplier = calculateFalloff(distance);
	// Reduced time multiplier for slower, more controlled changes
	// Use a smaller multiplier instead of normalizing to 60 FPS
	irr::f32 timeMultiplier = deltaTime * 10.0f; // Much slower rate
	return m_Strength * falloffMultiplier * timeMultiplier;
}
