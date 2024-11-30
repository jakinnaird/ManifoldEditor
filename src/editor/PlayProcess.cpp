/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "MainWindow.hpp"
#include "PlayProcess.hpp"

#include "wx/confbase.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/stdpaths.h"
#include "wx/txtstrm.h"
#include "wx/wfstream.h"
#include "wx/zipstrm.h"

PlayProcess::PlayProcess(MapEditor* parent)
	: wxProcess(wxPROCESS_REDIRECT),
	m_Parent(parent)
{
}

PlayProcess::~PlayProcess(void)
{
}

bool PlayProcess::ProcessRedirect(void)
{
	bool hasData = false;

	if (IsInputAvailable())
	{
		wxTextInputStream tis(*GetInputStream());

		wxLogMessage(tis.ReadLine());

		hasData = true;
	}

	if (IsErrorAvailable())
	{
		wxTextInputStream tis(*GetErrorStream());

		wxLogMessage(tis.ReadLine());

		hasData = true;
	}

	return hasData;
}

void PlayProcess::OnTerminate(int pid, int status)
{
	while (ProcessRedirect());

	m_Parent->PlayProcessTerminated();

	wxProcess::OnTerminate(pid, status);
}

PlayLauncher::PlayLauncher(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, _("Play Map"))
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->SetMinSize(320, 200);

	wxString exePath = wxConfigBase::Get()->Read(wxT("/Editor/Launcher"), wxT("mecc"));
	wxString params = wxConfigBase::Get()->Read(wxT("/Editor/LaunchParams"),
		wxT("%mappath%"));

	wxStaticBoxSizer* gameExe = new wxStaticBoxSizer(wxHORIZONTAL, this,
		_("Game Executable"));
	wxStaticBox* staticBox = gameExe->GetStaticBox();
	m_Executable = new wxFilePickerCtrl((wxWindow*)staticBox, 100,
		exePath, _("Game Executable"), _("Executable (*.exe)|*.exe"),
		wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST | wxFLP_USE_TEXTCTRL);
	gameExe->Add(m_Executable, wxSizerFlags(3));

	wxStaticBoxSizer* paramsSizer = new wxStaticBoxSizer(wxHORIZONTAL, this,
		_("Additional Options"));
	staticBox = paramsSizer->GetStaticBox();
	m_Params = new wxTextCtrl((wxWindow*)staticBox, wxID_ANY,
		params, wxDefaultPosition, wxDefaultSize);
	paramsSizer->Add(m_Params, wxSizerFlags(1));

	sizer->Add(gameExe, wxSizerFlags().Expand());
	sizer->Add(paramsSizer, wxSizerFlags().Expand());
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Expand());

	SetSizerAndFit(sizer);
}

PlayLauncher::~PlayLauncher(void)
{
}

wxString PlayLauncher::GetGameExe(void)
{
	return m_Executable->GetFileName().GetFullPath();
}

wxString PlayLauncher::GetParams(void)
{
	return m_Params->GetValue();
}
