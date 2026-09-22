#ifndef LOAD_MDL_H
#define LOAD_MDL_H

#include <irrlicht.h>
#include <string>
#include <vector>

namespace Vortex3D {

    class LoadMdl {
    public:
        LoadMdl(irr::scene::ISceneManager* smgr, irr::video::IVideoDriver* driver);
        ~LoadMdl();

        // Charge un modèle binaire .mdl statique
        bool Load(const std::string& filePath);

        // Applique une texture sur le modèle
        bool LoadTexture(const std::string& texturePath);

        // Positionnement et transformations
        void SetPosition(float x, float y, float z);
        void SetRotation(float pitch, float yaw, float roll);
        void SetScale(float x, float y, float z);

        irr::scene::IMeshSceneNode* getSceneNode() const { return m_node; }

    private:
        void cleanup();

        irr::scene::ISceneManager* m_smgr = nullptr;
        irr::video::IVideoDriver* m_driver = nullptr;
        irr::scene::SMesh* m_mesh = nullptr;
        irr::scene::IMeshSceneNode* m_node = nullptr;
    };

} // namespace Vortex3D

#endif // LOAD_MDL_H
