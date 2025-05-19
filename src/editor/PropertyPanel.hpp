/**
 * @file PropertyPanel.hpp
 * @brief Property panel implementation for the Manifold Editor
 * @author James Kinnaird
 * @copyright Copyright (c) 2023 James Kinnaird
 */

#pragma once

#include <wx/cmdproc.h>
#include <wx/panel.h>
#include <wx/propgrid/propgrid.h>
#include <wx/toolbar.h>

#include "Map.hpp"

#include "irrlicht.h"

#include <list>
#include <map>
#include <vector>

class ViewPanel;

/**
 * @class PropertyPanel
 * @brief Panel class for object property editing
 * 
 * The PropertyPanel class provides a property grid panel for editing
 * object properties. It supports various property types and real-time
 * property updates.
 */
class PropertyPanel : public wxPanel
{
private:
	wxCommandProcessor& m_Commands;
	wxToolBar* m_ToolBar;
	wxPropertyGrid* m_Properties;
	wxPGProperty* m_GeneralProperties;
	wxPGProperty* m_CustomProperties;

	irr::scene::ISceneNode* m_SceneNode;

	wxFloatProperty* m_PosX;
	wxFloatProperty* m_PosY;
	wxFloatProperty* m_PosZ;

	std::shared_ptr<Map> m_Map;

public:
	/**
	 * @brief Constructor for the PropertyPanel class
	 * @param parent Pointer to the parent window
	 * @param cmdProc Reference to the command processor
	 */
	PropertyPanel(wxWindow* parent, wxCommandProcessor& cmdProc);
	
	/**
	 * @brief Destructor
	 */
	~PropertyPanel(void);

	/**
	 * @brief Set the map for the property panel
	 * @param map Reference to the map
	 */
	void SetMap(std::shared_ptr<Map>& map);

	/**
	 * @brief Clear the property panel
	 */
	void Clear(void);

	/**
	 * @brief Refresh the property panel
	 */
	void Refresh(void);

	/**
	 * @brief Set the scene node for the property panel
	 * @param node Pointer to the scene node
	 */
	void SetSceneNode(irr::scene::ISceneNode* node);

private:
	/**
	 * @brief Handle tool add events
	 * @param event The command event
	 */
	void OnToolAdd(wxCommandEvent& event);

	/**
	 * @brief Handle tool remove events
	 * @param event The command event
	 */
	void OnToolRemove(wxCommandEvent& event);

	/**
	 * @brief Handle value changing events
	 * @param event The property grid event
	 */
	void OnValueChanging(wxPropertyGridEvent& event);

	/**
	 * @brief Handle value changed events
	 * @param event The property grid event
	 */
	void OnValueChanged(wxPropertyGridEvent& event);
};
