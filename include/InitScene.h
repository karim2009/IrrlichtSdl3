#ifndef INIT_SCENE_H
#define INIT_SCENE_H

#include <irrlicht.h>
#include <string>

namespace Vortex3D {

    // Initialisation du device
    irr::IrrlichtDevice* InitScene(int width, int height, const std::string& title, bool fullscreen = false);

    // Définition de la couleur de fond
    void SetBackgroundColor(irr::video::SColor color);
    void SetBackgroundColor(irr::u32 a, irr::u32 r, irr::u32 g, irr::u32 b);

    // Raccourci pour lancer le rendu de la trame avec la couleur enregistrée
    void BeginFrame(irr::video::IVideoDriver* driver);

} // namespace Vortex3D

#endif // INIT_SCENE_H
