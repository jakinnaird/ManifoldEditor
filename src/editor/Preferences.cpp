/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Preferences.hpp"

#include <wx/config.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/stdpaths.h>

wxDEFINE_EVENT(ME_CONFIGCHANGED, wxCommandEvent);

PreferencesWindow::PreferencesWindow(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, _("Preferences"))
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->SetMinSize(640, 480);

	m_Properties = new wxPropertyGridManager(this, wxID_ANY,
		wxDefaultPosition, wxDefaultSize,
		wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER |
		wxPG_TOOLBAR | wxPGMAN_DEFAULT_STYLE);

	// add all the options
	wxConfigBase* config = wxConfigBase::Get();
	wxPropertyGridPage* generalPage = m_Properties->AddPage(_("General"));

	// add all paths
	generalPage->Append(new wxPropertyCategory("Paths"));

	wxString entry;
	long cookie;
	wxConfigPathChanger paths(config, wxT("/Paths/"));
	if (config->GetFirstEntry(entry, cookie))
	{
		do
		{
			wxString path(wxT("/Paths/"));
			path.append(entry);

			wxDirProperty* prop = new wxDirProperty(entry, path, config->Read(entry));
			prop->SetAttribute(wxPG_FILE_SHOW_FULL_PATH, false);
			generalPage->Append(prop);
		} while (config->GetNextEntry(entry, cookie));
	}

	sizer->Add(m_Properties, wxSizerFlags(9).Expand());
	sizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxAPPLY),
		wxSizerFlags(1).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_BUTTON, &PreferencesWindow::OnButtonApply, this, wxID_APPLY);
	Bind(wxEVT_PG_CHANGING, &PreferencesWindow::OnValueChanging, this);
	Bind(wxEVT_PG_CHANGED, &PreferencesWindow::OnValueChanged, this);

	m_Changed = false;
}

PreferencesWindow::~PreferencesWindow(void)
{
}

void PreferencesWindow::ApplyChanges(void)
{
	if (m_Changed)
	{
		wxConfigBase* config = wxConfigBase::Get();

		wxPGVIterator propIter = m_Properties->GetVIterator(wxPG_ITERATE_DEFAULT);
		while (!propIter.AtEnd())
		{
			wxPGProperty* prop = propIter.GetProperty();
			if (prop)
				config->Write(prop->GetName(), prop->GetValueAsString());

			propIter.Next();
		}

		// emit the change event
		wxCommandEvent event(ME_CONFIGCHANGED);
		GetParent()->ProcessWindowEvent(event);

		m_Changed = false;
	}
}

void PreferencesWindow::OnButtonApply(wxCommandEvent& event)
{
	ApplyChanges();
}

void PreferencesWindow::OnValueChanging(wxPropertyGridEvent& event)
{
	wxString propName(event.GetPropertyName());
	//if (propName.StartsWith(wxT("/Paths/")))
	//{
	//}
}

void PreferencesWindow::OnValueChanged(wxPropertyGridEvent& event)
{
	wxString propName(event.GetPropertyName());
	if (propName.StartsWith(wxT("/Paths/")))
	{
		wxFileName path(event.GetPropertyValue(), wxT(""));

#if defined(__WXMSW__)
		// updating the path to be relative to the editor executable
		wxFileName basePath(wxStandardPaths::Get().GetExecutablePath());
		path.MakeRelativeTo(basePath.GetPath());
#endif

		event.GetProperty()->SetValue(path.GetPath(1, wxPATH_UNIX));
	}

	m_Changed = true;
}
