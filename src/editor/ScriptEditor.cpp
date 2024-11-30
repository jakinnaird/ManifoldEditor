/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "ProjectEditor.hpp"
#include "ScriptEditor.hpp"

#include "wx/sizer.h"
#include "wx/textdlg.h"

#define DEFAULT_LANGUAGE "<default>"

//! general style types
#define mySTC_TYPE_DEFAULT 0

#define mySTC_TYPE_WORD1 1
#define mySTC_TYPE_WORD2 2
#define mySTC_TYPE_WORD3 3
#define mySTC_TYPE_WORD4 4
#define mySTC_TYPE_WORD5 5
#define mySTC_TYPE_WORD6 6

#define mySTC_TYPE_COMMENT 7
#define mySTC_TYPE_COMMENT_DOC 8
#define mySTC_TYPE_COMMENT_LINE 9
#define mySTC_TYPE_COMMENT_SPECIAL 10

#define mySTC_TYPE_CHARACTER 11
#define mySTC_TYPE_CHARACTER_EOL 12
#define mySTC_TYPE_STRING 13
#define mySTC_TYPE_STRING_EOL 14

#define mySTC_TYPE_DELIMITER 15

#define mySTC_TYPE_PUNCTUATION 16

#define mySTC_TYPE_OPERATOR 17

#define mySTC_TYPE_BRACE 18

#define mySTC_TYPE_COMMAND 19
#define mySTC_TYPE_IDENTIFIER 20
#define mySTC_TYPE_LABEL 21
#define mySTC_TYPE_NUMBER 22
#define mySTC_TYPE_PARAMETER 23
#define mySTC_TYPE_REGEX 24
#define mySTC_TYPE_UUID 25
#define mySTC_TYPE_VALUE 26

#define mySTC_TYPE_PREPROCESSOR 27
#define mySTC_TYPE_SCRIPT 28

#define mySTC_TYPE_ERROR 29

//----------------------------------------------------------------------------
//! style bits types
#define mySTC_STYLE_BOLD 1
#define mySTC_STYLE_ITALIC 2
#define mySTC_STYLE_UNDERL 4
#define mySTC_STYLE_HIDDEN 8

//----------------------------------------------------------------------------
//! general folding types
#define mySTC_FOLD_COMMENT 1
#define mySTC_FOLD_COMPACT 2
#define mySTC_FOLD_PREPROC 4

#define mySTC_FOLD_HTML 16
#define mySTC_FOLD_HTMLPREP 32

#define mySTC_FOLD_COMMENTPY 64
#define mySTC_FOLD_QUOTESPY 128

//----------------------------------------------------------------------------
//! flags
#define mySTC_FLAG_WRAPMODE 16

enum {
	// menu IDs
	myID_PROPERTIES = wxID_HIGHEST,
	myID_EDIT_FIRST,
	myID_INDENTINC = myID_EDIT_FIRST,
	myID_INDENTRED,
	myID_FINDNEXT,
	myID_REPLACE,
	myID_REPLACENEXT,
	myID_BRACEMATCH,
	myID_GOTO,
	myID_DISPLAYEOL,
	myID_INDENTGUIDE,
	myID_LINENUMBER,
	myID_LONGLINEON,
	myID_WHITESPACE,
	myID_FOLDTOGGLE,
	myID_OVERTYPE,
	myID_READONLY,
	myID_WRAPMODEON,
	myID_ANNOTATION_ADD,
	myID_ANNOTATION_REMOVE,
	myID_ANNOTATION_CLEAR,
	myID_ANNOTATION_STYLE_HIDDEN,
	myID_ANNOTATION_STYLE_STANDARD,
	myID_ANNOTATION_STYLE_BOXED,
	myID_CHANGECASE,
	myID_CHANGELOWER,
	myID_CHANGEUPPER,
	myID_HIGHLIGHTLANG,
	myID_HIGHLIGHTFIRST,
	myID_HIGHLIGHTLAST = myID_HIGHLIGHTFIRST + 99,
	myID_CONVERTEOL,
	myID_CONVERTCR,
	myID_CONVERTCRLF,
	myID_CONVERTLF,
	myID_MULTIPLE_SELECTIONS,
	myID_MULTI_PASTE,
	myID_MULTIPLE_SELECTIONS_TYPING,
	myID_TECHNOLOGY_DEFAULT,
	myID_TECHNOLOGY_DIRECTWRITE,
	myID_CUSTOM_POPUP,
	myID_USECHARSET,
	myID_CHARSETANSI,
	myID_CHARSETMAC,
	myID_SELECTLINE,
	myID_EDIT_LAST = myID_SELECTLINE,
	myID_WINDOW_MINIMAL,

	// other IDs
	myID_ABOUTTIMER,
};

//----------------------------------------------------------------------------
// CommonInfo

struct CommonInfo {
	// editor functionality prefs
	bool syntaxEnable;
	bool foldEnable;
	bool indentEnable;
	// display defaults prefs
	bool readOnlyInitial;
	bool overTypeInitial;
	bool wrapModeInitial;
	bool displayEOLEnable;
	bool indentGuideEnable;
	bool lineNumberEnable;
	bool longLineOnEnable;
	bool whiteSpaceEnable;
};

const CommonInfo g_CommonPrefs = {
	// editor functionality prefs
	true,  // syntaxEnable
	true,  // foldEnable
	true,  // indentEnable
	// display defaults prefs
	false, // overTypeInitial
	false, // readOnlyInitial
	false,  // wrapModeInitial
	false, // displayEOLEnable
	false, // IndentGuideEnable
	true,  // lineNumberEnable
	false, // longLineOnEnable
	false, // whiteSpaceEnable
};

//----------------------------------------------------------------------------
// LanguageInfo

//----------------------------------------------------------------------------
// keywordlists
// JavaScript
const char* JsWordlist1 =
	"asm auto bool break case catch char class const const_cast "
	"continue default delete do double dynamic_cast else enum explicit "
	"export extern false float for friend goto if inline int long "
	"mutable namespace new operator private protected public register "
	"reinterpret_cast return short signed sizeof static static_cast "
	"struct switch template this throw true try typedef typeid "
	"typename union unsigned using virtual void volatile wchar_t "
	"while";
const char* JsWordlist2 =
	"file";
const char* JsWordlist3 =
	"a addindex addtogroup anchor arg attention author b brief bug c "
	"class code date def defgroup deprecated dontinclude e em endcode "
	"endhtmlonly endif endlatexonly endlink endverbatim enum example "
	"exception f$ f[ f] file fn hideinitializer htmlinclude "
	"htmlonly if image include ingroup internal invariant interface "
	"latexonly li line link mainpage name namespace nosubgrouping note "
	"overload p page par param post pre ref relates remarks return "
	"retval sa section see showinitializer since skip skipline struct "
	"subsection test throw todo typedef union until var verbatim "
	"verbinclude version warning weakgroup $ @ \"\" & < > # { }";

const LanguageInfo g_LanguagePrefs[] = {
	// JavaScript
	{"JavaScript",
	 "*.js",
	 wxSTC_LEX_CPP,
	 {{mySTC_TYPE_DEFAULT, NULL},
	  {mySTC_TYPE_COMMENT, NULL},
	  {mySTC_TYPE_COMMENT_LINE, NULL},
	  {mySTC_TYPE_COMMENT_DOC, NULL},
	  {mySTC_TYPE_NUMBER, NULL},
	  {mySTC_TYPE_WORD1, JsWordlist1}, // KEYWORDS
	  {mySTC_TYPE_STRING, NULL},
	  {mySTC_TYPE_CHARACTER, NULL},
	  {mySTC_TYPE_UUID, NULL},
	  {mySTC_TYPE_PREPROCESSOR, NULL},
	  {mySTC_TYPE_OPERATOR, NULL},
	  {mySTC_TYPE_IDENTIFIER, NULL},
	  {mySTC_TYPE_STRING_EOL, NULL},
	  {mySTC_TYPE_DEFAULT, NULL}, // VERBATIM
	  {mySTC_TYPE_REGEX, NULL},
	  {mySTC_TYPE_COMMENT_SPECIAL, NULL}, // DOXY
	  {mySTC_TYPE_WORD2, JsWordlist2}, // EXTRA WORDS
	  {mySTC_TYPE_WORD3, JsWordlist3}, // DOXY KEYWORDS
	  {mySTC_TYPE_ERROR, NULL}, // KEYWORDS ERROR
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL}},
	 mySTC_FOLD_COMMENT | mySTC_FOLD_COMPACT | mySTC_FOLD_PREPROC},

	 {wxTRANSLATE(DEFAULT_LANGUAGE),
	 "*.*",
	 wxSTC_LEX_PROPERTIES,
	 {{mySTC_TYPE_DEFAULT, NULL},
	  {mySTC_TYPE_DEFAULT, NULL},
	  {mySTC_TYPE_DEFAULT, NULL},
	  {mySTC_TYPE_DEFAULT, NULL},
	  {mySTC_TYPE_DEFAULT, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL},
	  {-1, NULL}},
	 0},
};

const int g_LanguagePrefsSize = WXSIZEOF(g_LanguagePrefs);

//----------------------------------------------------------------------------
// StyleInfo
struct StyleInfo {
	const wxString name;
	const wxString foreground;
	const wxString background;
	const wxString fontname;
	int fontsize;
	int fontstyle;
	int lettercase;
};

const StyleInfo g_StylePrefs[] =
{
	// mySTC_TYPE_DEFAULT
	{"Default",
	"BLACK", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_WORD1
	{"Keyword1",
	"BLUE", "WHITE",
	"", 10, mySTC_STYLE_BOLD, 0},

	// mySTC_TYPE_WORD2
	{"Keyword2",
	"MIDNIGHT BLUE", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_WORD3
	{"Keyword3",
	"CORNFLOWER BLUE", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_WORD4
	{"Keyword4",
	"CYAN", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_WORD5
	{"Keyword5",
	"DARK GREY", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_WORD6
	{"Keyword6",
	"GREY", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_COMMENT
	{"Comment",
	"FOREST GREEN", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_COMMENT_DOC
	{"Comment (Doc)",
	"FOREST GREEN", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_COMMENT_LINE
	{"Comment line",
	"FOREST GREEN", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_COMMENT_SPECIAL
	{"Special comment",
	"FOREST GREEN", "WHITE",
	"", 10, mySTC_STYLE_ITALIC, 0},

	// mySTC_TYPE_CHARACTER
	{"Character",
	"KHAKI", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_CHARACTER_EOL
	{"Character (EOL)",
	"KHAKI", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_STRING
	{"String",
	"BROWN", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_STRING_EOL
	{"String (EOL)",
	"BROWN", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_DELIMITER
	{"Delimiter",
	"ORANGE", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_PUNCTUATION
	{"Punctuation",
	"ORANGE", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_OPERATOR
	{"Operator",
	"BLACK", "WHITE",
	"", 10, mySTC_STYLE_BOLD, 0},

	// mySTC_TYPE_BRACE
	{"Label",
	"VIOLET", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_COMMAND
	{"Command",
	"BLUE", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_IDENTIFIER
	{"Identifier",
	"BLACK", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_LABEL
	{"Label",
	"VIOLET", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_NUMBER
	{"Number",
	"SIENNA", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_PARAMETER
	{"Parameter",
	"VIOLET", "WHITE",
	"", 10, mySTC_STYLE_ITALIC, 0},

	// mySTC_TYPE_REGEX
	{"Regular expression",
	"ORCHID", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_UUID
	{"UUID",
	"ORCHID", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_VALUE
	{"Value",
	"ORCHID", "WHITE",
	"", 10, mySTC_STYLE_ITALIC, 0},

	// mySTC_TYPE_PREPROCESSOR
	{"Preprocessor",
	"GREY", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_SCRIPT
	{"Script",
	"DARK GREY", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_ERROR
	{"Error",
	"RED", "WHITE",
	"", 10, 0, 0},

	// mySTC_TYPE_UNDEFINED
	{"Undefined",
	"ORANGE", "WHITE",
	"", 10, 0, 0}
};

const int g_StylePrefsSize = WXSIZEOF(g_StylePrefs);

const int ANNOTATION_STYLE = wxSTC_STYLE_LASTPREDEFINED + 1;

const char* hashtag_xpm[] = {
"10 10 2 1",
"  c None",
". c #BD08F9",
"  ..  ..  ",
"  ..  ..  ",
"..........",
"..........",
"  ..  ..  ",
"  ..  ..  ",
"..........",
"..........",
"  ..  ..  ",
"  ..  ..  " };

ScriptEditor::ScriptEditor(wxWindow* parent, wxMenu* editMenu, 
	const wxFileName& fileName)
	: EditorPage(parent, editMenu),
	m_FileName(fileName)
{
	m_TextCtrl = new wxStyledTextCtrl(this);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_TextCtrl, wxSizerFlags(1).Expand());

	SetSizerAndFit(sizer);

	// default font for all styles
	m_TextCtrl->SetViewEOL(g_CommonPrefs.displayEOLEnable);
	m_TextCtrl->SetIndentationGuides(g_CommonPrefs.indentGuideEnable);
	m_TextCtrl->SetEdgeMode(g_CommonPrefs.longLineOnEnable ?
		wxSTC_EDGE_LINE : wxSTC_EDGE_NONE);
	m_TextCtrl->SetViewWhiteSpace(g_CommonPrefs.whiteSpaceEnable ?
		wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
	m_TextCtrl->SetOvertype(g_CommonPrefs.overTypeInitial);
	m_TextCtrl->SetReadOnly(g_CommonPrefs.readOnlyInitial);
	m_TextCtrl->SetWrapMode(g_CommonPrefs.wrapModeInitial ?
		wxSTC_WRAP_WORD : wxSTC_WRAP_NONE);
	wxFont font(wxFontInfo(10).Family(wxFONTFAMILY_MODERN));
	m_TextCtrl->StyleSetFont(wxSTC_STYLE_DEFAULT, font);
	m_TextCtrl->StyleSetForeground(wxSTC_STYLE_DEFAULT, *wxBLACK);
	m_TextCtrl->StyleSetBackground(wxSTC_STYLE_DEFAULT, *wxWHITE);
	m_TextCtrl->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour("DARK GREY"));
	m_TextCtrl->StyleSetBackground(wxSTC_STYLE_LINENUMBER, *wxWHITE);
	m_TextCtrl->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, wxColour("DARK GREY"));
	InitializePreferences(DEFAULT_LANGUAGE);

	// set visibility
	m_TextCtrl->SetVisiblePolicy(wxSTC_VISIBLE_STRICT | wxSTC_VISIBLE_SLOP, 1);
	m_TextCtrl->SetXCaretPolicy(wxSTC_CARET_EVEN | wxSTC_VISIBLE_STRICT | wxSTC_CARET_SLOP, 1);
	m_TextCtrl->SetYCaretPolicy(wxSTC_CARET_EVEN | wxSTC_VISIBLE_STRICT | wxSTC_CARET_SLOP, 1);

	// markers
	m_TextCtrl->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_DOTDOTDOT, "BLACK", "BLACK");
	m_TextCtrl->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_ARROWDOWN, "BLACK", "BLACK");
	m_TextCtrl->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY, "BLACK", "BLACK");
	m_TextCtrl->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_DOTDOTDOT, "BLACK", "WHITE");
	m_TextCtrl->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_ARROWDOWN, "BLACK", "WHITE");
	m_TextCtrl->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY, "BLACK", "BLACK");
	m_TextCtrl->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY, "BLACK", "BLACK");

	// annotations
	m_TextCtrl->AnnotationSetVisible(wxSTC_ANNOTATION_BOXED);

	// autocompletion
	wxBitmap bmp(hashtag_xpm);
	m_TextCtrl->RegisterImage(0, bmp);

	// call tips
	m_TextCtrl->CallTipSetBackground(*wxYELLOW);
	m_calltipNo = 1;

	// miscellaneous
	m_LineNrMargin = m_TextCtrl->TextWidth(wxSTC_STYLE_LINENUMBER, "_999999");
	m_FoldingMargin = m_TextCtrl->FromDIP(16);
	m_TextCtrl->SetLayoutCache(wxSTC_CACHE_PAGE);
	m_TextCtrl->UsePopUp(wxSTC_POPUP_ALL);

	InitializePreferences(DeterminePreferences(m_FileName.GetFullName()));

	m_TextCtrl->Bind(wxEVT_MENU, &ScriptEditor::OnEditCut, this, wxID_CUT);
	m_TextCtrl->Bind(wxEVT_MENU, &ScriptEditor::OnEditCopy, this, wxID_COPY);
	m_TextCtrl->Bind(wxEVT_MENU, &ScriptEditor::OnEditPaste, this, wxID_PASTE);
	m_TextCtrl->Bind(wxEVT_STC_MARGINCLICK, &ScriptEditor::OnMarginClick, this);
	m_TextCtrl->Bind(wxEVT_STC_CHARADDED, &ScriptEditor::OnCharAdded, this);
	m_TextCtrl->Bind(wxEVT_STC_CALLTIP_CLICK, &ScriptEditor::OnCallTipClick, this);
	m_TextCtrl->Bind(wxEVT_KEY_DOWN, &ScriptEditor::OnKeyDown, this);
	m_TextCtrl->Bind(wxEVT_SIZE, &ScriptEditor::OnSize, this);

	if (m_FileName.FileExists())
		m_TextCtrl->LoadFile(m_FileName.GetFullPath());
}

ScriptEditor::~ScriptEditor(void)
{
}

bool ScriptEditor::InitializePreferences(const wxString& name)
{
	// initialize styles
	m_TextCtrl->StyleClearAll();
	const LanguageInfo* curInfo = NULL;

	// determine language
	bool found = false;
	int languageNr;
	for (languageNr = 0; languageNr < g_LanguagePrefsSize; languageNr++)
	{
		curInfo = &g_LanguagePrefs[languageNr];
		if (curInfo->name == name)
		{
			found = true;
			break;
		}
	}
	if (!found)
		return false;

	// set lexer and language
	m_TextCtrl->SetLexer(curInfo->lexer);
	m_Language = curInfo;

	// set margin for line numbers
	m_TextCtrl->SetMarginType(m_LineNrID, wxSTC_MARGIN_NUMBER);
	m_TextCtrl->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour("DARK GREY"));
	m_TextCtrl->StyleSetBackground(wxSTC_STYLE_LINENUMBER, *wxWHITE);
	m_TextCtrl->SetMarginWidth(m_LineNrID, 0); // start out not visible

	// annotations style
	m_TextCtrl->StyleSetBackground(ANNOTATION_STYLE, wxColour(244, 220, 220));
	m_TextCtrl->StyleSetForeground(ANNOTATION_STYLE, *wxBLACK);
	m_TextCtrl->StyleSetSizeFractional(ANNOTATION_STYLE,
		(m_TextCtrl->StyleGetSizeFractional(wxSTC_STYLE_DEFAULT) * 4) / 5);

	// default fonts for all styles!
	int Nr;
	for (Nr = 0; Nr < wxSTC_STYLE_LASTPREDEFINED; Nr++)
	{
		wxFont font(wxFontInfo(10).Family(wxFONTFAMILY_MODERN));
		m_TextCtrl->StyleSetFont(Nr, font);
	}

	// set common styles
	m_TextCtrl->StyleSetForeground(wxSTC_STYLE_DEFAULT, wxColour("DARK GREY"));
	m_TextCtrl->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, wxColour("DARK GREY"));

	// initialize settings
	if (g_CommonPrefs.syntaxEnable)
	{
		int keywordnr = 0;
		for (Nr = 0; Nr < STYLE_TYPES_COUNT; Nr++)
		{
			if (curInfo->styles[Nr].type == -1)
				continue;
			const StyleInfo& curType = g_StylePrefs[curInfo->styles[Nr].type];
			wxFont font(wxFontInfo(curType.fontsize)
				.Family(wxFONTFAMILY_MODERN)
				.FaceName(curType.fontname));
			m_TextCtrl->StyleSetFont(Nr, font);
			if (curType.foreground.length())
				m_TextCtrl->StyleSetForeground(Nr, wxColour(curType.foreground));
			if (curType.background.length())
				m_TextCtrl->StyleSetBackground(Nr, wxColour(curType.background));
			m_TextCtrl->StyleSetBold(Nr, (curType.fontstyle & mySTC_STYLE_BOLD) > 0);
			m_TextCtrl->StyleSetItalic(Nr, (curType.fontstyle & mySTC_STYLE_ITALIC) > 0);
			m_TextCtrl->StyleSetUnderline(Nr, (curType.fontstyle & mySTC_STYLE_UNDERL) > 0);
			m_TextCtrl->StyleSetVisible(Nr, (curType.fontstyle & mySTC_STYLE_HIDDEN) == 0);
			m_TextCtrl->StyleSetCase(Nr, curType.lettercase);
			const char* pwords = curInfo->styles[Nr].words;
			if (pwords)
			{
				m_TextCtrl->SetKeyWords(keywordnr, pwords);
				keywordnr += 1;
			}
		}
	}

	// set margin as unused
	m_TextCtrl->SetMarginType(m_DividerID, wxSTC_MARGIN_SYMBOL);
	m_TextCtrl->SetMarginWidth(m_DividerID, 0);
	m_TextCtrl->SetMarginSensitive(m_DividerID, false);

	// folding
	m_TextCtrl->SetMarginType(m_FoldingID, wxSTC_MARGIN_SYMBOL);
	m_TextCtrl->SetMarginMask(m_FoldingID, wxSTC_MASK_FOLDERS);
	m_TextCtrl->StyleSetBackground(m_FoldingID, *wxWHITE);
	m_TextCtrl->SetMarginWidth(m_FoldingID, 0);
	m_TextCtrl->SetMarginSensitive(m_FoldingID, false);
	if (g_CommonPrefs.foldEnable)
	{
		m_TextCtrl->SetMarginWidth(m_FoldingID, curInfo->folds != 0 ? m_FoldingMargin : 0);
		m_TextCtrl->SetMarginSensitive(m_FoldingID, curInfo->folds != 0);
		m_TextCtrl->SetProperty("fold", curInfo->folds != 0 ? "1" : "0");
		m_TextCtrl->SetProperty("fold.comment",
			(curInfo->folds & mySTC_FOLD_COMMENT) > 0 ? "1" : "0");
		m_TextCtrl->SetProperty("fold.compact",
			(curInfo->folds & mySTC_FOLD_COMPACT) > 0 ? "1" : "0");
		m_TextCtrl->SetProperty("fold.preprocessor",
			(curInfo->folds & mySTC_FOLD_PREPROC) > 0 ? "1" : "0");
		m_TextCtrl->SetProperty("fold.html",
			(curInfo->folds & mySTC_FOLD_HTML) > 0 ? "1" : "0");
		m_TextCtrl->SetProperty("fold.html.preprocessor",
			(curInfo->folds & mySTC_FOLD_HTMLPREP) > 0 ? "1" : "0");
		m_TextCtrl->SetProperty("fold.comment.python",
			(curInfo->folds & mySTC_FOLD_COMMENTPY) > 0 ? "1" : "0");
		m_TextCtrl->SetProperty("fold.quotes.python",
			(curInfo->folds & mySTC_FOLD_QUOTESPY) > 0 ? "1" : "0");
	}
	m_TextCtrl->SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED |
		wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

	// set spaces and indentation
	m_TextCtrl->SetTabWidth(4);
	m_TextCtrl->SetUseTabs(false);
	m_TextCtrl->SetTabIndents(true);
	m_TextCtrl->SetBackSpaceUnIndents(true);
	m_TextCtrl->SetIndent(g_CommonPrefs.indentEnable ? 4 : 0);

	// others
	m_TextCtrl->SetViewEOL(g_CommonPrefs.displayEOLEnable);
	m_TextCtrl->SetIndentationGuides(g_CommonPrefs.indentGuideEnable);
	m_TextCtrl->SetEdgeColumn(80);
	m_TextCtrl->SetEdgeMode(g_CommonPrefs.longLineOnEnable ? wxSTC_EDGE_LINE : wxSTC_EDGE_NONE);
	m_TextCtrl->SetViewWhiteSpace(g_CommonPrefs.whiteSpaceEnable ?
		wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
	m_TextCtrl->SetOvertype(g_CommonPrefs.overTypeInitial);
	m_TextCtrl->SetReadOnly(g_CommonPrefs.readOnlyInitial);
	m_TextCtrl->SetWrapMode(g_CommonPrefs.wrapModeInitial ?
		wxSTC_WRAP_WORD : wxSTC_WRAP_NONE);

	return true;
}

wxString ScriptEditor::DeterminePreferences(const wxString& fileName)
{
	LanguageInfo const* curInfo;

	// determine language from filepatterns
	int languageNr;
	for (languageNr = 0; languageNr < g_LanguagePrefsSize; languageNr++)
	{
		curInfo = &g_LanguagePrefs[languageNr];
		wxString filepattern = curInfo->filepattern;
		filepattern.Lower();
		while (!filepattern.empty())
		{
			wxString cur = filepattern.BeforeFirst(';');
			if ((cur == fileName) ||
				(cur == (fileName.BeforeLast('.') + ".*")) ||
				(cur == ("*." + fileName.AfterLast('.'))))
			{
				return curInfo->name;
			}

			filepattern = filepattern.AfterFirst(';');
		}
	}

	return wxEmptyString;
}

void ScriptEditor::OnCut(void)
{
	if (m_TextCtrl->GetReadOnly() ||
		(m_TextCtrl->GetSelectionEnd() - m_TextCtrl->GetSelectionStart() <= 0))
		return;

	m_TextCtrl->Cut();
}

void ScriptEditor::OnCopy(void)
{
	if (m_TextCtrl->GetSelectionEnd() - m_TextCtrl->GetSelectionStart() <= 0)
		return;

	m_TextCtrl->Copy();
}

void ScriptEditor::OnPaste(void)
{
	if (!m_TextCtrl->CanPaste())
		return;

	m_TextCtrl->Paste();
}

void ScriptEditor::OnSize(wxSizeEvent& event)
{
	int x = GetClientSize().x +
		(g_CommonPrefs.lineNumberEnable ? m_LineNrMargin : 0) +
		(g_CommonPrefs.foldEnable ? m_FoldingMargin : 0);
	if (x > 0)
		m_TextCtrl->SetScrollWidth(x);
	event.Skip();
}

void ScriptEditor::OnKeyDown(wxKeyEvent& event)
{
	if (m_TextCtrl->CallTipActive())
		m_TextCtrl->CallTipCancel();
	if (event.GetKeyCode() == WXK_SPACE && event.ControlDown() && event.ShiftDown())
	{
		// Show our first call tip at the current position of the caret.
		m_calltipNo = 1;
		ShowCallTipAt(m_TextCtrl->GetCurrentPos());
		return;
	}
	event.Skip();
}

void ScriptEditor::OnEditCut(wxCommandEvent& WXUNUSED(event))
{
	OnCut();
}

void ScriptEditor::OnEditCopy(wxCommandEvent& WXUNUSED(event))
{
	OnCopy();
}

void ScriptEditor::OnEditPaste(wxCommandEvent& WXUNUSED(event))
{
	OnPaste();
}

void ScriptEditor::OnFind(wxCommandEvent& WXUNUSED(event))
{
}

void ScriptEditor::OnFindNext(wxCommandEvent& WXUNUSED(event))
{
}

void ScriptEditor::OnReplace(wxCommandEvent& WXUNUSED(event))
{
}

void ScriptEditor::OnReplaceNext(wxCommandEvent& WXUNUSED(event))
{
}

void ScriptEditor::OnBraceMatch(wxCommandEvent& WXUNUSED(event))
{
	int min = m_TextCtrl->GetCurrentPos();
	int max = m_TextCtrl->BraceMatch(min);
	if (max > (min + 1))
	{
		m_TextCtrl->BraceHighlight(min + 1, max);
		m_TextCtrl->SetSelection(min + 1, max);
	}
	else
	{
		m_TextCtrl->BraceBadLight(min);
	}
}

void ScriptEditor::OnEditIndentInc(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->CmdKeyExecute(wxSTC_CMD_TAB);
}

void ScriptEditor::OnEditIndentRed(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->CmdKeyExecute(wxSTC_CMD_DELETEBACK);
}

void ScriptEditor::OnEditSelectAll(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetSelection(0, m_TextCtrl->GetTextLength());
}

void ScriptEditor::OnEditSelectLine(wxCommandEvent& WXUNUSED(event))
{
	int lineStart = m_TextCtrl->PositionFromLine(m_TextCtrl->GetCurrentLine());
	int lineEnd = m_TextCtrl->PositionFromLine(m_TextCtrl->GetCurrentLine() + 1);
	m_TextCtrl->SetSelection(lineStart, lineEnd);
}

void ScriptEditor::OnHighlightLang(wxCommandEvent& event)
{
	InitializePreferences(g_LanguagePrefs[event.GetId() - myID_HIGHLIGHTFIRST].name);
}

void ScriptEditor::OnDisplayEOL(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetViewEOL(!m_TextCtrl->GetViewEOL());
}

void ScriptEditor::OnIndentGuide(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetIndentationGuides(!m_TextCtrl->GetIndentationGuides());
}

void ScriptEditor::OnLineNumber(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetMarginWidth(m_LineNrID,
		m_TextCtrl->GetMarginWidth(m_LineNrID) == 0 ? m_LineNrMargin : 0);
}

void ScriptEditor::OnLongLineOn(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetEdgeMode(m_TextCtrl->GetEdgeMode() == 0 ? wxSTC_EDGE_LINE : wxSTC_EDGE_NONE);
}

void ScriptEditor::OnWhiteSpace(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetViewWhiteSpace(m_TextCtrl->GetViewWhiteSpace() == 0 ?
		wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
}

void ScriptEditor::OnFoldToggle(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->ToggleFold(m_TextCtrl->GetFoldParent(m_TextCtrl->GetCurrentLine()));
}

void ScriptEditor::OnSetOverType(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetOvertype(!m_TextCtrl->GetOvertype());
}

void ScriptEditor::OnSetReadOnly(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetReadOnly(!m_TextCtrl->GetReadOnly());
}

void ScriptEditor::OnWrapmodeOn(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->SetWrapMode(m_TextCtrl->GetWrapMode() == 0 ? wxSTC_WRAP_WORD : wxSTC_WRAP_NONE);
}

void ScriptEditor::OnUseCharset(wxCommandEvent& event)
{
	int Nr;
	int charset = m_TextCtrl->GetCodePage();
	switch (event.GetId())
	{
	case myID_CHARSETANSI: { charset = wxSTC_CHARSET_ANSI; break; }
	case myID_CHARSETMAC: { charset = wxSTC_CHARSET_ANSI; break; }
	}

	for (Nr = 0; Nr < wxSTC_STYLE_LASTPREDEFINED; Nr++)
	{
		m_TextCtrl->StyleSetCharacterSet(Nr, charset);
	}
	
	m_TextCtrl->SetCodePage(charset);
}

void ScriptEditor::OnAnnotationAdd(wxCommandEvent& WXUNUSED(event))
{
	const int line = m_TextCtrl->GetCurrentLine();

	wxString ann = m_TextCtrl->AnnotationGetText(line);
	ann = wxGetTextFromUser
	(
		wxString::Format("Enter annotation for the line %d", line),
		"Edit annotation",
		ann,
		this
	);
	if (ann.empty())
		return;

	m_TextCtrl->AnnotationSetText(line, ann);
	m_TextCtrl->AnnotationSetStyle(line, ANNOTATION_STYLE);

	// Scintilla doesn't update the scroll width for annotations, even with
	// scroll width tracking on, so do it manually.
	const int width = m_TextCtrl->GetScrollWidth();

	// NB: The following adjustments are only needed when using
	//     wxSTC_ANNOTATION_BOXED annotations style, but we apply them always
	//     in order to make things simpler and not have to redo the width
	//     calculations when the annotations visibility changes. In a real
	//     program you'd either just stick to a fixed annotations visibility or
	//     update the width when it changes.

	// Take into account the fact that the annotation is shown indented, with
	// the same indent as the line it's attached to.
	int indent = m_TextCtrl->GetLineIndentation(line);

	// This is just a hack to account for the width of the box, there doesn't
	// seem to be any way to get it directly from Scintilla.
	indent += 3;

	const int widthAnn = m_TextCtrl->TextWidth(ANNOTATION_STYLE, ann + wxString(indent, ' '));

	if (widthAnn > width)
		m_TextCtrl->SetScrollWidth(widthAnn);
}

void ScriptEditor::OnAnnotationRemove(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->AnnotationSetText(m_TextCtrl->GetCurrentLine(), wxString());
}

void ScriptEditor::OnAnnotationClear(wxCommandEvent& WXUNUSED(event))
{
	m_TextCtrl->AnnotationClearAll();
}

void ScriptEditor::OnAnnotationStyle(wxCommandEvent& event)
{
	int style = 0;
	switch (event.GetId())
	{
	case myID_ANNOTATION_STYLE_HIDDEN:
		style = wxSTC_ANNOTATION_HIDDEN;
		break;

	case myID_ANNOTATION_STYLE_STANDARD:
		style = wxSTC_ANNOTATION_STANDARD;
		break;

	case myID_ANNOTATION_STYLE_BOXED:
		style = wxSTC_ANNOTATION_BOXED;
		break;
	}

	m_TextCtrl->AnnotationSetVisible(style);
}

void ScriptEditor::OnChangeCase(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case myID_CHANGELOWER:
	{
		m_TextCtrl->CmdKeyExecute(wxSTC_CMD_LOWERCASE);
		break;
	}
	case myID_CHANGEUPPER:
	{
		m_TextCtrl->CmdKeyExecute(wxSTC_CMD_UPPERCASE);
		break;
	}
	}
}

void ScriptEditor::OnConvertEOL(wxCommandEvent& event)
{
	int eolMode = m_TextCtrl->GetEOLMode();
	switch (event.GetId())
	{
	case myID_CONVERTCR: { eolMode = wxSTC_EOL_CR; break; }
	case myID_CONVERTCRLF: { eolMode = wxSTC_EOL_CRLF; break; }
	case myID_CONVERTLF: { eolMode = wxSTC_EOL_LF; break; }
	}
	
	m_TextCtrl->ConvertEOLs(eolMode);
	m_TextCtrl->SetEOLMode(eolMode);
}

void ScriptEditor::OnMarginClick(wxStyledTextEvent& event)
{
	if (event.GetMargin() == 2)
	{
		int lineClick = m_TextCtrl->LineFromPosition(event.GetPosition());
		int levelClick = m_TextCtrl->GetFoldLevel(lineClick);
		if ((levelClick & wxSTC_FOLDLEVELHEADERFLAG) > 0)
		{
			m_TextCtrl->ToggleFold(lineClick);
		}
	}
}

void ScriptEditor::OnCharAdded(wxStyledTextEvent& event)
{
	char chr = (char)event.GetKey();
	int currentLine = m_TextCtrl->GetCurrentLine();
	// Change this if support for mac files with \r is needed
	if (chr == '\n')
	{
		int lineInd = 0;
		if (currentLine > 0)
		{
			lineInd = m_TextCtrl->GetLineIndentation(currentLine - 1);
		}
		
		if (lineInd == 0)
			return;
		
		m_TextCtrl->SetLineIndentation(currentLine, lineInd);
		m_TextCtrl->GotoPos(m_TextCtrl->PositionFromLine(currentLine) + lineInd);
	}
	else if (chr == '#')
	{
		wxString s = "define?0 elif?0 else?0 endif?0 error?0 if?0 ifdef?0 "
			"ifndef?0 include?0 line?0 pragma?0 undef?0";
		m_TextCtrl->AutoCompShow(0, s);
	}
}

void ScriptEditor::OnCallTipClick(wxStyledTextEvent& event)
{
	if (event.GetPosition() == 1)
	{
		// If position=1, the up arrow has been clicked. Show the next tip.
		m_calltipNo = m_calltipNo == 3 ? 1 : (m_calltipNo + 1);
		ShowCallTipAt(m_TextCtrl->CallTipPosAtStart());
	}
	else if (event.GetPosition() == 2)
	{
		// If position=2, the down arrow has been clicked. Show previous tip.
		m_calltipNo = m_calltipNo == 1 ? 3 : (m_calltipNo - 1);
		ShowCallTipAt(m_TextCtrl->CallTipPosAtStart());
	}
}

void ScriptEditor::ShowCallTipAt(int position)
{
	// In a call tip string, the character '\001' will become a clickable
	// up arrow and '\002' will become a clickable down arrow.
	wxString ctString = wxString::Format("\001 %d of 3 \002 ", m_calltipNo);
	if (m_calltipNo == 1)
		ctString += "This is a call tip. Try clicking the up or down buttons.";
	else if (m_calltipNo == 2)
		ctString += "It is meant to be a context sensitive popup helper for "
		"the user.";
	else
		ctString += "This is a call tip with multiple lines.\n"
		"You can provide slightly longer help with "
		"call tips like these.";

	if (m_TextCtrl->CallTipActive())
		m_TextCtrl->CallTipCancel();
	m_TextCtrl->CallTipShow(position, ctString);
}
