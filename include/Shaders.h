#ifndef SHADERS_H
#define SHADERS_H

#include <irrlicht.h>
#include <string>

namespace Vortex3D {

    class ShaderCallBack : public irr::video::IShaderConstantSetCallBack {
    public:
        irr::core::vector3df lightPos{10.0f, 20.0f, -10.0f};

        virtual void OnSetConstants(irr::video::IMaterialRendererServices* services, irr::s32 userData) override;
    };

    class Shaders {
    public:
        Shaders(irr::IrrlichtDevice* device);
        ~Shaders();

        // Charge les fichiers GLSL (.vert et .frag) et crée le matériau
        bool LoadShader(const std::string& vertPath, const std::string& fragPath);

        // Applique le shader sur un nœud 3D Irrlicht
        void ApplyToNode(irr::scene::IMeshSceneNode* node);

        // Ajuste la position de la lumière transmise au shader
        void SetLightPosition(float x, float y, float z);

        irr::s32 getMaterialType() const { return m_materialType; }

    private:
        irr::IrrlichtDevice* m_device = nullptr;
        ShaderCallBack* m_callBack = nullptr;
        irr::s32 m_materialType = irr::video::EMT_SOLID;
        bool m_isLoaded = false;
    };

} // namespace Vortex3D

#endif // SHADERS_H
