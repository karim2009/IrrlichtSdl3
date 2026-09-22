#ifndef CAMERA_H
#define CAMERA_H

#include <irrlicht.h>

namespace Vortex3D {

    class Camera : public irr::IEventReceiver {
    public:
        Camera(irr::IrrlichtDevice* device);
        ~Camera();

        // Définit la position de la caméra
        void SetCamera(float x, float y, float z);

        // Définit la cible / le point regardé par la caméra
        void SetCameraLook(float x, float y, float z);

        // Active le mode orbite contrôlable avec la souris et la molette
        void SetOrbitalCamera(bool active = true);

        // Gestionnaire d'événements Irrlicht (clic, déplacement souris, molette)
        virtual bool OnEvent(const irr::SEvent& event) override;

        irr::scene::ICameraSceneNode* getCameraNode() const { return m_camera; }

    private:
        void updateOrbitalPosition();

        irr::IrrlichtDevice* m_device = nullptr;
        irr::scene::ICameraSceneNode* m_camera = nullptr;

        irr::core::vector3df m_target{0.0f, 0.0f, 0.0f};
        float m_distance = 25.0f;
        float m_angleH = 0.0f;   // Angle horizontal (yaw)
        float m_angleV = 20.0f;  // Angle vertical (pitch)

        bool m_isOrbital = false;
        bool m_isMouseDown = false;
        irr::core::position2di m_lastMousePos{0, 0};
    };

} // namespace Vortex3D

#endif // CAMERA_H
