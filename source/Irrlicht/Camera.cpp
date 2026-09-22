#include "Camera.h"
#include <cmath>
#include <algorithm>

namespace Vortex3D {

    Camera::Camera(irr::IrrlichtDevice* device) : m_device(device) {
        if (m_device) {
            irr::scene::ISceneManager* smgr = m_device->getSceneManager();
            m_camera = smgr->addCameraSceneNode(nullptr, irr::core::vector3df(0.0f, 10.0f, -25.0f), m_target);
        }
    }

    Camera::~Camera() {
        if (m_camera) {
            m_camera->remove();
            m_camera = nullptr;
        }
    }

    void Camera::SetCamera(float x, float y, float z) {
        if (m_camera) {
            m_camera->setPosition(irr::core::vector3df(x, y, z));
        }

        // Recalcul de la distance et des angles orbitaux selon la nouvelle position
        irr::core::vector3df pos(x, y, z);
        irr::core::vector3df diff = pos - m_target;
        m_distance = diff.getLength();

        if (m_distance > 0.0001f) {
            m_angleV = irr::core::radToDeg(asin(diff.Y / m_distance));
            m_angleH = irr::core::radToDeg(atan2(diff.X, diff.Z));
        }
    }

    void Camera::SetCameraLook(float x, float y, float z) {
        m_target.set(x, y, z);
        if (m_camera) {
            m_camera->setTarget(m_target);
        }
        if (m_isOrbital) {
            updateOrbitalPosition();
        }
    }

    void Camera::SetOrbitalCamera(bool active) {
        m_isOrbital = active;
        if (m_device && active) {
            m_device->setEventReceiver(this);
            updateOrbitalPosition();
        }
    }

    bool Camera::OnEvent(const irr::SEvent& event) {
        if (!m_isOrbital || !m_camera) return false;

        if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
            switch (event.MouseInput.Event) {
                case irr::EMIE_LMOUSE_PRESSED_DOWN:
                case irr::EMIE_RMOUSE_PRESSED_DOWN:
                    m_isMouseDown = true;
                    m_lastMousePos.X = event.MouseInput.X;
                    m_lastMousePos.Y = event.MouseInput.Y;
                    return true;

                case irr::EMIE_LMOUSE_LEFT_UP:
                case irr::EMIE_RMOUSE_LEFT_UP:
                    m_isMouseDown = false;
                    return true;

                case irr::EMIE_MOUSE_MOVED:
                    if (m_isMouseDown) {
                        float dx = static_cast<float>(event.MouseInput.X - m_lastMousePos.X);
                        float dy = static_cast<float>(event.MouseInput.Y - m_lastMousePos.Y);

                        m_angleH += dx * 0.5f;
                        m_angleV -= dy * 0.5f;

                        // Blocage de l'angle vertical entre -89° et 89° pour éviter la déformation
                        m_angleV = std::clamp(m_angleV, -89.0f, 89.0f);

                        m_lastMousePos.X = event.MouseInput.X;
                        m_lastMousePos.Y = event.MouseInput.Y;

                        updateOrbitalPosition();
                        return true;
                    }
                    break;

                case irr::EMIE_MOUSE_WHEEL:
                    // Zoom avant/arrière avec la molette
                    m_distance -= event.MouseInput.Wheel * 2.0f;
                    if (m_distance < 1.0f) m_distance = 1.0f;

                    updateOrbitalPosition();
                return true;

                default:
                    break;
            }
        }

        return false;
    }

    void Camera::updateOrbitalPosition() {
        if (!m_camera) return;

        float radH = irr::core::degToRad(m_angleH);
        float radV = irr::core::degToRad(m_angleV);

        float x = m_target.X + m_distance * cos(radV) * sin(radH);
        float y = m_target.Y + m_distance * sin(radV);
        float z = m_target.Z + m_distance * cos(radV) * cos(radH);

        m_camera->setPosition(irr::core::vector3df(x, y, z));
        m_camera->setTarget(m_target);
    }

} // namespace Vortex3D
