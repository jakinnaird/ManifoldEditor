/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/filename.h>
#include <wx/stc/stc.h>

class EditorPage;

// based on the wxWidgets STC sample

#define STYLE_TYPES_COUNT 32
struct LanguageInfo {
	const char* name;
	const char* filepattern;
	int lexer;
	struct {
		int type;
		const char* words;
	} styles[STYLE_TYPES_COUNT];
	int folds;
};

class ScriptEditor : public EditorPage
{
private:
	wxStyledTextCtrl* m_TextCtrl;
	wxFileName m_FileName;

	// language properties
	const LanguageInfo* m_Language;

	// margin variables
	int m_LineNrID;
	int m_LineNrMargin;
	int m_FoldingID;
	int m_FoldingMargin;
	int m_DividerID;

	// call tip data
	int m_calltipNo;

public:
	ScriptEditor(wxWindow* parent, wxMenu* editMenu, const wxFileName& fileName);
	virtual ~ScriptEditor(void);

	bool HasChanged(void) { return m_TextCtrl->IsModified(); }
	void Save(void)
	{
		m_TextCtrl->SaveFile(m_FileName.GetFullPath());
		m_TextCtrl->SetSavePoint();
	}
	void OnUndo(void) { m_TextCtrl->Undo(); }
	void OnRedo(void) { m_TextCtrl->Redo(); }

	void OnCut(void);
	void OnCopy(void);
	void OnPaste(void);

private:
	bool InitializePreferences(const wxString& name);
	wxString DeterminePreferences(const wxString& fileName);

private:
	void OnSize(wxSizeEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	void OnEditCut(wxCommandEvent& event);
	void OnEditCopy(wxCommandEvent& event);
	void OnEditPaste(wxCommandEvent& event);

	void OnFind(wxCommandEvent& event);
	void OnFindNext(wxCommandEvent& event);
	void OnReplace(wxCommandEvent& event);
	void OnReplaceNext(wxCommandEvent& event);
	void OnBraceMatch(wxCommandEvent& event);
	void OnEditIndentInc(wxCommandEvent& event);
	void OnEditIndentRed(wxCommandEvent& event);
	void OnEditSelectAll(wxCommandEvent& event);
	void OnEditSelectLine(wxCommandEvent& event);
	void OnHighlightLang(wxCommandEvent& event);
	void OnDisplayEOL(wxCommandEvent& event);
	void OnIndentGuide(wxCommandEvent& event);
	void OnLineNumber(wxCommandEvent& event);
	void OnLongLineOn(wxCommandEvent& event);
	void OnWhiteSpace(wxCommandEvent& event);
	void OnFoldToggle(wxCommandEvent& event);
	void OnSetOverType(wxCommandEvent& event);
	void OnSetReadOnly(wxCommandEvent& event);
	void OnWrapmodeOn(wxCommandEvent& event);
	void OnUseCharset(wxCommandEvent& event);
	void OnAnnotationAdd(wxCommandEvent& event);
	void OnAnnotationRemove(wxCommandEvent& event);
	void OnAnnotationClear(wxCommandEvent& event);
	void OnAnnotationStyle(wxCommandEvent& event);
	void OnChangeCase(wxCommandEvent& event);
	void OnConvertEOL(wxCommandEvent& event);
	void OnMarginClick(wxStyledTextEvent& event);
	void OnCharAdded(wxStyledTextEvent& event);
	void OnCallTipClick(wxStyledTextEvent& event);
	void ShowCallTipAt(int position);
};
