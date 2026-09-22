#include "Shaders.h"
#include <iostream>

namespace Vortex3D {

    void ShaderCallBack::OnSetConstants(irr::video::IMaterialRendererServices* services, irr::s32 userData) {
        irr::video::IVideoDriver* driver = services->getVideoDriver();

        // 1. Calcul des matrices
        irr::core::matrix4 world = driver->getTransform(irr::video::ETS_WORLD);
        irr::core::matrix4 view = driver->getTransform(irr::video::ETS_VIEW);
        irr::core::matrix4 proj = driver->getTransform(irr::video::ETS_PROJECTION);
        irr::core::matrix4 worldViewProj = proj * view * world;

        // --- VERTEX SHADER UNIFORMS (Utilisation explicite de getVertexShaderConstantID) ---
        irr::s32 mvpID = services->getVertexShaderConstantID("mWorldViewProj");
        if (mvpID >= 0) {
            services->setVertexShaderConstant(mvpID, worldViewProj.pointer(), 16);
        }

        irr::s32 worldID = services->getVertexShaderConstantID("mWorld");
        if (worldID >= 0) {
            services->setVertexShaderConstant(worldID, world.pointer(), 16);
        }

        // --- PIXEL / FRAGMENT SHADER UNIFORMS (Utilisation explicite de getPixelShaderConstantID) ---
        irr::core::matrix4 invView;
        view.getInverse(invView);
        irr::core::vector3df camPos = invView.getTranslation();

        irr::s32 camPosID = services->getPixelShaderConstantID("camPos");
        if (camPosID >= 0) {
            services->setPixelShaderConstant(camPosID, &camPos.X, 3);
        }

        irr::s32 lightPosID = services->getPixelShaderConstantID("lightPos");
        if (lightPosID >= 0) {
            services->setPixelShaderConstant(lightPosID, &lightPos.X, 3);
        }

        int textureUnit = 0;
        irr::s32 texID = services->getPixelShaderConstantID("myTexture");
        if (texID >= 0) {
            services->setPixelShaderConstant(texID, &textureUnit, 1);
        }
    }

    Shaders::Shaders(irr::IrrlichtDevice* device) : m_device(device) {
        m_callBack = new ShaderCallBack();
    }

    Shaders::~Shaders() {
        if (m_callBack) {
            m_callBack->drop();
            m_callBack = nullptr;
        }
    }

    bool Shaders::LoadShader(const std::string& vertPath, const std::string& fragPath) {
        if (!m_device) {
            std::cerr << "[Shaders] Erreur : Device Irrlicht non valide.\n";
            return false;
        }

        irr::video::IVideoDriver* driver = m_device->getVideoDriver();
        irr::video::IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();

        if (!gpu) {
            std::cerr << "[Shaders] Erreur : GPU Programming Services non disponibles.\n";
            return false;
        }

        m_materialType = gpu->addHighLevelShaderMaterialFromFiles(
            vertPath.c_str(),
                                                                  fragPath.c_str(),
                                                                  m_callBack,
                                                                  irr::video::EMT_SOLID
        );

        if (m_materialType < 0) {
            std::cerr << "[Shaders] Erreur de chargement des shaders : "
            << vertPath << " / " << fragPath << std::endl;
            return false;
        }

        m_isLoaded = true;
        std::cout << "[Shaders] Shader charge avec succes : " << vertPath << " & " << fragPath << std::endl;
        return true;
    }

    void Shaders::ApplyToNode(irr::scene::IMeshSceneNode* node) {
        if (!node || !m_isLoaded) {
            std::cerr << "[Shaders] Erreur : Nœud invalide ou shader non charge.\n";
            return;
        }

        node->setMaterialType((irr::video::E_MATERIAL_TYPE)m_materialType);
    }

    void Shaders::SetLightPosition(float x, float y, float z) {
        if (m_callBack) {
            m_callBack->lightPos.set(x, y, z);
        }
    }

} // namespace Vortex3D
