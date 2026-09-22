// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_

#include "CIrrDeviceSDL.h"
#include "IEventReceiver.h"
#include "irrList.h"
#include "os.h"
#include "CTimer.h"
#include "irrString.h"
#include "Keycodes.h"
#include "COSOperator.h"
#include <stdio.h>
#include <stdlib.h>
#include "SIrrCreationParameters.h"

#ifdef _MSC_VER
#pragma comment(lib, "SDL3.lib")
#endif // _MSC_VER

static int SDLDeviceInstances = 0;

namespace irr
{
	namespace video
	{
		#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_
		IVideoDriver* createDirectX9Driver(const irr::SIrrlichtCreationParameters& params,
										   io::IFileSystem* io, HWND window);
		#endif

		#ifdef _IRR_COMPILE_WITH_OPENGL_
		IVideoDriver* createOpenGLDriver(const SIrrlichtCreationParameters& params,
										 io::IFileSystem* io, CIrrDeviceSDL* device);
		#endif
	} // end namespace video

} // end namespace irr


namespace irr
{

	//! constructor
	CIrrDeviceSDL::CIrrDeviceSDL(const SIrrlichtCreationParameters& param)
	: CIrrDeviceStub(param),
	Window(0), Context(0), WindowFlags(0),
	MouseX(0), MouseY(0), MouseButtonStates(0),
	Width(param.WindowSize.Width), Height(param.WindowSize.Height),
	Resizable(false), WindowHasFocus(true), WindowMinimized(false)
	{
		#ifdef _DEBUG
		setDebugName("CIrrDeviceSDL");
		#endif

		if ( ++SDLDeviceInstances == 1 )
		{
			Uint32 initFlags = SDL_INIT_VIDEO;
			#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
			initFlags |= SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD;
			#endif
			if (!SDL_Init(initFlags))
			{
				os::Printer::log("Unable to initialize SDL3!", SDL_GetError(), ELL_ERROR);
				Close = true;
			}
			else
			{
				os::Printer::log("SDL3 initialized", ELL_INFORMATION);
			}
		}

		core::stringc sdlversion = "SDL Version ";
		sdlversion += SDL_MAJOR_VERSION;
		sdlversion += ".";
		sdlversion += SDL_MINOR_VERSION;
		sdlversion += ".";
		sdlversion += SDL_MICRO_VERSION;

		Operator = new COSOperator(sdlversion);
		if ( SDLDeviceInstances == 1 )
		{
			os::Printer::log(sdlversion.c_str(), ELL_INFORMATION);
		}

		// create keymap
		createKeyMap();

		if ( CreationParams.DriverType != video::EDT_NULL )
		{
			createWindow();
		}

		// create cursor control
		CursorControl = new CCursorControl(this);

		// create driver
		createDriver();

		if (VideoDriver)
			createGUIAndScene();
	}


	//! destructor
	CIrrDeviceSDL::~CIrrDeviceSDL()
	{
		if (Context)
		{
			SDL_GL_DestroyContext(Context);
			Context = 0;
		}

		if (Window)
		{
			SDL_DestroyWindow(Window);
			Window = 0;
		}

		if ( --SDLDeviceInstances == 0 )
		{
			#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
			const u32 numJoysticks = Joysticks.size();
			for (u32 i=0; i<numJoysticks; ++i)
			{
				if (Joysticks[i])
					SDL_CloseJoystick(Joysticks[i]);
			}
			#endif
			SDL_Quit();

			os::Printer::log("Quit SDL3", ELL_INFORMATION);
		}
	}


	bool CIrrDeviceSDL::createWindow()
	{
		if ( Close )
			return false;

		Uint32 flags = 0;

		if (CreationParams.Fullscreen)
			flags |= SDL_WINDOW_FULLSCREEN;
		if (0)
		{
			flags |= SDL_WINDOW_RESIZABLE;
			Resizable = true;
		}
		if (0)
			flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

		if (CreationParams.DriverType == video::EDT_OPENGL)
		{
			flags |= SDL_WINDOW_OPENGL;

			if (CreationParams.Bits == 16)
			{
				SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 4);
				SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 4);
				SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 4);
				SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, CreationParams.WithAlphaChannel ? 1 : 0);
			}
			else
			{
				SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, CreationParams.WithAlphaChannel ? 8 : 0);
			}

			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, CreationParams.ZBufferBits);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, CreationParams.Doublebuffer ? 1 : 0);
			SDL_GL_SetAttribute(SDL_GL_STEREO, CreationParams.Stereobuffer ? 1 : 0);

			if (CreationParams.AntiAlias > 1)
			{
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, CreationParams.AntiAlias);
			}
		}

		const char* title = "Irrlicht Engine";

		Window = SDL_CreateWindow(title, Width, Height, flags);

		if (!Window && CreationParams.AntiAlias > 1)
		{
			while (--CreationParams.AntiAlias > 1)
			{
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, CreationParams.AntiAlias);
				Window = SDL_CreateWindow(title, Width, Height, flags);
				if (Window)
					break;
			}
			if (!Window)
			{
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
				Window = SDL_CreateWindow(title, Width, Height, flags);
				if (Window)
					os::Printer::log("AntiAliasing disabled due to lack of support!", ELL_WARNING);
			}
		}

		if (!Window)
		{
			os::Printer::log("Could not create SDL3 window!", SDL_GetError(), ELL_ERROR);
			return false;
		}

		WindowFlags = flags;

		if (CreationParams.DriverType == video::EDT_OPENGL)
		{
			Context = SDL_GL_CreateContext(Window);
			if (!Context)
			{
				os::Printer::log("Could not create OpenGL context!", SDL_GetError(), ELL_ERROR);
				return false;
			}

			SDL_GL_MakeCurrent(Window, Context);

			if (CreationParams.Vsync)
				SDL_GL_SetSwapInterval(1);
			else
				SDL_GL_SetSwapInterval(0);
		}

		return true;
	}


	//! create the driver
	void CIrrDeviceSDL::createDriver()
	{
		switch(CreationParams.DriverType)
		{
			case video::DEPRECATED_EDT_DIRECT3D8_NO_LONGER_EXISTS:
				os::Printer::log("DIRECT3D8 Driver is no longer supported in Irrlicht. Try another one.", ELL_ERROR);
				break;

			case video::EDT_DIRECT3D9:
				#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_

				VideoDriver = video::createDirectX9Driver(CreationParams, FileSystem, HWnd);
				if (!VideoDriver)
				{
					os::Printer::log("Could not create DIRECT3D9 Driver.", ELL_ERROR);
				}
				#else
				os::Printer::log("DIRECT3D9 Driver was not compiled into this dll. Try another one.", ELL_ERROR);
				#endif // _IRR_COMPILE_WITH_DIRECT3D_9_

				break;

			case video::EDT_SOFTWARE:
				#ifdef _IRR_COMPILE_WITH_SOFTWARE_
				VideoDriver = video::createSoftwareDriver(CreationParams.WindowSize, CreationParams.Fullscreen, FileSystem, this);
				#else
				os::Printer::log("No Software driver support compiled in.", ELL_ERROR);
				#endif
				break;

			case video::EDT_BURNINGSVIDEO:
				#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
				VideoDriver = video::createBurningVideoDriver(CreationParams, FileSystem, this);
				#else
				os::Printer::log("Burning's video driver was not compiled in.", ELL_ERROR);
				#endif
				break;

			case video::EDT_OPENGL:
				#ifdef _IRR_COMPILE_WITH_OPENGL_
				VideoDriver = video::createOpenGLDriver(CreationParams, FileSystem, this);
				#else
				os::Printer::log("No OpenGL support compiled in.", ELL_ERROR);
				#endif
				break;

			case video::EDT_NULL:
				VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
				break;

			default:
				os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
				break;
		}
	}


	//! runs the device. Returns false if device wants to be deleted
	bool CIrrDeviceSDL::run()
	{
		os::Timer::tick();

		SEvent irrevent;
		SDL_Event SDL_event;

		while ( !Close && SDL_PollEvent( &SDL_event ) )
		{
			switch ( SDL_event.type )
			{
				case SDL_EVENT_MOUSE_MOTION:
					irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
					irrevent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
					MouseX = irrevent.MouseInput.X = (s32)SDL_event.motion.x;
					MouseY = irrevent.MouseInput.Y = (s32)SDL_event.motion.y;
					irrevent.MouseInput.ButtonStates = MouseButtonStates;

					postEventFromUser(irrevent);
					break;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_MOUSE_BUTTON_UP:

					irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
					irrevent.MouseInput.X = (s32)SDL_event.button.x;
					irrevent.MouseInput.Y = (s32)SDL_event.button.y;

					irrevent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;

					switch(SDL_event.button.button)
					{
						case SDL_BUTTON_LEFT:
							if (SDL_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
							{
								irrevent.MouseInput.Event = irr::EMIE_LMOUSE_PRESSED_DOWN;
								MouseButtonStates |= irr::EMBSM_LEFT;
							}
							else
							{
								irrevent.MouseInput.Event = irr::EMIE_LMOUSE_LEFT_UP;
								MouseButtonStates &= ~irr::EMBSM_LEFT;
							}
							break;

						case SDL_BUTTON_RIGHT:
							if (SDL_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
							{
								irrevent.MouseInput.Event = irr::EMIE_RMOUSE_PRESSED_DOWN;
								MouseButtonStates |= irr::EMBSM_RIGHT;
							}
							else
							{
								irrevent.MouseInput.Event = irr::EMIE_RMOUSE_LEFT_UP;
								MouseButtonStates &= ~irr::EMBSM_RIGHT;
							}
							break;

						case SDL_BUTTON_MIDDLE:
							if (SDL_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
							{
								irrevent.MouseInput.Event = irr::EMIE_MMOUSE_PRESSED_DOWN;
								MouseButtonStates |= irr::EMBSM_MIDDLE;
							}
							else
							{
								irrevent.MouseInput.Event = irr::EMIE_MMOUSE_LEFT_UP;
								MouseButtonStates &= ~irr::EMBSM_MIDDLE;
							}
							break;
					}

					irrevent.MouseInput.ButtonStates = MouseButtonStates;

					if (irrevent.MouseInput.Event != irr::EMIE_MOUSE_MOVED)
					{
						postEventFromUser(irrevent);

						if ( irrevent.MouseInput.Event >= EMIE_LMOUSE_PRESSED_DOWN && irrevent.MouseInput.Event <= EMIE_MMOUSE_PRESSED_DOWN )
						{
							u32 clicks = checkSuccessiveClicks(irrevent.MouseInput.X, irrevent.MouseInput.Y, irrevent.MouseInput.Event);
							if ( clicks == 2 )
							{
								irrevent.MouseInput.Event = (EMOUSE_INPUT_EVENT)(EMIE_LMOUSE_DOUBLE_CLICK + irrevent.MouseInput.Event-EMIE_LMOUSE_PRESSED_DOWN);
								postEventFromUser(irrevent);
							}
							else if ( clicks == 3 )
							{
								irrevent.MouseInput.Event = (EMOUSE_INPUT_EVENT)(EMIE_LMOUSE_TRIPLE_CLICK + irrevent.MouseInput.Event-EMIE_LMOUSE_PRESSED_DOWN);
								postEventFromUser(irrevent);
							}
						}
					}
					break;

						case SDL_EVENT_MOUSE_WHEEL:
							irrevent.EventType = irr::EET_MOUSE_INPUT_EVENT;
							irrevent.MouseInput.Event = irr::EMIE_MOUSE_WHEEL;
							irrevent.MouseInput.X = MouseX;
							irrevent.MouseInput.Y = MouseY;
							irrevent.MouseInput.Wheel = SDL_event.wheel.y;
							irrevent.MouseInput.ButtonStates = MouseButtonStates;
							postEventFromUser(irrevent);
							break;

						case SDL_EVENT_KEY_DOWN:
						case SDL_EVENT_KEY_UP:
						{
							SKeyMap mp;
							mp.SDLKey = SDL_event.key.key;
							s32 idx = KeyMap.binary_search(mp);

							EKEY_CODE key;
							if (idx == -1)
								key = (EKEY_CODE)0;
							else
								key = (EKEY_CODE)KeyMap[idx].Win32Key;

							#ifdef _IRR_WINDOWS_API_
							// handle alt+f4 in Windows
							if ( (SDL_event.key.mod & SDL_KMOD_ALT) && key == KEY_F4)
							{
								Close = true;
								break;
							}
							#endif
							irrevent.EventType = irr::EET_KEY_INPUT_EVENT;
							irrevent.KeyInput.Char = 0;
							irrevent.KeyInput.Key = key;
							irrevent.KeyInput.PressedDown = (SDL_event.type == SDL_EVENT_KEY_DOWN);
							irrevent.KeyInput.Shift = (SDL_event.key.mod & SDL_KMOD_SHIFT) != 0;
							irrevent.KeyInput.Control = (SDL_event.key.mod & SDL_KMOD_CTRL) != 0;
							postEventFromUser(irrevent);
						}
						break;

						case SDL_EVENT_TEXT_INPUT:
						{
							irrevent.EventType = irr::EET_KEY_INPUT_EVENT;
							irrevent.KeyInput.Key = KEY_KEY_CODES_COUNT;
							irrevent.KeyInput.PressedDown = true;
							irrevent.KeyInput.Shift = false;
							irrevent.KeyInput.Control = false;

							const char* p = SDL_event.text.text;
							while (*p)
							{
								wchar_t wchar = 0;
								int len = mbtowc(&wchar, p, 4);
								if (len > 0 && wchar != 0)
								{
									irrevent.KeyInput.Char = wchar;
									postEventFromUser(irrevent);
									p += len;
								}
								else
									break;
							}
						}
						break;

						case SDL_EVENT_QUIT:
							Close = true;
							break;

						case SDL_EVENT_WINDOW_FOCUS_GAINED:
							WindowHasFocus = true;
							break;

						case SDL_EVENT_WINDOW_FOCUS_LOST:
							WindowHasFocus = false;
							break;

						case SDL_EVENT_WINDOW_MINIMIZED:
							WindowMinimized = true;
							break;

						case SDL_EVENT_WINDOW_RESTORED:
						case SDL_EVENT_WINDOW_MAXIMIZED:
							WindowMinimized = false;
							break;

						case SDL_EVENT_WINDOW_RESIZED:
						case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
							if ((SDL_event.window.data1 != (int)Width) || (SDL_event.window.data2 != (int)Height))
							{
								Width = SDL_event.window.data1;
								Height = SDL_event.window.data2;
								if (VideoDriver)
									VideoDriver->OnResize(core::dimension2d<u32>(Width, Height));
							}
							break;

						case SDL_EVENT_USER:
							irrevent.EventType = irr::EET_USER_EVENT;
							irrevent.UserEvent.UserData1 = reinterpret_cast<uintptr_t>(SDL_event.user.data1);
							irrevent.UserEvent.UserData2 = reinterpret_cast<uintptr_t>(SDL_event.user.data2);

							postEventFromUser(irrevent);
							break;

						default:
							break;
			} // end switch

		} // end while

		#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
		SDL_UpdateJoysticks();
		SEvent joyevent;
		joyevent.EventType = EET_JOYSTICK_INPUT_EVENT;
		for (u32 i=0; i<Joysticks.size(); ++i)
		{
			SDL_Joystick* joystick = Joysticks[i];
			if (joystick)
			{
				int j;
				const int numButtons = core::min_(SDL_GetNumJoystickButtons(joystick), 32);
				joyevent.JoystickEvent.ButtonStates=0;
				for (j=0; j<numButtons; ++j)
					joyevent.JoystickEvent.ButtonStates |= (SDL_GetJoystickButton(joystick, j)<<j);

				const int numAxes = core::min_(SDL_GetNumJoystickAxes(joystick), (int)SEvent::SJoystickEvent::NUMBER_OF_AXES);
				joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_X]=0;
				joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_Y]=0;
				joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_Z]=0;
				joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_R]=0;
				joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_U]=0;
				joyevent.JoystickEvent.Axis[SEvent::SJoystickEvent::AXIS_V]=0;
				for (j=0; j<numAxes; ++j)
					joyevent.JoystickEvent.Axis[j] = SDL_GetJoystickAxis(joystick, j);

				if (SDL_GetNumJoystickHats(joystick)>0)
				{
					switch (SDL_GetJoystickHat(joystick, 0))
					{
						case SDL_HAT_UP:
							joyevent.JoystickEvent.POV=0;
							break;
						case SDL_HAT_RIGHTUP:
							joyevent.JoystickEvent.POV=4500;
							break;
						case SDL_HAT_RIGHT:
							joyevent.JoystickEvent.POV=9000;
							break;
						case SDL_HAT_RIGHTDOWN:
							joyevent.JoystickEvent.POV=13500;
							break;
						case SDL_HAT_DOWN:
							joyevent.JoystickEvent.POV=18000;
							break;
						case SDL_HAT_LEFTDOWN:
							joyevent.JoystickEvent.POV=22500;
							break;
						case SDL_HAT_LEFT:
							joyevent.JoystickEvent.POV=27000;
							break;
						case SDL_HAT_LEFTUP:
							joyevent.JoystickEvent.POV=31500;
							break;
						case SDL_HAT_CENTERED:
						default:
							joyevent.JoystickEvent.POV=65535;
							break;
					}
				}
				else
				{
					joyevent.JoystickEvent.POV=65535;
				}

				joyevent.JoystickEvent.Joystick=static_cast<u8>(i);
				postEventFromUser(joyevent);
			}
		}
		#endif
		return !Close;
	}

	//! Activate any joysticks, and generate events for them.
	bool CIrrDeviceSDL::activateJoysticks(core::array<SJoystickInfo> & joystickInfo)
	{
		#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
		joystickInfo.clear();

		int count = 0;
		SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);
		if (!joysticks || count == 0)
			return false;

		const int numJoysticks = core::min_(count, 256);
		Joysticks.reallocate(numJoysticks);
		joystickInfo.reallocate(numJoysticks);

		for (int i = 0; i < numJoysticks; ++i)
		{
			SDL_Joystick* js = SDL_OpenJoystick(joysticks[i]);
			Joysticks.push_back(js);
			SJoystickInfo info;

			info.Joystick = i;
			if (js)
			{
				info.Axes = SDL_GetNumJoystickAxes(js);
				info.Buttons = SDL_GetNumJoystickButtons(js);
				const char* name = SDL_GetJoystickName(js);
				info.Name = name ? name : "Unknown Joystick";
				info.PovHat = (SDL_GetNumJoystickHats(js) > 0)
				? SJoystickInfo::POV_HAT_PRESENT : SJoystickInfo::POV_HAT_ABSENT;
			}

			joystickInfo.push_back(info);
		}

		SDL_free(joysticks);

		for(u32 joystick = 0; joystick < joystickInfo.size(); ++joystick)
		{
			char logString[256];
			(void)sprintf(logString, "Found joystick %d, %d axes, %d buttons '%s'",
						  joystick, joystickInfo[joystick].Axes,
				 joystickInfo[joystick].Buttons, joystickInfo[joystick].Name.c_str());
			os::Printer::log(logString, ELL_INFORMATION);
		}

		return true;

		#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_

		return false;
	}


	//! pause execution temporarily
	void CIrrDeviceSDL::yield()
	{
		SDL_Delay(0);
	}


	//! pause execution for a specified time
	void CIrrDeviceSDL::sleep(u32 timeMs, bool pauseTimer)
	{
		const bool wasStopped = Timer ? Timer->isStopped() : true;
		if (pauseTimer && !wasStopped)
			Timer->stop();

		SDL_Delay(timeMs);

		if (pauseTimer && !wasStopped)
			Timer->start();
	}


	//! sets the caption of the window
	void CIrrDeviceSDL::setWindowCaption(const wchar_t* text)
	{
		core::stringc textc = text;
		if (Window)
			SDL_SetWindowTitle(Window, textc.c_str());
	}


	//! presents a surface in the client area
	bool CIrrDeviceSDL::present(video::IImage* surface, void* windowId, core::rect<s32>* srcClip)
	{
		if (CreationParams.DriverType == video::EDT_OPENGL)
		{
			if (Window)
			{
				SDL_GL_SwapWindow(Window);
				return true;
			}
			return false;
		}

		SDL_Surface* winSurface = Window ? SDL_GetWindowSurface(Window) : NULL;
		if (!winSurface)
			return false;

		SDL_Surface *sdlSurface = SDL_CreateSurfaceFrom(
			surface->getDimension().Width, surface->getDimension().Height,
														SDL_PIXELFORMAT_ARGB8888, surface->getData(), surface->getPitch());
		if (!sdlSurface)
			return false;

		if (srcClip)
		{
			SDL_Rect sdlsrcClip;
			sdlsrcClip.x = srcClip->UpperLeftCorner.X;
			sdlsrcClip.y = srcClip->UpperLeftCorner.Y;
			sdlsrcClip.w = srcClip->getWidth();
			sdlsrcClip.h = srcClip->getHeight();
			SDL_BlitSurface(sdlSurface, &sdlsrcClip, winSurface, NULL);
		}
		else
		{
			SDL_BlitSurface(sdlSurface, NULL, winSurface, NULL);
		}

		SDL_UpdateWindowSurface(Window);
		SDL_DestroySurface(sdlSurface);
		return true;
	}


	//! notifies the device that it should close itself
	void CIrrDeviceSDL::closeDevice()
	{
		Close = true;
	}


	//! \return Pointer to a list with all video modes supported
	video::IVideoModeList* CIrrDeviceSDL::getVideoModeList()
	{
		if (!VideoModeList->getVideoModeCount())
		{
			int count = 0;
			SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
			SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displayID, &count);
			if (modes != NULL && count > 0)
			{
				for (int i = 0; i < count; ++i)
				{
					VideoModeList->addMode(
						core::dimension2d<u32>(modes[i]->w, modes[i]->h),
										   SDL_BITSPERPIXEL(modes[i]->format)
					);
				}
				SDL_free((void*)modes);
			}
		}

		return VideoModeList;
	}


	//! Sets if the window should be resizable in windowed mode.
	void CIrrDeviceSDL::setResizable(bool resize)
	{
		if (resize != Resizable)
		{
			Resizable = resize;
			if (Window)
				SDL_SetWindowResizable(Window, resize ? true : false);
		}
	}


	//! Minimizes window if possible
	void CIrrDeviceSDL::minimizeWindow()
	{
		if (Window)
			SDL_MinimizeWindow(Window);
	}


	//! Maximize window
	void CIrrDeviceSDL::maximizeWindow()
	{
		if (Window)
			SDL_MaximizeWindow(Window);
	}

	//! Get the position of this window on screen
	core::position2di CIrrDeviceSDL::getWindowPosition()
	{
		int x = -1, y = -1;
		if (Window)
			SDL_GetWindowPosition(Window, &x, &y);
		return core::position2di(x, y);
	}


	//! Restore original window size
	void CIrrDeviceSDL::restoreWindow()
	{
		if (Window)
			SDL_RestoreWindow(Window);
	}


	//! returns if window is active. if not, nothing need to be drawn
	bool CIrrDeviceSDL::isWindowActive() const
	{
		return (WindowHasFocus && !WindowMinimized);
	}


	//! returns if window has focus.
	bool CIrrDeviceSDL::isWindowFocused() const
	{
		return WindowHasFocus;
	}


	//! returns if window is minimized.
	bool CIrrDeviceSDL::isWindowMinimized() const
	{
		return WindowMinimized;
	}


	//! Set the current Gamma Value for the Display
	bool CIrrDeviceSDL::setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast )
	{
		return false;
	}

	//! Get the current Gamma Value for the Display
	bool CIrrDeviceSDL::getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast )
	{
		return false;
	}

	//! returns color format of the window.
	video::ECOLOR_FORMAT CIrrDeviceSDL::getColorFormat() const
	{
		return video::ECF_A8R8G8B8;
	}


	void CIrrDeviceSDL::createKeyMap()
	{
		KeyMap.reallocate(105);

		KeyMap.push_back(SKeyMap(SDLK_BACKSPACE, KEY_BACK));
		KeyMap.push_back(SKeyMap(SDLK_TAB, KEY_TAB));
		KeyMap.push_back(SKeyMap(SDLK_CLEAR, KEY_CLEAR));
		KeyMap.push_back(SKeyMap(SDLK_RETURN, KEY_RETURN));

		KeyMap.push_back(SKeyMap(SDLK_PAUSE, KEY_PAUSE));
		KeyMap.push_back(SKeyMap(SDLK_CAPSLOCK, KEY_CAPITAL));

		KeyMap.push_back(SKeyMap(SDLK_ESCAPE, KEY_ESCAPE));

		KeyMap.push_back(SKeyMap(SDLK_SPACE, KEY_SPACE));
		KeyMap.push_back(SKeyMap(SDLK_PAGEUP, KEY_PRIOR));
		KeyMap.push_back(SKeyMap(SDLK_PAGEDOWN, KEY_NEXT));
		KeyMap.push_back(SKeyMap(SDLK_END, KEY_END));
		KeyMap.push_back(SKeyMap(SDLK_HOME, KEY_HOME));
		KeyMap.push_back(SKeyMap(SDLK_LEFT, KEY_LEFT));
		KeyMap.push_back(SKeyMap(SDLK_UP, KEY_UP));
		KeyMap.push_back(SKeyMap(SDLK_RIGHT, KEY_RIGHT));
		KeyMap.push_back(SKeyMap(SDLK_DOWN, KEY_DOWN));

		KeyMap.push_back(SKeyMap(SDLK_PRINTSCREEN, KEY_PRINT));
		KeyMap.push_back(SKeyMap(SDLK_PRINTSCREEN, KEY_SNAPSHOT));

		KeyMap.push_back(SKeyMap(SDLK_INSERT, KEY_INSERT));
		KeyMap.push_back(SKeyMap(SDLK_DELETE, KEY_DELETE));
		KeyMap.push_back(SKeyMap(SDLK_HELP, KEY_HELP));

		KeyMap.push_back(SKeyMap(SDLK_0, KEY_KEY_0));
		KeyMap.push_back(SKeyMap(SDLK_1, KEY_KEY_1));
		KeyMap.push_back(SKeyMap(SDLK_2, KEY_KEY_2));
		KeyMap.push_back(SKeyMap(SDLK_3, KEY_KEY_3));
		KeyMap.push_back(SKeyMap(SDLK_4, KEY_KEY_4));
		KeyMap.push_back(SKeyMap(SDLK_5, KEY_KEY_5));
		KeyMap.push_back(SKeyMap(SDLK_6, KEY_KEY_6));
		KeyMap.push_back(SKeyMap(SDLK_7, KEY_KEY_7));
		KeyMap.push_back(SKeyMap(SDLK_8, KEY_KEY_8));
		KeyMap.push_back(SKeyMap(SDLK_9, KEY_KEY_9));

		KeyMap.push_back(SKeyMap(SDLK_A, KEY_KEY_A));
		KeyMap.push_back(SKeyMap(SDLK_B, KEY_KEY_B));
		KeyMap.push_back(SKeyMap(SDLK_C, KEY_KEY_C));
		KeyMap.push_back(SKeyMap(SDLK_D, KEY_KEY_D));
		KeyMap.push_back(SKeyMap(SDLK_E, KEY_KEY_E));
		KeyMap.push_back(SKeyMap(SDLK_F, KEY_KEY_F));
		KeyMap.push_back(SKeyMap(SDLK_G, KEY_KEY_G));
		KeyMap.push_back(SKeyMap(SDLK_H, KEY_KEY_H));
		KeyMap.push_back(SKeyMap(SDLK_I, KEY_KEY_I));
		KeyMap.push_back(SKeyMap(SDLK_J, KEY_KEY_J));
		KeyMap.push_back(SKeyMap(SDLK_K, KEY_KEY_K));
		KeyMap.push_back(SKeyMap(SDLK_L, KEY_KEY_L));
		KeyMap.push_back(SKeyMap(SDLK_M, KEY_KEY_M));
		KeyMap.push_back(SKeyMap(SDLK_N, KEY_KEY_N));
		KeyMap.push_back(SKeyMap(SDLK_O, KEY_KEY_O));
		KeyMap.push_back(SKeyMap(SDLK_P, KEY_KEY_P));
		KeyMap.push_back(SKeyMap(SDLK_Q, KEY_KEY_Q));
		KeyMap.push_back(SKeyMap(SDLK_R, KEY_KEY_R));
		KeyMap.push_back(SKeyMap(SDLK_S, KEY_KEY_S));
		KeyMap.push_back(SKeyMap(SDLK_T, KEY_KEY_T));
		KeyMap.push_back(SKeyMap(SDLK_U, KEY_KEY_U));
		KeyMap.push_back(SKeyMap(SDLK_V, KEY_KEY_V));
		KeyMap.push_back(SKeyMap(SDLK_W, KEY_KEY_W));
		KeyMap.push_back(SKeyMap(SDLK_X, KEY_KEY_X));
		KeyMap.push_back(SKeyMap(SDLK_Y, KEY_KEY_Y));
		KeyMap.push_back(SKeyMap(SDLK_Z, KEY_KEY_Z));

		KeyMap.push_back(SKeyMap(SDLK_LGUI, KEY_LWIN));
		KeyMap.push_back(SKeyMap(SDLK_RGUI, KEY_RWIN));
		KeyMap.push_back(SKeyMap(SDLK_SLEEP, KEY_SLEEP));

		KeyMap.push_back(SKeyMap(SDLK_KP_0, KEY_NUMPAD0));
		KeyMap.push_back(SKeyMap(SDLK_KP_1, KEY_NUMPAD1));
		KeyMap.push_back(SKeyMap(SDLK_KP_2, KEY_NUMPAD2));
		KeyMap.push_back(SKeyMap(SDLK_KP_3, KEY_NUMPAD3));
		KeyMap.push_back(SKeyMap(SDLK_KP_4, KEY_NUMPAD4));
		KeyMap.push_back(SKeyMap(SDLK_KP_5, KEY_NUMPAD5));
		KeyMap.push_back(SKeyMap(SDLK_KP_6, KEY_NUMPAD6));
		KeyMap.push_back(SKeyMap(SDLK_KP_7, KEY_NUMPAD7));
		KeyMap.push_back(SKeyMap(SDLK_KP_8, KEY_NUMPAD8));
		KeyMap.push_back(SKeyMap(SDLK_KP_9, KEY_NUMPAD9));
		KeyMap.push_back(SKeyMap(SDLK_KP_MULTIPLY, KEY_MULTIPLY));
		KeyMap.push_back(SKeyMap(SDLK_KP_PLUS, KEY_ADD));
		KeyMap.push_back(SKeyMap(SDLK_KP_MINUS, KEY_SUBTRACT));
		KeyMap.push_back(SKeyMap(SDLK_KP_PERIOD, KEY_DECIMAL));
		KeyMap.push_back(SKeyMap(SDLK_KP_DIVIDE, KEY_DIVIDE));

		KeyMap.push_back(SKeyMap(SDLK_F1,  KEY_F1));
		KeyMap.push_back(SKeyMap(SDLK_F2,  KEY_F2));
		KeyMap.push_back(SKeyMap(SDLK_F3,  KEY_F3));
		KeyMap.push_back(SKeyMap(SDLK_F4,  KEY_F4));
		KeyMap.push_back(SKeyMap(SDLK_F5,  KEY_F5));
		KeyMap.push_back(SKeyMap(SDLK_F6,  KEY_F6));
		KeyMap.push_back(SKeyMap(SDLK_F7,  KEY_F7));
		KeyMap.push_back(SKeyMap(SDLK_F8,  KEY_F8));
		KeyMap.push_back(SKeyMap(SDLK_F9,  KEY_F9));
		KeyMap.push_back(SKeyMap(SDLK_F10, KEY_F10));
		KeyMap.push_back(SKeyMap(SDLK_F11, KEY_F11));
		KeyMap.push_back(SKeyMap(SDLK_F12, KEY_F12));
		KeyMap.push_back(SKeyMap(SDLK_F13, KEY_F13));
		KeyMap.push_back(SKeyMap(SDLK_F14, KEY_F14));
		KeyMap.push_back(SKeyMap(SDLK_F15, KEY_F15));

		KeyMap.push_back(SKeyMap(SDLK_NUMLOCKCLEAR, KEY_NUMLOCK));
		KeyMap.push_back(SKeyMap(SDLK_SCROLLLOCK, KEY_SCROLL));
		KeyMap.push_back(SKeyMap(SDLK_LSHIFT, KEY_LSHIFT));
		KeyMap.push_back(SKeyMap(SDLK_RSHIFT, KEY_RSHIFT));
		KeyMap.push_back(SKeyMap(SDLK_LCTRL,  KEY_LCONTROL));
		KeyMap.push_back(SKeyMap(SDLK_RCTRL,  KEY_RCONTROL));
		KeyMap.push_back(SKeyMap(SDLK_LALT,   KEY_LMENU));
		KeyMap.push_back(SKeyMap(SDLK_RALT,   KEY_RMENU));

		KeyMap.push_back(SKeyMap(SDLK_PLUS,   KEY_PLUS));
		KeyMap.push_back(SKeyMap(SDLK_COMMA,  KEY_COMMA));
		KeyMap.push_back(SKeyMap(SDLK_MINUS,  KEY_MINUS));
		KeyMap.push_back(SKeyMap(SDLK_PERIOD, KEY_PERIOD));

		KeyMap.sort();
	}

} // end namespace irr

#endif // _IRR_COMPILE_WITH_SDL_DEVICE_
