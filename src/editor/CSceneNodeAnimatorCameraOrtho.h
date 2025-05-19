/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "ISceneNodeAnimator.h"
#include "ICameraSceneNode.h"
#include "vector2d.h"
#include "vector3d.h"

namespace irr
{

namespace gui
{
	class ICursorControl;
}

namespace scene
{
	class CSceneNodeAnimatorCameraOrtho : public ISceneNodeAnimator
	{
	public:
		enum EORTHO_ORIENTATION
		{
			EOO_XY, // FRONT
			EOO_YZ, // RIGHT
			EOO_XZ, // TOP
		};

	public:
		CSceneNodeAnimatorCameraOrtho(gui::ICursorControl* cursor,
			const core::dimension2du& viewSize, EORTHO_ORIENTATION orientation,
			f32 zoomSpeed = 0.05f, f32 translationSpeed = 20.f, f32 zoom = 1.f,
			f32 nearZ = -10000, f32 farZ = 10000);
		~CSceneNodeAnimatorCameraOrtho(void);

		void animateNode(ISceneNode* node, u32 timeMs);
		bool OnEvent(const SEvent& event);

		void resize(const core::dimension2di& viewSize);
		core::vector3df transformPoint(s32 x, s32 y);

		bool isEventReceiverEnabled() const
		{
			return true;
		}

		ESCENE_NODE_ANIMATOR_TYPE getType() const
		{
			return ESNAT_CAMERA_MAYA;
		}

		ISceneNodeAnimator* createClone(ISceneNode* node, ISceneManager* newManager = 0);

	private:
		void allKeysUp();
		bool isMouseKeyDown(s32 key) const;

		bool MouseKeys[3];

		core::dimension2du ViewSize;
		EORTHO_ORIENTATION Orientation;

		gui::ICursorControl* CursorControl;
		core::position2df TranslateStart;
		core::position2df MousePos;
		f32 ZoomSpeed;
		f32 TranslateSpeed;
		f32 CurrentZoom;
		bool Translating;
		f32 NearZ;
		f32 FarZ;
	};
}
}