/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "TerrainEditor.hpp"
#include "TerrainToolbar.hpp"
#include "UpdatableTerrainSceneNode.hpp"
#include "HeightBrush.hpp"
#include "SmoothBrush.hpp"
#include <wx/event.h>
#include <wx/log.h>

TerrainEditor::TerrainEditor(irr::video::IVideoDriver* driver, irr::scene::ISceneManager* sceneMgr, irr::ITimer* timer)
	: m_CurrentMode(MODE_SCULPT)
	, m_IsEnabled(false)
	, m_IsEditing(false)
	, m_Terrain(nullptr)
	, m_CurrentBrushIndex(-1)
	, m_CurrentBrush(nullptr)
	, m_LastMousePos(0, 0)
	, m_BrushWorldPos(0, 0, 0)
	, m_Driver(driver)
	, m_SceneManager(sceneMgr)
	, m_Camera(nullptr)
	, m_Timer(timer)
	, m_Toolbar(nullptr)
	, m_UndoIndex(-1)
	, m_MaxUndoSteps(20)
{
	// Initialize mouse buttons
	for (int i = 0; i < 3; ++i)
		m_MouseButtonDown[i] = false;
}

TerrainEditor::~TerrainEditor()
{
	shutdown();
}

bool TerrainEditor::initialize()
{
	if (!m_Driver || !m_SceneManager)
		return false;

	// Get active camera
	m_Camera = m_SceneManager->getActiveCamera();
	
	// Initialize default brushes
	initializeDefaultBrushes();
	
	// Set first brush as active if available
	if (m_Brushes.size() > 0)
		setCurrentBrush(0);

	m_IsEnabled = true;
	return true;
}

void TerrainEditor::shutdown()
{
	cleanupBrushes();
	//clearUndoHistory();
	m_IsEnabled = false;
}

void TerrainEditor::setTerrain(UpdatableTerrainSceneNode* terrain)
{
	m_Terrain = terrain;
	
	// Clear undo history when switching terrains
	clearUndoHistory();
}

bool TerrainEditor::addBrush(TerrainBrush* brush)
{
	if (!brush)
		return false;
		
	m_Brushes.push_back(brush);
	
	// Set as current brush if it's the first one
	if (m_Brushes.size() == 1)
		setCurrentBrush(0);
		
	return true;
}

void TerrainEditor::removeBrush(irr::s32 index)
{
	if (index < 0 || index >= (irr::s32)m_Brushes.size())
		return;
		
	// Delete the brush
	delete m_Brushes[index];
	m_Brushes.erase(index);
	
	// Update current brush index
	if (m_CurrentBrushIndex >= (irr::s32)m_Brushes.size())
	{
		m_CurrentBrushIndex = (irr::s32)m_Brushes.size() - 1;
	}
	
	// Update current brush pointer
	if (m_CurrentBrushIndex >= 0)
		m_CurrentBrush = m_Brushes[m_CurrentBrushIndex];
	else
		m_CurrentBrush = nullptr;
}

void TerrainEditor::setCurrentBrush(irr::s32 index)
{
	if (index < 0 || index >= (irr::s32)m_Brushes.size())
		return;
		
	// Deactivate current brush
	if (m_CurrentBrush)
		m_CurrentBrush->setActive(false);
		
	m_CurrentBrushIndex = index;
	m_CurrentBrush = m_Brushes[index];
	
	// Stop editing when switching brushes
	stopEditing();
}

TerrainBrush* TerrainEditor::getBrush(irr::s32 index) const
{
	if (index < 0 || index >= (irr::s32)m_Brushes.size())
		return nullptr;
		
	return m_Brushes[index];
}

bool TerrainEditor::onMouseEvent(wxMouseEvent& event)
{
	if (!m_IsEnabled || !m_Terrain || !m_CurrentBrush)
		return false;

	// Update mouse position
	m_LastMousePos.X = event.GetX();
	m_LastMousePos.Y = event.GetY();
	
	// Update brush position based on mouse
	updateBrushPosition(event.GetX(), event.GetY());

	// Handle mouse button events
	wxEventType type = event.GetEventType();
	
	if (type == wxEVT_LEFT_DOWN)
	{
		m_MouseButtonDown[0] = true;
		if (m_CurrentMode == MODE_SCULPT)
		{
			startEditing();
			return true;
		}
	}
	else if (type == wxEVT_LEFT_UP)
	{
		m_MouseButtonDown[0] = false;
		if (m_IsEditing)
		{
			stopEditing();
			return true;
		}
	}
	else if (type == wxEVT_RIGHT_DOWN)
	{
		m_MouseButtonDown[2] = true;
		// Right mouse button could switch to alternate brush mode
		if (m_CurrentBrush && m_CurrentBrush->getType() == TerrainBrush::BRUSH_RAISE)
		{
			// Switch to lower mode temporarily
			HeightBrush* heightBrush = static_cast<HeightBrush*>(m_CurrentBrush);
			heightBrush->setHeightMode(HeightBrush::HEIGHT_LOWER);
		}
		if (m_CurrentMode == MODE_SCULPT)
		{
			startEditing();
			return true;
		}
	}
	else if (type == wxEVT_RIGHT_UP)
	{
		m_MouseButtonDown[2] = false;
		// Restore original brush mode
		if (m_CurrentBrush && m_CurrentBrush->getType() == TerrainBrush::BRUSH_RAISE)
		{
			HeightBrush* heightBrush = static_cast<HeightBrush*>(m_CurrentBrush);
			heightBrush->setHeightMode(HeightBrush::HEIGHT_RAISE);
		}
		if (m_IsEditing)
		{
			stopEditing();
			return true;
		}
	}
	else if (type == wxEVT_MOUSEWHEEL)
	{
		// Mouse wheel adjusts brush size
		if (m_CurrentBrush)
		{
			irr::f32 currentSize = m_CurrentBrush->getSize();
			irr::f32 delta = event.GetWheelRotation() > 0 ? 1.2f : 0.8f;
			m_CurrentBrush->setSize(currentSize * delta);
			return true;
		}
	}
	else if (type == wxEVT_MOTION)
	{
		// Update brush position on mouse move
		updateBrushPosition(event.GetX(), event.GetY());
	}

	return false;
}

bool TerrainEditor::onKeyEvent(wxKeyEvent& event)
{
	if (!m_IsEnabled)
		return false;

	bool isKeyDown = (event.GetEventType() == wxEVT_KEY_DOWN);
	if (!isKeyDown)
		return false;

	int keyCode = event.GetKeyCode();
	
	// Switch brushes with number keys (1-5)
	if (keyCode >= '1' && keyCode <= '5')
	{
		irr::s32 brushIndex = keyCode - '1';
		if (brushIndex < (irr::s32)m_Brushes.size())
		{
			setCurrentBrush(brushIndex);
			return true;
		}
	}
	
	switch (keyCode)
	{
		case WXK_CONTROL:
			// Control key held - enable flatten mode temporarily
			if (m_CurrentBrush && m_CurrentBrush->getType() == TerrainBrush::BRUSH_RAISE)
			{
				HeightBrush* heightBrush = static_cast<HeightBrush*>(m_CurrentBrush);
				heightBrush->setHeightMode(HeightBrush::HEIGHT_FLATTEN);
				// Set target height to current terrain height at brush position
				heightBrush->setTargetHeight(m_Terrain->getHeight(m_BrushWorldPos.X, m_BrushWorldPos.Z));
			}
			break;
			
		case WXK_SHIFT:
			// Shift key increases brush strength temporarily
			if (m_CurrentBrush)
			{
				irr::f32 currentStrength = m_CurrentBrush->getStrength();
				m_CurrentBrush->setStrength(irr::core::min_(1.0f, currentStrength * 1.5f));
			}
			break;
			
		case 'Z':
		case 'z':
			// Ctrl+Z for undo
			if (event.ControlDown())
			{
				undo();
				return true;
			}
			break;
			
		case 'Y':
		case 'y':
			// Ctrl+Y for redo
			if (event.ControlDown())
			{
				redo();
				return true;
			}
			break;
	}

	return false;
}

void TerrainEditor::update(irr::f32 deltaTime)
{
	if (!m_IsEnabled || !m_Terrain || !m_CurrentBrush)
		return;

	// Update current time for all brushes
	for (irr::u32 i = 0; i < m_Brushes.size(); ++i)
	{
		m_Brushes[i]->setCurrentTime(deltaTime);
	}

	// Process brush input if editing
	if (m_IsEditing)
	{
		processBrushInput(deltaTime);
	}
	
	// Update brush position to follow terrain height
	if (m_Terrain)
	{
		irr::f32 terrainHeight = m_Terrain->getHeight(m_BrushWorldPos.X, m_BrushWorldPos.Z);
		m_BrushWorldPos.Y = terrainHeight;
		m_CurrentBrush->setPosition(m_BrushWorldPos);
	}
}

void TerrainEditor::render()
{
	if (!m_IsEnabled || !m_Driver)
		return;

	renderBrushPreview();
}

void TerrainEditor::renderBrushPreview()
{
	if (!m_CurrentBrush || !m_Camera)
		return;

	// Get view matrix
	irr::core::matrix4 viewMatrix = m_Camera->getViewMatrix();
	
	// Render brush preview
	m_CurrentBrush->renderPreview(m_Driver, viewMatrix);
}

void TerrainEditor::createSnapshot()
{
	if (!m_Terrain)
	{
		wxLogMessage(_("TerrainEditor::createSnapshot(): No terrain, aborting"));
		return;
	}

	// Create a snapshot before making changes
	TerrainSnapshot snapshot;
	snapshot.ModifiedRegion = irr::core::aabbox3d<irr::s32>(0, 0, 0, 
		(irr::s32)m_Terrain->getHeightmapSize() - 1, 0, (irr::s32)m_Terrain->getHeightmapSize() - 1);
	
	// Copy entire heightmap (could be optimized to only save modified regions)
	irr::u32 dataSize = m_Terrain->getHeightmapSize() * m_Terrain->getHeightmapSize();
	snapshot.HeightData.reallocate(dataSize);
	snapshot.HeightData.set_used(dataSize);
	
	const irr::f32* heightData = m_Terrain->getHeightmapData();
	for (irr::u32 i = 0; i < dataSize; ++i)
	{
		snapshot.HeightData[i] = heightData[i];
	}
	
	snapshot.Timestamp = m_Timer ? m_Timer->getTime() : 0;
	
	// Remove any redo data
	if (m_UndoIndex >= 0 && m_UndoIndex < (irr::s32)m_UndoStack.size() - 1)
	{
		for (irr::s32 i = m_UndoStack.size() - 1; i > m_UndoIndex; --i)
		{
			m_UndoStack.erase(i);
		}
	}
	
	// Add new snapshot
	m_UndoStack.push_back(snapshot);
	m_UndoIndex = (irr::s32)m_UndoStack.size() - 1;
	
	// Trim undo stack if too large
	trimUndoStack();
	
	// Update toolbar buttons
	updateToolbar();
}

bool TerrainEditor::undo()
{
	if (!canUndo())
		return false;

	// Get previous snapshot
	const TerrainSnapshot& snapshot = m_UndoStack[m_UndoIndex - 1];
	restoreTerrainSnapshot(snapshot);
	
	m_UndoIndex--;
	
	// Update toolbar buttons
	updateToolbar();
	
	return true;
}

bool TerrainEditor::redo()
{
	if (!canRedo())
		return false;

	// Get next snapshot
	const TerrainSnapshot& snapshot = m_UndoStack[m_UndoIndex + 1];
	restoreTerrainSnapshot(snapshot);
	
	m_UndoIndex++;
	
	// Update toolbar buttons
	updateToolbar();
	
	return true;
}

void TerrainEditor::clearUndoHistory()
{
	m_UndoStack.clear();
	m_UndoIndex = -1;
	
	// Update toolbar buttons
	updateToolbar();
}

bool TerrainEditor::canUndo() const
{
	bool result = m_UndoIndex > 0;
	return result;
}

bool TerrainEditor::canRedo() const
{
	bool result = m_UndoIndex >= 0 && m_UndoIndex < (irr::s32)m_UndoStack.size() - 1;
	return result;
}

bool TerrainEditor::getTerrainIntersection(irr::s32 screenX, irr::s32 screenY, irr::core::vector3df& worldPos)
{
	if (!m_Camera || !m_Terrain)
		return false;

	// Create ray from screen coordinates
	irr::core::line3d<irr::f32> ray = m_SceneManager->getSceneCollisionManager()->getRayFromScreenCoordinates(
		irr::core::position2di(screenX, screenY), m_Camera);

	// Get terrain triangle selector
	irr::scene::ITriangleSelector* selector = m_Terrain->getTriangleSelector();
	if (!selector)
		return false;

	// Perform collision detection
	irr::core::vector3df intersection;
	irr::core::triangle3df hitTriangle;
	irr::scene::ISceneNode* hitNode = nullptr;

	if (m_SceneManager->getSceneCollisionManager()->getCollisionPoint(
		ray, selector, intersection, hitTriangle, hitNode))
	{
		worldPos = intersection;
		return true;
	}

	// Fallback: intersect with terrain bounding box plane
	// Get terrain bounding box to determine appropriate Y level
	irr::core::aabbox3df terrainBBox = m_Terrain->getBoundingBox();
	irr::core::plane3df terrainPlane(irr::core::vector3df(0, 1, 0), terrainBBox.getCenter().Y);
	if (terrainPlane.getIntersectionWithLine(ray.start, ray.getVector(), worldPos))
	{
		return true;
	}

	// Final fallback: intersect with ground plane at Y=0
	irr::core::plane3df groundPlane(irr::core::vector3df(0, 1, 0), 0);
	if (groundPlane.getIntersectionWithLine(ray.start, ray.getVector(), worldPos))
	{
		return true;
	}

	return false;
}

void TerrainEditor::updateBrushPosition(irr::s32 screenX, irr::s32 screenY)
{
	irr::core::vector3df worldPos;
	if (getTerrainIntersection(screenX, screenY, worldPos))
	{
		m_BrushWorldPos = worldPos;
		
		if (m_CurrentBrush)
		{
			m_CurrentBrush->setPosition(worldPos);
		}
	}
}

void TerrainEditor::initializeDefaultBrushes()
{
	// Create default brush set
	addBrush(new HeightBrush(HeightBrush::HEIGHT_RAISE));    // Brush 1: Raise
	addBrush(new HeightBrush(HeightBrush::HEIGHT_LOWER));    // Brush 2: Lower
	addBrush(new HeightBrush(HeightBrush::HEIGHT_FLATTEN));  // Brush 3: Flatten
	addBrush(new SmoothBrush(SmoothBrush::SMOOTH_AVERAGE));  // Brush 4: Smooth
	addBrush(new SmoothBrush(SmoothBrush::SMOOTH_GAUSSIAN)); // Brush 5: Gaussian Smooth
}

void TerrainEditor::cleanupBrushes()
{
	for (irr::u32 i = 0; i < m_Brushes.size(); ++i)
	{
		delete m_Brushes[i];
	}
	m_Brushes.clear();
	m_CurrentBrush = nullptr;
	m_CurrentBrushIndex = -1;
}

void TerrainEditor::processBrushInput(irr::f32 deltaTime)
{
	if (!m_CurrentBrush || !m_IsEditing)
		return;

	// Apply brush to terrain
	m_CurrentBrush->apply(m_Terrain, deltaTime);
}

void TerrainEditor::startEditing()
{
	if (m_IsEditing || !m_CurrentBrush)
		return;

	// Create undo snapshot before editing
	createSnapshot();
	
	m_IsEditing = true;
	m_CurrentBrush->setActive(true);
}

void TerrainEditor::stopEditing()
{
	if (!m_IsEditing || !m_CurrentBrush)
		return;

	m_IsEditing = false;
	m_CurrentBrush->setActive(false);
}

void TerrainEditor::saveTerrainSnapshot(const irr::core::aabbox3d<irr::s32>& region)
{
	// This could be optimized to only save the specified region
	createSnapshot();
}

void TerrainEditor::restoreTerrainSnapshot(const TerrainSnapshot& snapshot)
{
	if (!m_Terrain)
		return;

	// Restore heightmap data
	irr::u32 terrainSize = m_Terrain->getHeightmapSize();
	
	for (irr::u32 z = 0; z < terrainSize; ++z)
	{
		for (irr::u32 x = 0; x < terrainSize; ++x)
		{
			irr::u32 index = z * terrainSize + x;
			if (index < snapshot.HeightData.size())
			{
				m_Terrain->updateHeight((irr::s32)x, (irr::s32)z, snapshot.HeightData[index]);
			}
		}
	}
}

void TerrainEditor::trimUndoStack()
{
	while (m_UndoStack.size() > m_MaxUndoSteps)
	{
		m_UndoStack.erase(0);
		m_UndoIndex--;
	}
	
	if (m_UndoIndex < 0)
		m_UndoIndex = 0;
}

void TerrainEditor::updateToolbar()
{
	if (m_Toolbar)
	{
		m_Toolbar->UpdateFromTerrainEditor();
	}
}
