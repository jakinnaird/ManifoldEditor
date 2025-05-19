/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "Common.hpp"
#include "ExplorerPanel.hpp"
#include "Map.hpp"

#include <wx/cmdproc.h>
#include "irrlicht.h"

#include <list>
#include <memory>

class AddNodeCommand : public wxCommand
{
protected:
	TOOLID m_ToolId;
	ExplorerPanel* m_ExplorerPanel;
	irr::scene::ISceneManager* m_SceneMgr;
	irr::scene::ISceneNode* m_MapRoot;
	std::shared_ptr<Map> m_Map;
	irr::core::vector3df m_Position;
	wxString m_Name;
	wxString m_Actor;
	
public:
	AddNodeCommand(TOOLID toolId,
		ExplorerPanel* explorerPanel,
		irr::scene::ISceneManager* sceneMgr,
		irr::scene::ISceneNode* mapRoot,
		std::shared_ptr<Map>& map,
		const irr::core::vector3df& position,
		const wxString& name);
	AddNodeCommand(const wxString& nodeType,
		ExplorerPanel* explorerPanel,
		irr::scene::ISceneManager* sceneMgr,
		irr::scene::ISceneNode* mapRoot,
		std::shared_ptr<Map>& map,
		const wxString& name);
	virtual ~AddNodeCommand(void);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class TranslateNodeCommand : public wxCommand
{
public:
	typedef std::list<wxString> selection_t;

private:
	irr::scene::ISceneManager* m_SceneMgr;
	selection_t m_Selection;
	irr::core::vector3df m_Delta;

public:
	TranslateNodeCommand(irr::scene::ISceneManager* sceneMgr, 
		const selection_t& selection, const irr::core::vector3df& start);
	TranslateNodeCommand(irr::scene::ISceneNode* node,
		const irr::core::vector3df& start, const irr::core::vector3df& end);
	virtual ~TranslateNodeCommand(void);

	void Update(const irr::core::vector3df& delta);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class RotateNodeCommand : public wxCommand
{
public:
	typedef std::list<wxString> selection_t;

private:
	irr::scene::ISceneManager* m_SceneMgr;
	selection_t m_Selection;
	irr::core::vector3df m_Delta;

public:
	//RotateNodeCommand(irr::scene::ISceneManager* sceneMgr,
	//	irr::scene::ISceneNode* parent, const selection_t& selection/*,
	//	const irr::core::vector3df& start*/);
	RotateNodeCommand(irr::scene::ISceneNode* node,
		const irr::core::vector3df& start, const irr::core::vector3df& end);
	virtual ~RotateNodeCommand(void);

	void Update(const irr::core::vector3df& delta);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class ScaleNodeCommand : public wxCommand
{
public:
	typedef std::list<wxString> selection_t;

private:
	irr::scene::ISceneManager* m_SceneMgr;
	selection_t m_Selection;
	irr::core::vector3df m_Delta;

public:
	//ScaleNodeCommand(irr::scene::ISceneManager* sceneMgr,
	//	irr::scene::ISceneNode* parent, const selection_t& selection/*,
	//	const irr::core::vector3df& start*/);
	ScaleNodeCommand(irr::scene::ISceneNode* node,
		const irr::core::vector3df& start, const irr::core::vector3df& end);
	virtual ~ScaleNodeCommand(void);

	void Update(const irr::core::vector3df& delta);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class ResizeNodeCommand : public wxCommand
{
private:
	irr::scene::ISceneManager* m_SceneMgr;
	wxString m_Name;
	irr::core::vector3df m_Size;
	irr::core::dimension2df m_TileSize;
	irr::core::dimension2du m_TileCount;

public:
	ResizeNodeCommand(irr::scene::ISceneNode* node,
		const irr::core::vector3df& newSize);
	ResizeNodeCommand(irr::scene::ISceneNode* node,
		const irr::core::dimension2df& newSize,
		const irr::core::dimension2du& newCount);
	virtual ~ResizeNodeCommand(void);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class ChangeColorCommand : public wxCommand
{
public:
	enum COLOR_TYPE
	{
		CT_AMBIENT,
		CT_DIFFUSE,
		CT_EMISSIVE,
		CT_SPECULAR,
		CT_SHINY
	};

private:
	irr::scene::ISceneManager* m_SceneMgr;
	COLOR_TYPE m_Type;
	wxString m_Name;
	irr::u32 m_Material;
	irr::video::SColorf m_Color;
	irr::f32 m_Shiny;

public:
	ChangeColorCommand(COLOR_TYPE type, irr::scene::ISceneNode* node,
		irr::u32 material, const irr::video::SColorf& color);
	ChangeColorCommand(COLOR_TYPE type, irr::scene::ISceneNode* node,
		irr::u32 material, const irr::f32& shiny);
	virtual ~ChangeColorCommand(void);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class ChangeTextureCommand : public wxCommand
{
public:
	typedef std::list<wxString> selection_t;

private:
	irr::scene::ISceneManager* m_SceneMgr;
	selection_t m_Selection;
	irr::u32 m_Material;
	irr::u32 m_TextureId;

	typedef std::map<wxString, wxString> texturemap_t;
	texturemap_t m_Textures;

public:
	ChangeTextureCommand(irr::scene::ISceneNode* node,
		irr::u32 material, irr::u32 textureId, const wxString& texture);
	ChangeTextureCommand(irr::scene::ISceneManager* sceneMgr,
		const selection_t& selection, irr::u32 material, irr::u32 textureId, 
		const wxString& texture);
	virtual ~ChangeTextureCommand(void);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class AlignNodeCommand : public wxCommand
{
public:
	typedef std::list<wxString> selection_t;

	enum ALIGN_TYPE : int
	{
		ALIGN_TOP,
		ALIGN_MIDDLE,
		ALIGN_BOTTOM
	};

private:
	irr::scene::ISceneManager* m_SceneMgr;
	selection_t m_Selection;
	ALIGN_TYPE m_Type;

	typedef std::map<wxString, irr::core::vector3df> positions_t;
	positions_t m_OldPosition;

public:
	AlignNodeCommand(irr::scene::ISceneManager* sceneMgr,
		const selection_t& selection, ALIGN_TYPE type);
	virtual ~AlignNodeCommand(void);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class DeleteNodeCommand : public wxCommand
{
public:
	typedef std::list<wxString> selection_t;

private:
	ExplorerPanel* m_ExplorerPanel;
	irr::scene::ISceneManager* m_SceneMgr;
	irr::scene::ISceneNode* m_MapRoot;
	std::shared_ptr<Map> m_Map;

	selection_t m_Selection;

	typedef std::map<wxString, irr::io::IAttributes*> itemmap_t;
	itemmap_t m_Geometry;
	itemmap_t m_Actors;
	itemmap_t m_Materials;

	typedef std::map<wxString, irr::scene::ESCENE_NODE_TYPE> typemap_t;
	typemap_t m_Type;

public:
	DeleteNodeCommand(ExplorerPanel* explorerPanel, 
		irr::scene::ISceneManager* sceneMgr,
		irr::scene::ISceneNode* mapRoot,
		std::shared_ptr<Map>& map,
		const selection_t& selection);
	virtual ~DeleteNodeCommand(void);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};

class UpdatePathNameCommand : public wxCommand
{
private:
	irr::scene::ISceneManager* m_SceneMgr;
	wxString m_PathNode;
	wxString m_PathName;

public:
	UpdatePathNameCommand(irr::scene::ISceneManager* sceneMgr,
		const wxString& pathNode, const wxString& pathName);
	virtual ~UpdatePathNameCommand(void);

	bool CanUndo(void) const;
	bool Do(void);
	wxString GetName(void) const;
	bool Undo(void);
};
