/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
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

class PropertyPanel : public wxPanel
{
private:
	wxCommandProcessor& m_Commands;
	wxToolBar* m_ToolBar;
	wxPropertyGrid* m_Properties;

	irr::scene::ISceneNode* m_SceneNode;

	wxFloatProperty* m_PosX;
	wxFloatProperty* m_PosY;
	wxFloatProperty* m_PosZ;

	std::shared_ptr<Map> m_Map;

public:
	PropertyPanel(wxWindow* parent, wxCommandProcessor& cmdProc);
	~PropertyPanel(void);

	void SetMap(std::shared_ptr<Map>& map);

	void Clear(void);
	void Refresh(void);

	void SetSceneNode(irr::scene::ISceneNode* node);

private:
	void OnToolAdd(wxCommandEvent& event);
	void OnToolRemove(wxCommandEvent& event);
	void OnValueChanging(wxPropertyGridEvent& event);
	void OnValueChanged(wxPropertyGridEvent& event);
};
