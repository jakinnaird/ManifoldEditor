/**
 * @file TerrainToolbar.hpp
 * @brief Floating terrain editing toolbar for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>

class ViewPanel;
class TerrainEditor;

/**
 * @class TerrainToolbar
 * @brief Floating toolbar for terrain editing controls
 * 
 * The TerrainToolbar class provides a floating window with controls for
 * terrain editing operations including brush selection, property adjustment,
 * and mode management.
 */
class TerrainToolbar : public wxFrame
{
private:
	ViewPanel* m_ViewPanel;              ///< Reference to the view panel
	TerrainEditor* m_TerrainEditor;      ///< Reference to the terrain editor

	// Brush type selection
	wxChoice* m_BrushTypeChoice;         ///< Brush type selector
	
	// Brush property controls
	wxSlider* m_SizeSlider;              ///< Brush size slider
	wxSlider* m_StrengthSlider;          ///< Brush strength slider
	wxChoice* m_FalloffChoice;           ///< Falloff type selector
	
	// Property value displays
	wxStaticText* m_SizeLabel;           ///< Size value display
	wxStaticText* m_StrengthLabel;       ///< Strength value display
	
	// Mode controls
	wxButton* m_ModeToggleButton;        ///< Terrain editing mode toggle
	
	// Action buttons
	wxButton* m_UndoButton;              ///< Undo button
	wxButton* m_RedoButton;              ///< Redo button
	
	// Height brush specific controls
	wxPanel* m_HeightPanel;              ///< Panel for height brush controls
	wxChoice* m_HeightModeChoice;        ///< Height mode selector (raise/lower/flatten)
	wxSlider* m_TargetHeightSlider;      ///< Target height for flatten mode
	wxStaticText* m_TargetHeightLabel;   ///< Target height value display
	wxCheckBox* m_AdaptiveCheckBox;      ///< Adaptive strength checkbox
	
	// Smooth brush specific controls
	wxPanel* m_SmoothPanel;              ///< Panel for smooth brush controls
	wxChoice* m_SmoothModeChoice;        ///< Smooth mode selector
	wxSlider* m_IterationsSlider;        ///< Smoothing iterations slider
	wxStaticText* m_IterationsLabel;     ///< Iterations value display

public:
	/**
	 * @brief Constructor for the TerrainToolbar class
	 * @param parent Pointer to the parent window
	 * @param viewPanel Pointer to the view panel
	 */
	TerrainToolbar(wxWindow* parent, ViewPanel* viewPanel);
	
	/**
	 * @brief Destructor
	 */
	~TerrainToolbar();

	/**
	 * @brief Update toolbar state based on terrain editor
	 */
	void UpdateFromTerrainEditor();
	
	/**
	 * @brief Set terrain editor reference
	 * @param terrainEditor Pointer to the terrain editor
	 */
	void SetTerrainEditor(TerrainEditor* terrainEditor);

private:
	/**
	 * @brief Create and layout all controls
	 */
	void CreateControls();
	
	/**
	 * @brief Update brush-specific panels visibility
	 */
	void UpdateBrushPanels();
	
	/**
	 * @brief Update value labels
	 */
	void UpdateLabels();

	// Event handlers
	void OnClose(wxCloseEvent& event);
	void OnBrushTypeChanged(wxCommandEvent& event);
	void OnSizeChanged(wxCommandEvent& event);
	void OnStrengthChanged(wxCommandEvent& event);
	void OnFalloffChanged(wxCommandEvent& event);
	void OnModeToggle(wxCommandEvent& event);
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);
	
	// Height brush events
	void OnHeightModeChanged(wxCommandEvent& event);
	void OnTargetHeightChanged(wxCommandEvent& event);
	void OnAdaptiveChanged(wxCommandEvent& event);
	
	// Smooth brush events
	void OnSmoothModeChanged(wxCommandEvent& event);
	void OnIterationsChanged(wxCommandEvent& event);
};

// Control IDs
enum TerrainToolbarIDs
{
	ID_BRUSH_TYPE = wxID_HIGHEST + 1000,
	ID_SIZE_SLIDER,
	ID_STRENGTH_SLIDER,
	ID_FALLOFF_CHOICE,
	ID_MODE_TOGGLE,
	ID_UNDO_BUTTON,
	ID_REDO_BUTTON,
	ID_HEIGHT_MODE,
	ID_TARGET_HEIGHT,
	ID_ADAPTIVE_CHECKBOX,
	ID_SMOOTH_MODE,
	ID_ITERATIONS_SLIDER
};
