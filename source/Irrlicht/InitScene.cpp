#include "InitScene.h"
#include <SDL3/SDL.h>

namespace Vortex3D {

    static irr::video::SColor g_backgroundColor(255, 100, 101, 140);

    irr::IrrlichtDevice* InitScene(int width, int height, const std::string& title, bool fullscreen) {
        // Priorité à Wayland sous SDL3
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");

        irr::SIrrlichtCreationParameters params;
        params.DeviceType = irr::EIDT_SDL; // Force le device SDL3 (empêche le basculement en X11 natif)
        params.DriverType = irr::video::EDT_OPENGL;
        params.WindowSize = irr::core::dimension2d<irr::u32>(width, height);
        params.Bits = 32;
        params.Fullscreen = fullscreen;
        params.Stencilbuffer = true;
        params.Vsync = true;

        irr::IrrlichtDevice* device = irr::createDeviceEx(params);
        if (device) {
            std::string fullTitle = title;
            std::wstring wTitle(fullTitle.begin(), fullTitle.end());
            device->setWindowCaption(wTitle.c_str());
        }

        return device;
    }

    void SetBackgroundColor(irr::video::SColor color) {
        g_backgroundColor = color;
    }

    void SetBackgroundColor(irr::u32 a, irr::u32 r, irr::u32 g, irr::u32 b) {
        g_backgroundColor = irr::video::SColor(a, r, g, b);
    }

    void BeginFrame(irr::video::IVideoDriver* driver) {
        if (driver) {
            driver->beginScene(true, true, g_backgroundColor);
        }
    }

} // namespace Vortex3D
