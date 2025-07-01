/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "irrlicht.h"
#include "TerrainBrush.hpp"
#include <wx/event.h>

class UpdatableTerrainSceneNode;
class TerrainToolbar;

/**
 * Main terrain editing manager that coordinates brushes and handles input.
 * Manages the terrain editing workflow and tool switching.
 */
class TerrainEditor
{
public:
	enum EditMode
	{
		MODE_SCULPT = 0,    // Height modification mode
		MODE_PAINT,         // Texture painting mode
		MODE_SELECT         // Selection mode (no editing)
	};

private:
	// Editor state
	EditMode m_CurrentMode;
	bool m_IsEnabled;
	bool m_IsEditing;          // Currently applying brush
	
	// Terrain reference
	UpdatableTerrainSceneNode* m_Terrain;
	
	// Brush management
	irr::core::array<TerrainBrush*> m_Brushes;
	irr::s32 m_CurrentBrushIndex;
	TerrainBrush* m_CurrentBrush;
	
	// Input handling
	irr::core::vector2di m_LastMousePos;
	irr::core::vector3df m_BrushWorldPos;
	bool m_MouseButtonDown[3];  // Left, Middle, Right
	
	// Rendering
	irr::video::IVideoDriver* m_Driver;
	irr::scene::ISceneManager* m_SceneManager;
	irr::scene::ICameraSceneNode* m_Camera;
	irr::ITimer* m_Timer;
	
	// UI integration
	TerrainToolbar* m_Toolbar;
	
	// Undo/Redo system
	struct TerrainSnapshot
	{
		irr::core::aabbox3d<irr::s32> ModifiedRegion;
		irr::core::array<irr::f32> HeightData;
		irr::u32 Timestamp;
	};
	
	irr::core::array<TerrainSnapshot> m_UndoStack;
	irr::s32 m_UndoIndex;
	irr::u32 m_MaxUndoSteps;

public:
	TerrainEditor(irr::video::IVideoDriver* driver, irr::scene::ISceneManager* sceneMgr, irr::ITimer* timer);
	~TerrainEditor();

	// Initialization and cleanup
	bool initialize();
	void shutdown();
	
	// Terrain management
	void setTerrain(UpdatableTerrainSceneNode* terrain);
	UpdatableTerrainSceneNode* getTerrain() const { return m_Terrain; }
	
	// Camera management
	void setActiveCamera(irr::scene::ICameraSceneNode* camera) { m_Camera = camera; }
	irr::scene::ICameraSceneNode* getActiveCamera() const { return m_Camera; }
	
	// UI management
	void setToolbar(TerrainToolbar* toolbar) { m_Toolbar = toolbar; }
	TerrainToolbar* getToolbar() const { return m_Toolbar; }
	
	// Editor state
	void setEnabled(bool enabled) { m_IsEnabled = enabled; }
	bool isEnabled() const { return m_IsEnabled; }
	void setMode(EditMode mode) { m_CurrentMode = mode; }
	EditMode getMode() const { return m_CurrentMode; }
	
	// Brush management
	bool addBrush(TerrainBrush* brush);
	void removeBrush(irr::s32 index);
	void setCurrentBrush(irr::s32 index);
	TerrainBrush* getCurrentBrush() const { return m_CurrentBrush; }
	irr::s32 getCurrentBrushIndex() const { return m_CurrentBrushIndex; }
	irr::u32 getBrushCount() const { return m_Brushes.size(); }
	TerrainBrush* getBrush(irr::s32 index) const;
	
	// Input handling (wxWidgets events)
	bool onMouseEvent(wxMouseEvent& mouseEvent);
	bool onKeyEvent(wxKeyEvent& keyEvent);
	void update(irr::f32 deltaTime);
	
	// Rendering
	void render();
	void renderBrushPreview();
	
	// Undo/Redo system
	void createSnapshot();
	bool undo();
	bool redo();
	void clearUndoHistory();
	bool canUndo() const;
	bool canRedo() const;
	
	// Utility functions
	bool getTerrainIntersection(irr::s32 screenX, irr::s32 screenY, irr::core::vector3df& worldPos);
	void updateBrushPosition(irr::s32 screenX, irr::s32 screenY);

private:
	// Internal brush management
	void initializeDefaultBrushes();
	void cleanupBrushes();
	
	// Input processing
	void processBrushInput(irr::f32 deltaTime);
	void startEditing();
	void stopEditing();
	
	// Undo/Redo implementation
	void saveTerrainSnapshot(const irr::core::aabbox3d<irr::s32>& region);
	void restoreTerrainSnapshot(const TerrainSnapshot& snapshot);
	void trimUndoStack();
	
	// UI updates
	void updateToolbar();
};
