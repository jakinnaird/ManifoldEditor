/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "wx/choice.h"
#include "wx/dialog.h"
#include "wx/filepicker.h"
#include "wx/process.h"
#include "wx/textctrl.h"

class MapEditor;

class PlayProcess : public wxProcess
{
private:
	MapEditor* m_Parent;

public:
	PlayProcess(MapEditor* parent);
	~PlayProcess(void);

	bool ProcessRedirect(void);

	void OnTerminate(int pid, int status) wxOVERRIDE;
};

class PlayLauncher : public wxDialog
{
private:
	wxFilePickerCtrl* m_Executable;
	wxTextCtrl* m_Params;

public:
	PlayLauncher(wxWindow* parent);
	~PlayLauncher(void);

	wxString GetGameExe(void);
	wxString GetParams(void);
};
