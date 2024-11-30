/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Common.hpp"
#include "ExplorerPanel.hpp"
#include "MainWindow.hpp"
#include "ViewPanel.hpp"

#include "wx/sizer.h"

ExplorerPanel::ExplorerPanel(wxWindow* parent, wxCommandProcessor& cmdProc,
	BrowserWindow* browser)
	: wxPanel(parent), m_Commands(cmdProc), m_Browser(browser), m_ViewPanel(nullptr),
	  m_Explorer(nullptr), m_SceneMgr(nullptr), m_Changing(false)
{
	m_Explorer = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxTR_HAS_BUTTONS | wxTR_MULTIPLE);
	m_Root = m_Explorer->AddRoot(_("untitled"));
	m_GeometryRoot = m_Explorer->InsertItem(m_Root, 0, _("Geometry"));
	m_ActorRoot = m_Explorer->InsertItem(m_Root, 1, _("Actors"));
	m_Explorer->Expand(m_Root);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_Explorer, wxSizerFlags(1).Expand());
	this->SetSizerAndFit(sizer);

	Bind(wxEVT_TREE_SEL_CHANGED, &ExplorerPanel::OnSelectionChanged, this);
	Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &ExplorerPanel::OnItemRightClick, this);
}

ExplorerPanel::~ExplorerPanel(void)
{
}

void ExplorerPanel::SetViewPanel(ViewPanel* viewPanel)
{
	m_ViewPanel = viewPanel;
	Bind(wxEVT_MENU, &ViewPanel::OnEditCut, m_ViewPanel, wxID_CUT);
	Bind(wxEVT_MENU, &ViewPanel::OnEditCopy, m_ViewPanel, wxID_COPY);
	Bind(wxEVT_MENU, &ViewPanel::OnEditPaste, m_ViewPanel, wxID_PASTE);
	Bind(wxEVT_MENU, &ViewPanel::OnEditDelete, m_ViewPanel, wxID_DELETE);

	Bind(wxEVT_MENU, &ViewPanel::OnMenuSetTexture, m_ViewPanel, MENU_SETTEXTURE);
}

void ExplorerPanel::SetSceneManager(irr::scene::ISceneManager* sceneMgr)
{
	m_SceneMgr = sceneMgr;
}

void ExplorerPanel::SetMapName(const wxString& name)
{
	m_Explorer->SetItemText(m_Root, name);
}

void ExplorerPanel::Clear(void)
{
	m_Explorer->DeleteChildren(m_GeometryRoot);
	m_Explorer->DeleteChildren(m_ActorRoot);
}

void ExplorerPanel::SelectItem(const wxString& name)
{
	wxTreeItemId item = FindItem(name, m_Root);
	if (item.IsOk())
	{
		m_Explorer->SelectItem(item);
		m_Explorer->EnsureVisible(item);
	}
}

void ExplorerPanel::UnselectItem(const wxString& name)
{
	wxTreeItemId item = FindItem(name, m_Root);
	if (item.IsOk())
		m_Explorer->UnselectItem(item);
}

void ExplorerPanel::UnselectAll(void)
{
	if (m_Changing)
		return;

	m_Explorer->UnselectAll();
}

void ExplorerPanel::AddGeometry(const wxString& name)
{
	m_Explorer->AppendItem(m_GeometryRoot, name);
	m_Explorer->SortChildren(m_GeometryRoot);
	m_Explorer->Expand(m_GeometryRoot);
}

void ExplorerPanel::RemoveGeometry(const wxString& name)
{
	wxTreeItemId item = FindItem(name, m_GeometryRoot);
	if (item.IsOk())
		m_Explorer->Delete(item);
}

bool ExplorerPanel::IsGeometry(const wxString& name)
{
	wxTreeItemId item = FindItem(name, m_GeometryRoot);
	if (item.IsOk())
		return true;

	return false;
}

void ExplorerPanel::AddActor(const wxString& name)
{
	m_Explorer->AppendItem(m_ActorRoot, name);
	m_Explorer->SortChildren(m_ActorRoot);
	m_Explorer->Expand(m_ActorRoot);
}

void ExplorerPanel::RemoveActor(const wxString& name)
{
	wxTreeItemId item = FindItem(name, m_ActorRoot);
	if (item.IsOk())
		m_Explorer->Delete(item);
}

bool ExplorerPanel::IsActor(const wxString& name)
{
	wxTreeItemId item = FindItem(name, m_ActorRoot);
	if (item.IsOk())
		return true;

	return false;
}

wxTreeItemId ExplorerPanel::FindItem(const wxString& name, wxTreeItemId& start)
{
	wxTreeItemIdValue cookie;
	wxTreeItemId item = m_Explorer->GetFirstChild(start, cookie);
	while (item.IsOk())
	{
		wxString itemName = m_Explorer->GetItemText(item);
		if (itemName == name)
			return item;

		item = m_Explorer->GetNextSibling(item);
	}

	return wxTreeItemId(); // not found
}

void ExplorerPanel::OnSelectionChanged(wxTreeEvent& event)
{
	m_Changing = true;

	m_ViewPanel->ClearSelection();

	wxArrayTreeItemIds selection;
	size_t count = m_Explorer->GetSelections(selection);
	for (size_t i = 0; i < count; ++i)
	{
		irr::scene::ISceneNode* node = m_SceneMgr->getSceneNodeFromName(
			m_Explorer->GetItemText(selection[i]).c_str());
		if (node)
			m_ViewPanel->AddToSelection(node, true);
	}

	m_Changing = false;
}

void ExplorerPanel::OnItemRightClick(wxTreeEvent& event)
{
	// make sure it's not a control item
	wxTreeItemId item = event.GetItem();
	if (item == m_Root ||
		item == m_GeometryRoot ||
		item == m_ActorRoot)
		return;

	// popup menu
	wxMenu popupMenu;
	popupMenu.Append(wxID_CUT);
	popupMenu.Append(wxID_COPY);
	popupMenu.Append(wxID_PASTE);
	popupMenu.Append(wxID_DELETE);

	const wxString& texture = m_Browser->GetTexture();
	if (!texture.empty())
	{
		popupMenu.AppendSeparator();
		popupMenu.Append(MENU_SETTEXTURE, wxString::Format(_("Apply texture: %s"),
			texture));
	}

	PopupMenu(&popupMenu);
}
