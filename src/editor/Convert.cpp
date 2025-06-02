/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Convert.hpp"
#include <sstream>

irr::core::vector2df valueToVec2(const wxString& value)
{
	std::istringstream iss(value.utf8_string());
	std::string token;

	// X
	double _x = 0;
	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToDouble(&_x);

	// Y
	double _y = 0;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToDouble(&_y);

	return irr::core::vector2df(_x, _y);
}

irr::core::vector3df valueToVec3(const wxString& value)
{
	std::istringstream iss(value.utf8_string());
	std::string token;

	// X
	double _x = 0;
	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToDouble(&_x);

	// Y
	double _y = 0;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToDouble(&_y);

	// Z
	double _z = 0;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToDouble(&_z);

	return irr::core::vector3df(_x, _y, _z);
}

irr::core::dimension2df valueToDim2df(const wxString& value)
{
	irr::core::dimension2df result;

	std::istringstream iss(value.utf8_string());
	std::string token;

	double _value = 0;
	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToDouble(&_value);
	result.Width = _value;

	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToDouble(&_value);
	result.Height = _value;

	return result;
}

irr::core::dimension2du valueToDim2du(const wxString& value)
{
	irr::core::dimension2du result;

	std::istringstream iss(value.utf8_string());
	std::string token;

	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToUInt(&result.Width);

	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToUInt(&result.Height);

	return result;
}

irr::video::SColor valueToColor(const wxString& value)
{
	std::istringstream iss(value.utf8_string());
	std::string token;

	irr::u32 alpha = 255;
	std::getline(iss, token, ';');
	wxString _token(token);
	_token.ToUInt(&alpha);

	irr::u32 red = 255;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToUInt(&red);

	irr::u32 green = 255;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToUInt(&green);

	irr::u32 blue = 255;
	std::getline(iss, token, ';');
	_token.assign(token);
	_token.ToUInt(&blue);

	return irr::video::SColor(alpha, red, green, blue);
}

irr::f32 valueToFloat(const wxString& value)
{
	irr::f32 result;
	if (value.ToDouble((double*)&result))
		return result;

	return 0;
}

irr::s32 valueToInt(const wxString& value)
{
	irr::s32 result;
	if (value.ToInt(&result))
		return result;

	return 0;
}

bool valueToBool(const wxString& value)
{
	if (value.CmpNoCase(wxT("true")) == 0)
		return true;

	return false;
}
