/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <wx/defs.h>

#define APP_NAME    "Manifold Editor"
#define APP_VERSION "0.1.0"

#define SAFE_DELETE(x) if (x) { delete x; x = nullptr; }
#define SAFE_UNREF(x) if (x) { x->UnRef(); x = nullptr; }

enum TOOLID : int
{
    TOOL_CUBE = wxID_HIGHEST + 100, // don't collide with existing
    TOOL_CYLINDER,
    TOOL_SPHERE,
    TOOL_PLANE,
    TOOL_TERRAIN,
    TOOL_SKYBOX, // actually a sky dome, but who's asking?  
    TOOL_PLAYERSTART,
    TOOL_LIGHT,
    TOOL_PATHNODE,
    TOOL_ACTOR,
    TOOL_MESH,
    TOOL_IRRLICHT_ID, // used to determine if the TOOLID is an Irrlicht ID

    TOOL_PACKAGEMANAGER,
    TOOL_BROWSER,
    TOOL_ACTORBROWSER,
    TOOL_TEXTUREBROWSER,
    TOOL_SOUNDBROWSER,
    TOOL_MESHBROWSER,
    TOOL_CALCLIGHTING,
    TOOL_PLAYMAP,

    MENU_NEW_MAP,
    MENU_NEW_PROJECT,
    MENU_OPEN_MAP,
    MENU_OPEN_PROJECT,

    MENU_ALIGNTOP,
    MENU_ALIGNMIDDLE,
    MENU_ALIGNBOTTOM,

    MENU_SETTEXTURE,
    MENU_FREELOOK,

    MENU_BUILDPROJECT,
    MENU_CLEANPROJECT,
    MENU_BUILDPACKAGE,
    MENU_CLEANPACKAGE,
    MENU_NEWPACKAGE,
    MENU_NEWMAP,
    MENU_ADDNEWFILE,
    MENU_ADDEXISTINGFILE,
    MENU_ADDFILTER,
    MENU_OPENFILE,
    MENU_REMOVE,
    MENU_RENAME,
    MENU_PROPERTIES,

    MENU_PLAYSOUND,
    MENU_STOPSOUND,
};

enum NODEID : int
{
    NID_NONE = 0,
    NID_PICKABLE = 1 << 0,
    NID_NOSAVE = 1 << 1,
};
