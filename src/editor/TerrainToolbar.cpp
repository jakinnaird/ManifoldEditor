/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "TerrainToolbar.hpp"
#include "ViewPanel.hpp"
#include "TerrainEditor.hpp"
#include "TerrainBrush.hpp"
#include "HeightBrush.hpp"
#include "SmoothBrush.hpp"

#include <wx/artprov.h>
#include <wx/log.h>
#include <wx/statbox.h>

TerrainToolbar::TerrainToolbar(wxWindow* parent, ViewPanel* viewPanel)
	: wxFrame(parent, wxID_ANY, _("Terrain Editor"), wxDefaultPosition, wxDefaultSize, 
		wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_TOOL_WINDOW)
	, m_ViewPanel(viewPanel)
	, m_TerrainEditor(nullptr)
	, m_BrushTypeChoice(nullptr)
	, m_SizeSlider(nullptr)
	, m_StrengthSlider(nullptr)
	, m_FalloffChoice(nullptr)
	, m_SizeLabel(nullptr)
	, m_StrengthLabel(nullptr)
	, m_ModeToggleButton(nullptr)
	, m_UndoButton(nullptr)
	, m_RedoButton(nullptr)
	, m_HeightPanel(nullptr)
	, m_HeightModeChoice(nullptr)
	, m_TargetHeightSlider(nullptr)
	, m_TargetHeightLabel(nullptr)
	, m_AdaptiveCheckBox(nullptr)
	, m_SmoothPanel(nullptr)
	, m_SmoothModeChoice(nullptr)
	, m_IterationsSlider(nullptr)
	, m_IterationsLabel(nullptr)
{
	CreateControls();
	
	// Set window size and make it non-resizable for now
	SetSize(250, 625);
	// SetMinSize(wxSize(250, 650));
	// SetMaxSize(wxSize(250, 400));
	
	// Center on parent initially
	// CenterOnParent();

	// make sure it's on the left side of the parent
	SetPosition(parent->GetPosition());
	
	// Bind close event
	Bind(wxEVT_CLOSE_WINDOW, &TerrainToolbar::OnClose, this);
}

TerrainToolbar::~TerrainToolbar()
{
}

void TerrainToolbar::CreateControls()
{
	wxPanel* mainPanel = new wxPanel(this, wxID_ANY);
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	
	// Mode control section
	wxStaticBoxSizer* modeSizer = new wxStaticBoxSizer(wxVERTICAL, mainPanel, _("Mode"));
	m_ModeToggleButton = new wxButton(mainPanel, ID_MODE_TOGGLE, _("Enable Terrain Editing"));
	modeSizer->Add(m_ModeToggleButton, 0, wxEXPAND | wxALL, 5);
	mainSizer->Add(modeSizer, 0, wxEXPAND | wxALL, 5);
	
	// Brush type selection
	wxStaticBoxSizer* brushSizer = new wxStaticBoxSizer(wxVERTICAL, mainPanel, _("Brush Type"));
	wxArrayString brushTypes;
	brushTypes.Add(_("Height - Raise"));
	brushTypes.Add(_("Height - Lower"));
	brushTypes.Add(_("Height - Flatten"));
	brushTypes.Add(_("Smooth - Average"));
	brushTypes.Add(_("Smooth - Gaussian"));
	m_BrushTypeChoice = new wxChoice(mainPanel, ID_BRUSH_TYPE, wxDefaultPosition, wxDefaultSize, brushTypes);
	m_BrushTypeChoice->SetSelection(0);
	brushSizer->Add(m_BrushTypeChoice, 0, wxEXPAND | wxALL, 5);
	mainSizer->Add(brushSizer, 0, wxEXPAND | wxALL, 5);
	
	// Brush properties section
	wxStaticBoxSizer* propSizer = new wxStaticBoxSizer(wxVERTICAL, mainPanel, _("Brush Properties"));
	
	// Size control
	propSizer->Add(new wxStaticText(mainPanel, wxID_ANY, _("Size:")), 0, wxALL, 2);
	m_SizeSlider = new wxSlider(mainPanel, ID_SIZE_SLIDER, 50, 10, 500, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	propSizer->Add(m_SizeSlider, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	m_SizeLabel = new wxStaticText(mainPanel, wxID_ANY, _("5.0"));
	propSizer->Add(m_SizeLabel, 0, wxALIGN_CENTER | wxALL, 2);
	
	// Strength control
	propSizer->Add(new wxStaticText(mainPanel, wxID_ANY, _("Strength:")), 0, wxALL, 2);
	m_StrengthSlider = new wxSlider(mainPanel, ID_STRENGTH_SLIDER, 10, 1, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	propSizer->Add(m_StrengthSlider, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	m_StrengthLabel = new wxStaticText(mainPanel, wxID_ANY, _("0.1"));
	propSizer->Add(m_StrengthLabel, 0, wxALIGN_CENTER | wxALL, 2);
	
	// Falloff control
	propSizer->Add(new wxStaticText(mainPanel, wxID_ANY, _("Falloff:")), 0, wxALL, 2);
	wxArrayString falloffTypes;
	falloffTypes.Add(_("Linear"));
	falloffTypes.Add(_("Smooth"));
	falloffTypes.Add(_("Sharp"));
	falloffTypes.Add(_("Constant"));
	m_FalloffChoice = new wxChoice(mainPanel, ID_FALLOFF_CHOICE, wxDefaultPosition, wxDefaultSize, falloffTypes);
	m_FalloffChoice->SetSelection(1); // Default to Smooth
	propSizer->Add(m_FalloffChoice, 0, wxEXPAND | wxALL, 5);
	
	mainSizer->Add(propSizer, 0, wxEXPAND | wxALL, 5);
	
	// Height brush specific controls
	m_HeightPanel = new wxPanel(mainPanel, wxID_ANY);
	wxStaticBoxSizer* heightSizer = new wxStaticBoxSizer(wxVERTICAL, m_HeightPanel, _("Height Options"));
	
	// Height mode
	heightSizer->Add(new wxStaticText(m_HeightPanel, wxID_ANY, _("Mode:")), 0, wxALL, 2);
	wxArrayString heightModes;
	heightModes.Add(_("Raise"));
	heightModes.Add(_("Lower"));
	heightModes.Add(_("Flatten"));
	heightModes.Add(_("Set Height"));
	m_HeightModeChoice = new wxChoice(m_HeightPanel, ID_HEIGHT_MODE, wxDefaultPosition, wxDefaultSize, heightModes);
	m_HeightModeChoice->SetSelection(0);
	heightSizer->Add(m_HeightModeChoice, 0, wxEXPAND | wxALL, 5);
	
	// Target height (for flatten mode)
	heightSizer->Add(new wxStaticText(m_HeightPanel, wxID_ANY, _("Target Height:")), 0, wxALL, 2);
	m_TargetHeightSlider = new wxSlider(m_HeightPanel, ID_TARGET_HEIGHT, 0, -100, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	heightSizer->Add(m_TargetHeightSlider, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	m_TargetHeightLabel = new wxStaticText(m_HeightPanel, wxID_ANY, _("0.0"));
	heightSizer->Add(m_TargetHeightLabel, 0, wxALIGN_CENTER | wxALL, 2);
	
	// Adaptive strength
	m_AdaptiveCheckBox = new wxCheckBox(m_HeightPanel, ID_ADAPTIVE_CHECKBOX, _("Adaptive Strength"));
	heightSizer->Add(m_AdaptiveCheckBox, 0, wxALL, 5);
	
	m_HeightPanel->SetSizer(heightSizer);
	mainSizer->Add(m_HeightPanel, 0, wxEXPAND | wxALL, 5);
	
	// Smooth brush specific controls
	m_SmoothPanel = new wxPanel(mainPanel, wxID_ANY);
	wxStaticBoxSizer* smoothSizer = new wxStaticBoxSizer(wxVERTICAL, m_SmoothPanel, _("Smooth Options"));
	
	// Smooth mode
	smoothSizer->Add(new wxStaticText(m_SmoothPanel, wxID_ANY, _("Algorithm:")), 0, wxALL, 2);
	wxArrayString smoothModes;
	smoothModes.Add(_("Average"));
	smoothModes.Add(_("Gaussian"));
	smoothModes.Add(_("Preserve Detail"));
	m_SmoothModeChoice = new wxChoice(m_SmoothPanel, ID_SMOOTH_MODE, wxDefaultPosition, wxDefaultSize, smoothModes);
	m_SmoothModeChoice->SetSelection(0);
	smoothSizer->Add(m_SmoothModeChoice, 0, wxEXPAND | wxALL, 5);
	
	// Iterations
	smoothSizer->Add(new wxStaticText(m_SmoothPanel, wxID_ANY, _("Iterations:")), 0, wxALL, 2);
	m_IterationsSlider = new wxSlider(m_SmoothPanel, ID_ITERATIONS_SLIDER, 1, 1, 5, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
	smoothSizer->Add(m_IterationsSlider, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	m_IterationsLabel = new wxStaticText(m_SmoothPanel, wxID_ANY, _("1"));
	smoothSizer->Add(m_IterationsLabel, 0, wxALIGN_CENTER | wxALL, 2);
	
	m_SmoothPanel->SetSizer(smoothSizer);
	mainSizer->Add(m_SmoothPanel, 0, wxEXPAND | wxALL, 5);
	
	// Action buttons section
	wxStaticBoxSizer* actionSizer = new wxStaticBoxSizer(wxHORIZONTAL, mainPanel, _("Actions"));
	m_UndoButton = new wxButton(mainPanel, ID_UNDO_BUTTON, _("Undo"));
	m_RedoButton = new wxButton(mainPanel, ID_REDO_BUTTON, _("Redo"));
	
	// Start buttons as disabled (they'll be enabled when terrain editing state allows)
	m_UndoButton->Enable(false);
	m_RedoButton->Enable(false);
	
	actionSizer->Add(m_UndoButton, 1, wxEXPAND | wxALL, 5);
	actionSizer->Add(m_RedoButton, 1, wxEXPAND | wxALL, 5);
	mainSizer->Add(actionSizer, 0, wxEXPAND | wxALL, 5);
	
	mainPanel->SetSizer(mainSizer);
	
	// Bind events
	Bind(wxEVT_CHOICE, &TerrainToolbar::OnBrushTypeChanged, this, ID_BRUSH_TYPE);
	Bind(wxEVT_SLIDER, &TerrainToolbar::OnSizeChanged, this, ID_SIZE_SLIDER);
	Bind(wxEVT_SLIDER, &TerrainToolbar::OnStrengthChanged, this, ID_STRENGTH_SLIDER);
	Bind(wxEVT_CHOICE, &TerrainToolbar::OnFalloffChanged, this, ID_FALLOFF_CHOICE);
	Bind(wxEVT_BUTTON, &TerrainToolbar::OnModeToggle, this, ID_MODE_TOGGLE);
	Bind(wxEVT_BUTTON, &TerrainToolbar::OnUndo, this, ID_UNDO_BUTTON);
	Bind(wxEVT_BUTTON, &TerrainToolbar::OnRedo, this, ID_REDO_BUTTON);
	Bind(wxEVT_CHOICE, &TerrainToolbar::OnHeightModeChanged, this, ID_HEIGHT_MODE);
	Bind(wxEVT_SLIDER, &TerrainToolbar::OnTargetHeightChanged, this, ID_TARGET_HEIGHT);
	Bind(wxEVT_CHECKBOX, &TerrainToolbar::OnAdaptiveChanged, this, ID_ADAPTIVE_CHECKBOX);
	Bind(wxEVT_CHOICE, &TerrainToolbar::OnSmoothModeChanged, this, ID_SMOOTH_MODE);
	Bind(wxEVT_SLIDER, &TerrainToolbar::OnIterationsChanged, this, ID_ITERATIONS_SLIDER);
	
	// Initial state
	UpdateBrushPanels();
	UpdateLabels();
}

void TerrainToolbar::SetTerrainEditor(TerrainEditor* terrainEditor)
{
	m_TerrainEditor = terrainEditor;
	UpdateFromTerrainEditor();
}

void TerrainToolbar::UpdateFromTerrainEditor()
{
	if (!m_TerrainEditor)
		return;
		
	// Update mode button
	bool isEnabled = m_TerrainEditor->isEnabled();
	m_ModeToggleButton->SetLabel(isEnabled ? _("Disable Terrain Editing") : _("Enable Terrain Editing"));
	
	// Update undo/redo buttons
	bool canUndo = m_TerrainEditor->canUndo();
	bool canRedo = m_TerrainEditor->canRedo();
	
	m_UndoButton->Enable(canUndo);
	m_RedoButton->Enable(canRedo);
	
	// Force UI refresh
	m_UndoButton->Refresh();
	m_RedoButton->Refresh();
	Update(); // Update the entire toolbar window
	
	// Double-check that the buttons actually got enabled
	bool undoActuallyEnabled = m_UndoButton->IsEnabled();
	bool redoActuallyEnabled = m_RedoButton->IsEnabled();
	
	// Try a more aggressive refresh if the buttons still aren't working
	if (canUndo && !undoActuallyEnabled)
	{
		wxLogMessage(_("TerrainToolbar: Undo button failed to enable, trying aggressive refresh"));
		m_UndoButton->Enable(false);
		m_UndoButton->Update();
		m_UndoButton->Enable(true);
		m_UndoButton->Update();
	}
	
	// Update brush controls if we have a current brush
	TerrainBrush* currentBrush = m_TerrainEditor->getCurrentBrush();
	if (currentBrush)
	{
		// Update size and strength
		int sizeValue = (int)(currentBrush->getSize() * 10); // Convert to slider range
		int strengthValue = (int)(currentBrush->getStrength() * 100); // Convert to slider range
		
		m_SizeSlider->SetValue(sizeValue);
		m_StrengthSlider->SetValue(strengthValue);
		
		// Update falloff
		m_FalloffChoice->SetSelection((int)currentBrush->getFalloff());
		
		// Update brush type based on current brush
		TerrainBrush::BrushType brushType = currentBrush->getType();
		switch (brushType)
		{
			case TerrainBrush::BRUSH_RAISE:
				m_BrushTypeChoice->SetSelection(0);
				break;
			case TerrainBrush::BRUSH_LOWER:
				m_BrushTypeChoice->SetSelection(1);
				break;
			case TerrainBrush::BRUSH_FLATTEN:
				m_BrushTypeChoice->SetSelection(2);
				break;
			case TerrainBrush::BRUSH_SMOOTH:
				// Need to check smooth brush mode to determine selection
				SmoothBrush* smoothBrush = dynamic_cast<SmoothBrush*>(currentBrush);
				if (smoothBrush)
				{
					if (smoothBrush->getSmoothMode() == SmoothBrush::SMOOTH_GAUSSIAN)
						m_BrushTypeChoice->SetSelection(4);
					else
						m_BrushTypeChoice->SetSelection(3);
				}
				break;
		}
	}
	
	UpdateBrushPanels();
	UpdateLabels();
}

void TerrainToolbar::UpdateBrushPanels()
{
	if (!m_BrushTypeChoice)
		return;
		
	int selection = m_BrushTypeChoice->GetSelection();
	
	// Show/hide appropriate panels
	bool showHeight = (selection >= 0 && selection <= 2); // Height brushes
	bool showSmooth = (selection >= 3 && selection <= 4); // Smooth brushes
	
	m_HeightPanel->Show(showHeight);
	m_SmoothPanel->Show(showSmooth);
	
	Layout();
}

void TerrainToolbar::UpdateLabels()
{
	if (m_SizeSlider && m_SizeLabel)
	{
		float size = m_SizeSlider->GetValue() / 10.0f;
		m_SizeLabel->SetLabel(wxString::Format(_("%.1f"), size));
	}
	
	if (m_StrengthSlider && m_StrengthLabel)
	{
		float strength = m_StrengthSlider->GetValue() / 100.0f;
		m_StrengthLabel->SetLabel(wxString::Format(_("%.2f"), strength));
	}
	
	if (m_TargetHeightSlider && m_TargetHeightLabel)
	{
		float height = m_TargetHeightSlider->GetValue() / 10.0f;
		m_TargetHeightLabel->SetLabel(wxString::Format(_("%.1f"), height));
	}
	
	if (m_IterationsSlider && m_IterationsLabel)
	{
		int iterations = m_IterationsSlider->GetValue();
		m_IterationsLabel->SetLabel(wxString::Format(_("%d"), iterations));
	}
}

// Event handlers
void TerrainToolbar::OnClose(wxCloseEvent& event)
{
	// Hide instead of closing to keep the toolbar available
	Hide();
}

void TerrainToolbar::OnBrushTypeChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	int selection = event.GetSelection();
	
	// Switch to appropriate brush based on selection
	switch (selection)
	{
		case 0: // Height - Raise
			m_TerrainEditor->setCurrentBrush(0);
			break;
		case 1: // Height - Lower  
			m_TerrainEditor->setCurrentBrush(1);
			break;
		case 2: // Height - Flatten
			m_TerrainEditor->setCurrentBrush(2);
			break;
		case 3: // Smooth - Average
			m_TerrainEditor->setCurrentBrush(3);
			break;
		case 4: // Smooth - Gaussian
			m_TerrainEditor->setCurrentBrush(4);
			break;
	}
	
	UpdateBrushPanels();
}

void TerrainToolbar::OnSizeChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	float size = event.GetInt() / 10.0f;
	TerrainBrush* brush = m_TerrainEditor->getCurrentBrush();
	if (brush)
	{
		brush->setSize(size);
	}
	
	UpdateLabels();
}

void TerrainToolbar::OnStrengthChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	float strength = event.GetInt() / 100.0f;
	TerrainBrush* brush = m_TerrainEditor->getCurrentBrush();
	if (brush)
	{
		brush->setStrength(strength);
	}
	
	UpdateLabels();
}

void TerrainToolbar::OnFalloffChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	TerrainBrush::FalloffType falloff = (TerrainBrush::FalloffType)event.GetSelection();
	TerrainBrush* brush = m_TerrainEditor->getCurrentBrush();
	if (brush)
	{
		brush->setFalloff(falloff);
	}
}

void TerrainToolbar::OnModeToggle(wxCommandEvent& event)
{
	if (!m_ViewPanel)
		return;
		
	bool currentMode = m_ViewPanel->IsTerrainEditingMode();
	m_ViewPanel->SetTerrainEditingMode(!currentMode);
	
	// Update button text
	m_ModeToggleButton->SetLabel(currentMode ? _("Enable Terrain Editing") : _("Disable Terrain Editing"));
}

void TerrainToolbar::OnUndo(wxCommandEvent& event)
{
	if (m_TerrainEditor)
	{
		m_TerrainEditor->undo();
		UpdateFromTerrainEditor();
	}
}

void TerrainToolbar::OnRedo(wxCommandEvent& event)
{
	if (m_TerrainEditor)
	{
		m_TerrainEditor->redo();
		UpdateFromTerrainEditor();
	}
}

void TerrainToolbar::OnHeightModeChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	TerrainBrush* brush = m_TerrainEditor->getCurrentBrush();
	HeightBrush* heightBrush = dynamic_cast<HeightBrush*>(brush);
	if (heightBrush)
	{
		HeightBrush::HeightMode mode = (HeightBrush::HeightMode)event.GetSelection();
		heightBrush->setHeightMode(mode);
	}
}

void TerrainToolbar::OnTargetHeightChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	float height = event.GetInt() / 10.0f;
	TerrainBrush* brush = m_TerrainEditor->getCurrentBrush();
	HeightBrush* heightBrush = dynamic_cast<HeightBrush*>(brush);
	if (heightBrush)
	{
		heightBrush->setTargetHeight(height);
	}
	
	UpdateLabels();
}

void TerrainToolbar::OnAdaptiveChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	bool adaptive = event.IsChecked();
	TerrainBrush* brush = m_TerrainEditor->getCurrentBrush();
	HeightBrush* heightBrush = dynamic_cast<HeightBrush*>(brush);
	if (heightBrush)
	{
		heightBrush->setAdaptiveStrength(adaptive);
	}
}

void TerrainToolbar::OnSmoothModeChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	TerrainBrush* brush = m_TerrainEditor->getCurrentBrush();
	SmoothBrush* smoothBrush = dynamic_cast<SmoothBrush*>(brush);
	if (smoothBrush)
	{
		SmoothBrush::SmoothMode mode = (SmoothBrush::SmoothMode)event.GetSelection();
		smoothBrush->setSmoothMode(mode);
	}
}

void TerrainToolbar::OnIterationsChanged(wxCommandEvent& event)
{
	if (!m_TerrainEditor)
		return;
		
	int iterations = event.GetInt();
	TerrainBrush* brush = m_TerrainEditor->getCurrentBrush();
	SmoothBrush* smoothBrush = dynamic_cast<SmoothBrush*>(brush);
	if (smoothBrush)
	{
		smoothBrush->setIterations(iterations);
	}
	
	UpdateLabels();
}
