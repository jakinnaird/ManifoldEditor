/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "wx/filename.h"

#include "irrlicht.h"

#include <list>
#include <map>
#include <memory>

class ExplorerPanel;

class Map : public std::enable_shared_from_this<Map>
{
protected:
	wxFileName m_FileName;

	irr::scene::ISceneManager* m_SceneMgr;
	irr::scene::ISceneNode* m_MapRoot;

	wxInt32 m_NextId;

	typedef std::map<wxString, irr::io::IAttributes*> entities_t;
	entities_t m_Entities;

	bool m_Lighting;

public:
	Map(void);
	Map(const wxFileName& fileName);
	~Map(void);

	void SetSceneMgr(irr::scene::ISceneManager* sceneMgr);
	irr::scene::ISceneManager* GetSceneMgr(void);

	bool HasFilename(void);
	const wxFileName& GetFileName(void);

	void Save(const wxFileName& fileName);
	void Load(irr::scene::ISceneNode* mapRoot, ExplorerPanel* explorerPanel);

	wxString NextName(const wxString& base);

	void AddEntity(const wxString& name, irr::io::IAttributes* attribs);
	void RemoveEntity(const wxString& name);

	void RecomputeLighting(bool lighting);
	bool IsLighting(void);

	irr::io::IAttributes* GetAttributes(const wxString& entityName);
};
