#ifndef LOAD_MODEL_H
#define LOAD_MODEL_H

#include <irrlicht.h>
#include <string>

namespace Vortex3D {

    class LoadModel {
    public:
        LoadModel(irr::scene::ISceneManager* smgr, irr::video::IVideoDriver* driver);
        ~LoadModel();

        bool loadObject(const std::string& modelPath);
        bool setTexture(const std::string& texturePath);

        void setPosition(float x, float y, float z);
        void setRotation(float pitch, float yaw, float roll);
        void setScale(float x, float y, float z);

        void draw();
        void cleanup();

        irr::scene::IMeshSceneNode* getSceneNode() const { return m_node; }

    private:
        irr::scene::ISceneManager* m_smgr = nullptr;
        irr::video::IVideoDriver* m_driver = nullptr;

        irr::scene::SMesh* m_mesh = nullptr;
        irr::scene::IMeshSceneNode* m_node = nullptr;

        irr::core::vector3df m_position{0.0f, 0.0f, 0.0f};
        irr::core::vector3df m_rotation{0.0f, 0.0f, 0.0f};
        irr::core::vector3df m_scale{1.0f, 1.0f, 1.0f};

        bool m_isLoaded = false;
        bool m_hasTexture = false;
    };

} // namespace Vortex3D

#endif // LOAD_MODEL_H
