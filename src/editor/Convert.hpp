/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include <irrlicht.h>
#include <wx/string.h>

irr::core::vector2df valueToVec2(const wxString& value);
irr::core::vector3df valueToVec3(const wxString& value);
irr::core::dimension2df valueToDim2df(const wxString& value);
irr::core::dimension2du valueToDim2du(const wxString& value);
irr::video::SColor valueToColor(const wxString& value);
irr::f32 valueToFloat(const wxString& value);
irr::s32 valueToInt(const wxString& value);
bool valueToBool(const wxString& value);
