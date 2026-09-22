// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_IRR_DEVICE_SDL_H_INCLUDED__
#define __C_IRR_DEVICE_SDL_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_

#include "IrrlichtDevice.h"
#include "CIrrDeviceStub.h"
#include "IImagePresenter.h"
#include "ICursorControl.h"

#include <SDL3/SDL.h>

namespace irr
{

	class CIrrDeviceSDL : public CIrrDeviceStub, video::IImagePresenter
	{
	public:

		//! constructor
		CIrrDeviceSDL(const SIrrlichtCreationParameters& param);

		//! destructor
		virtual ~CIrrDeviceSDL();

		//! runs the device. Returns false if device wants to be deleted
		virtual bool run() _IRR_OVERRIDE_;

		//! pause execution temporarily
		virtual void yield() _IRR_OVERRIDE_;

		//! pause execution for a specified time
		virtual void sleep(u32 timeMs, bool pauseTimer) _IRR_OVERRIDE_;

		//! sets the caption of the window
		virtual void setWindowCaption(const wchar_t* text) _IRR_OVERRIDE_;

		//! returns if window is active. if not, nothing need to be drawn
		virtual bool isWindowActive() const _IRR_OVERRIDE_;

		//! returns if window has focus.
		bool isWindowFocused() const _IRR_OVERRIDE_;

		//! returns if window is minimized.
		bool isWindowMinimized() const _IRR_OVERRIDE_;

		//! returns color format of the window.
		video::ECOLOR_FORMAT getColorFormat() const _IRR_OVERRIDE_;

		//! presents a surface in the client area
		virtual bool present(video::IImage* surface, void* windowId=0, core::rect<s32>* src=0) _IRR_OVERRIDE_;

		//! notifies the device that it should close itself
		virtual void closeDevice() _IRR_OVERRIDE_;

		//! \return Returns a pointer to a list with all video modes supported
		virtual video::IVideoModeList* getVideoModeList() _IRR_OVERRIDE_;

		//! Sets if the window should be resizable in windowed mode.
		virtual void setResizable(bool resize=false) _IRR_OVERRIDE_;

		//! Minimizes the window.
		virtual void minimizeWindow() _IRR_OVERRIDE_;

		//! Maximizes the window.
		virtual void maximizeWindow() _IRR_OVERRIDE_;

		//! Restores the window size.
		virtual void restoreWindow() _IRR_OVERRIDE_;

		//! Get the position of this window on screen
		virtual core::position2di getWindowPosition() _IRR_OVERRIDE_;

		//! Activate any joysticks, and generate events for them.
		virtual bool activateJoysticks(core::array<SJoystickInfo> & joystickInfo) _IRR_OVERRIDE_;

		//! Set the current Gamma Value for the Display
		virtual bool setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast ) _IRR_OVERRIDE_;

		//! Get the current Gamma Value for the Display
		virtual bool getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast ) _IRR_OVERRIDE_;

		//! Get the device type
		virtual E_DEVICE_TYPE getType() const _IRR_OVERRIDE_
		{
			return EIDT_SDL;
		}

		SDL_Window* getSDLWindow() const { return Window; }
		SDL_GLContext getSDLGLContext() const { return Context; }

		//! Implementation of the cursor control
		class CCursorControl : public gui::ICursorControl
		{
		public:

			CCursorControl(CIrrDeviceSDL* dev)
			: Device(dev), IsVisible(true)
			{
			}

			virtual void setVisible(bool visible) _IRR_OVERRIDE_
			{
				IsVisible = visible;
				if ( visible )
					SDL_ShowCursor();
				else
					SDL_HideCursor();
			}

			virtual bool isVisible() const _IRR_OVERRIDE_
			{
				return IsVisible;
			}

			virtual void setPosition(const core::position2d<f32> &pos) _IRR_OVERRIDE_
			{
				setPosition(pos.X, pos.Y);
			}

			virtual void setPosition(f32 x, f32 y) _IRR_OVERRIDE_
			{
				setPosition((s32)(x*Device->Width), (s32)(y*Device->Height));
			}

			virtual void setPosition(const core::position2d<s32> &pos) _IRR_OVERRIDE_
			{
				setPosition(pos.X, pos.Y);
			}

			virtual void setPosition(s32 x, s32 y) _IRR_OVERRIDE_
			{
				if (Device->Window)
					SDL_WarpMouseInWindow(Device->Window, (float)x, (float)y);
			}

			virtual const core::position2d<s32>& getPosition() _IRR_OVERRIDE_
			{
				updateCursorPos();
				return CursorPos;
			}

			virtual core::position2d<f32> getRelativePosition() _IRR_OVERRIDE_
			{
				updateCursorPos();
				return core::position2d<f32>(CursorPos.X / (f32)Device->Width,
											 CursorPos.Y / (f32)Device->Height);
			}

			virtual void setReferenceRect(core::rect<s32>* rect=0) _IRR_OVERRIDE_
			{
			}

		private:

			void updateCursorPos()
			{
				CursorPos.X = Device->MouseX;
				CursorPos.Y = Device->MouseY;

				if (CursorPos.X < 0)
					CursorPos.X = 0;
				if (CursorPos.X > (s32)Device->Width)
					CursorPos.X = Device->Width;
				if (CursorPos.Y < 0)
					CursorPos.Y = 0;
				if (CursorPos.Y > (s32)Device->Height)
					CursorPos.Y = Device->Height;
			}

			CIrrDeviceSDL* Device;
			core::position2d<s32> CursorPos;
			bool IsVisible;
		};

	private:

		void createDriver();
		bool createWindow();
		void createKeyMap();

		SDL_Window* Window;
		SDL_GLContext Context;
		Uint32 WindowFlags;

		#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
		core::array<SDL_Joystick*> Joysticks;
		#endif

		s32 MouseX, MouseY;
		u32 MouseButtonStates;

		u32 Width, Height;

		bool Resizable;
		bool WindowHasFocus;
		bool WindowMinimized;

		struct SKeyMap
		{
			SKeyMap() {}
			SKeyMap(SDL_Keycode sdlKey, s32 win32Key)
			: SDLKey(sdlKey), Win32Key(win32Key)
			{
			}

			SDL_Keycode SDLKey;
			s32 Win32Key;

			bool operator<(const SKeyMap& o) const
			{
				return SDLKey < o.SDLKey;
			}
		};

		core::array<SKeyMap> KeyMap;
	};

} // end namespace irr

#endif // _IRR_COMPILE_WITH_SDL_DEVICE_
#endif // __C_IRR_DEVICE_SDL_H_INCLUDED__
