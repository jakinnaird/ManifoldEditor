/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "BrowserWindow.hpp"
#include "ExplorerPanel.hpp"
#include "Map.hpp"
#include "PropertyPanel.hpp"

#include <wx/cmdproc.h>
#include <wx/cursor.h>
#include <wx/panel.h>
#include <wx/timer.h>

#include "irrlicht.h"
#include "CGridSceneNode.h"
#include "CSceneNodeAnimatorCameraOrtho.h"

#include <list>
#include <memory>

class ViewPanel : public wxPanel
{
	friend class PropertyPanel;

private:
	enum VIEW
	{
		VIEW_FRONT,	// top-left
		VIEW_TOP,	// top-right
		VIEW_RIGHT, // bottom-left
		VIEW_3D		// bottom-right
	};

	enum CURSORS
	{
		CURSOR_MOVE,
		CURSOR_ROTATE,

		NUM_CURSORS
	};

private:
	wxTimer m_RefreshTimer;
	wxCommandProcessor& m_Commands;
	BrowserWindow* m_Browser;
	ExplorerPanel* m_ExplorerPanel;
	PropertyPanel* m_PropertyPanel;
	wxCursor* m_Cursor[NUM_CURSORS];

	irr::IrrlichtDevice* m_RenderDevice;
	irr::video::SExposedVideoData m_VideoData;

	bool m_Init;
	VIEW m_ActiveView;
	bool m_FreeLook;

	irr::scene::ISceneNode* m_EditorRoot;
	irr::scene::ISceneNode* m_MapRoot;
	irr::scene::IBillboardSceneNode* m_Camera;

	irr::scene::ICameraSceneNode* m_View[4];
	irr::scene::CSceneNodeAnimatorCameraOrtho* m_Ortho[3];
	irr::scene::ISceneNodeAnimatorCameraFPS* m_3DCam;
	CGridSceneNode* m_Grid[4];
	irr::gui::IGUIStaticText* m_Label[4];

	std::shared_ptr<Map> m_Map;
	
	typedef std::list<irr::scene::ISceneNode*> selection_t;
	selection_t m_Selection;
	irr::core::aabbox3df m_SelectionBox;

	wxPoint m_LastMousePos;
	bool m_TranslatingSelection;

public:
	ViewPanel(wxWindow* parent, wxCommandProcessor& cmdProc,
		BrowserWindow* browserWindow,
		ExplorerPanel* explorerPanel, PropertyPanel* propertyPanel);
	~ViewPanel(void);

	irr::io::IFileSystem* GetFileSystem(void);
	int GetFPS(void);

	void SetMap(std::shared_ptr<Map>& map);

	void AddToSelection(irr::scene::ISceneNode* node, bool append);
	void UpdateSelectionBoundingBox(void);
	void ShowSelection(bool show); // used when saving the map
	void ClearSelection(void);
	void DeleteSelection(void);

	void BeginFreeLook(void);
	void EndFreeLook(void);

private:
	void OnTimer(wxTimerEvent& event);
	void OnResize(wxSizeEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnMouse(wxMouseEvent& event);
	void OnKey(wxKeyEvent& event);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
	void OnMouseCaptureChanged(wxMouseCaptureChangedEvent& event);

	void BuildPathLinks(void);

public:
	void OnToolCube(wxCommandEvent& event);
	void OnToolCylinder(wxCommandEvent& event);
	void OnToolSphere(wxCommandEvent& event);
	void OnToolPlane(wxCommandEvent& event);
	void OnToolTerrain(wxCommandEvent& event);
	void OnToolSkybox(wxCommandEvent& event);
	void OnToolPlayerStart(wxCommandEvent& event);
	void OnToolLight(wxCommandEvent& event);
	void OnToolPathNode(wxCommandEvent& event);

	void OnEditCut(wxCommandEvent& event);
	void OnEditCopy(wxCommandEvent& event);
	void OnEditPaste(wxCommandEvent& event);
	void OnEditDelete(wxCommandEvent& event);

	void OnMenuAlignTop(wxCommandEvent& event);
	void OnMenuAlignMiddle(wxCommandEvent& event);
	void OnMenuAlignBottom(wxCommandEvent& event);

	void OnMenuFreeLook(wxCommandEvent& event);
	void OnMenuSetTexture(wxCommandEvent& event);
};
