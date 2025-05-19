/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "CSceneNodeAnimatorCameraOrtho.h"
#include "ICursorControl.h"
#include "SViewFrustum.h"
#include "ISceneManager.h"

namespace irr
{
namespace scene
{

CSceneNodeAnimatorCameraOrtho::CSceneNodeAnimatorCameraOrtho(gui::ICursorControl* cursor,
	const core::dimension2du& viewSize, EORTHO_ORIENTATION orientation,
	f32 zoomSpeed, f32 translationSpeed, f32 zoom, f32 nearZ, f32 farZ)
	: CursorControl(cursor), ViewSize(viewSize), Orientation(orientation),
	MousePos(0.5f, 0.5f), ZoomSpeed(zoomSpeed), TranslateSpeed(translationSpeed),
	CurrentZoom(zoom), Translating(false), NearZ(nearZ), FarZ(farZ)
{
	#ifdef _DEBUG
	setDebugName("CSceneNodeAnimatorCameraOrtho");
	#endif

	if (CursorControl)
	{
		CursorControl->grab();
		MousePos = CursorControl->getRelativePosition();
	}

	allKeysUp();
}

CSceneNodeAnimatorCameraOrtho::~CSceneNodeAnimatorCameraOrtho(void)
{
	if (CursorControl)
		CursorControl->drop();
}

void CSceneNodeAnimatorCameraOrtho::animateNode(ISceneNode* node, u32 timeMs)
{
	if (!node || node->getType() != ESNT_CAMERA)
		return;

	ICameraSceneNode* camera = static_cast<ICameraSceneNode*>(node);

	// If the camera isn't the active camera, and receiving input, then don't process it.
	if (!camera->isInputReceiverEnabled())
		return;

	scene::ISceneManager* smgr = camera->getSceneManager();
	if (smgr && smgr->getActiveCamera() != camera)
		return;

	core::vector3df translate(camera->getPosition());
	core::vector3df target(camera->getTarget());

	if (isMouseKeyDown(1))
	{
		if (!Translating)
		{
			TranslateStart = MousePos;
			Translating = true;
		}
		else
		{
			f32 deltaX = (TranslateStart.X - MousePos.X) * TranslateSpeed;
			f32 deltaY = (TranslateStart.Y - MousePos.Y) * TranslateSpeed;
			switch (Orientation)
			{
			case EOO_XY:
				translate.X += deltaX;
				translate.Y += deltaY;
				target = translate;
				target.Z = 0;
				break;
			case EOO_YZ:
				translate.Z -= deltaX;
				translate.Y += deltaY;
				target = translate;
				target.X = 0;
				break;
			case EOO_XZ:
				translate.X += deltaX;
				translate.Z -= deltaY;
				target = translate;
				target.Y = 0;
				break;
			}
		}
	}
	else if (Translating)
	{
		f32 deltaX = (TranslateStart.X - MousePos.X) * TranslateSpeed;
		f32 deltaY = (TranslateStart.Y - MousePos.Y) * TranslateSpeed;
		switch (Orientation)
		{
		case EOO_XY:
			translate.X += deltaX;
			translate.Y += deltaY;
			target = translate;
			target.Z = 0;
			break;
		case EOO_YZ:
			translate.Z -= deltaX;
			translate.Y += deltaY;
			target = translate;
			target.X = 0;
			break;
		case EOO_XZ:
			translate.X += deltaX;
			translate.Z -= deltaY;
			target = translate;
			target.Y = 0;
			break;
		}

		Translating = false;
	}

	camera->setPosition(translate);
	camera->setTarget(target);

	// build the orthographic projection matrix
	core::matrix4 proj;
	proj.buildProjectionMatrixOrthoLH(ViewSize.Width / CurrentZoom, ViewSize.Height / CurrentZoom,
		NearZ, FarZ);
	camera->setProjectionMatrix(proj, true);
}

bool CSceneNodeAnimatorCameraOrtho::OnEvent(const SEvent& event)
{
	if (event.EventType != EET_MOUSE_INPUT_EVENT)
		return false;

	switch (event.MouseInput.Event)
	{
	case EMIE_LMOUSE_PRESSED_DOWN:
		MouseKeys[0] = true;
		break;
	case EMIE_RMOUSE_PRESSED_DOWN:
		MouseKeys[2] = true;
		break;
	case EMIE_MMOUSE_PRESSED_DOWN:
		MouseKeys[1] = true;
		break;
	case EMIE_LMOUSE_LEFT_UP:
		MouseKeys[0] = false;
		break;
	case EMIE_RMOUSE_LEFT_UP:
		MouseKeys[2] = false;
		break;
	case EMIE_MMOUSE_LEFT_UP:
		MouseKeys[1] = false;
		break;
	case EMIE_MOUSE_MOVED:
		MousePos = CursorControl->getRelativePosition();
		break;
	case EMIE_MOUSE_WHEEL:
		if (event.MouseInput.Wheel < 0)
			CurrentZoom -= ZoomSpeed;
		else
			CurrentZoom += ZoomSpeed;

		if (CurrentZoom < 0.1f)
			CurrentZoom = 0.1f;
		if (CurrentZoom > 5.0f)
			CurrentZoom = 5.0f;
		break;
	case EMIE_LMOUSE_DOUBLE_CLICK:
	case EMIE_RMOUSE_DOUBLE_CLICK:
	case EMIE_MMOUSE_DOUBLE_CLICK:
	case EMIE_LMOUSE_TRIPLE_CLICK:
	case EMIE_RMOUSE_TRIPLE_CLICK:
	case EMIE_MMOUSE_TRIPLE_CLICK:
	case EMIE_COUNT:
		return false;
	}
	return true;
}

void CSceneNodeAnimatorCameraOrtho::resize(const core::dimension2di& viewSize)
{
	ViewSize = viewSize;
}

core::vector3df CSceneNodeAnimatorCameraOrtho::transformPoint(s32 x, s32 y)
{
	core::vector3df transform;

	switch (Orientation)
	{
	case EOO_XY:
		transform.X = -x;
		transform.Y = -y;
		transform.Z = 0;
		break;
	case EOO_YZ:
		transform.Z = x;
		transform.Y = -y;
		transform.X = 0;
		break;
	case EOO_XZ:
		transform.X = -x;
		transform.Z = y;
		transform.Y = 0;
		break;
	}

	return transform;
}

void CSceneNodeAnimatorCameraOrtho::allKeysUp()
{
	for (s32 i = 0; i < 3; ++i)
		MouseKeys[i] = false;
}

bool CSceneNodeAnimatorCameraOrtho::isMouseKeyDown(s32 key) const
{
	return MouseKeys[key];
}

ISceneNodeAnimator* CSceneNodeAnimatorCameraOrtho::createClone(ISceneNode* node, ISceneManager* newManager)
{
	CSceneNodeAnimatorCameraOrtho* newAnimator =
		new CSceneNodeAnimatorCameraOrtho(CursorControl, ViewSize, Orientation, ZoomSpeed, TranslateSpeed);
	return newAnimator;
}

}
}