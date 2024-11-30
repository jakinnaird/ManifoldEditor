/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "wx/dialog.h"
#include "wx/propgrid/manager.h"

class PreferencesWindow : public wxDialog
{
public:
	enum PageNumbers : int
	{
		PAGE_GENERAL,
	};

private:
	wxPropertyGridManager* m_Properties;
	bool m_Changed;

public:
	PreferencesWindow(wxWindow* parent);
	~PreferencesWindow(void);

	void ApplyChanges(void);

private:
	void OnButtonApply(wxCommandEvent& event);
	void OnValueChanging(wxPropertyGridEvent& event);
	void OnValueChanged(wxPropertyGridEvent& event);
};

wxDECLARE_EVENT(ME_CONFIGCHANGED, wxCommandEvent);
