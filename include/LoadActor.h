#ifndef LOAD_ACTOR_H
#define LOAD_ACTOR_H

#include <irrlicht.h>
#include <assimp/scene.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace Vortex3D {

    struct ActorVertex {
        irr::core::vector3df position;
        irr::core::vector3df normal;
        irr::core::vector2df uv;
        int boneIDs[4] = {-1, -1, -1, -1};
        float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        void addBone(int boneID, float weight) {
            for (int i = 0; i < 4; ++i) {
                if (boneIDs[i] < 0) {
                    boneIDs[i] = boneID;
                    weights[i] = weight;
                    return;
                }
            }
        }
    };

    struct ActorBoneInfo {
        int id;
        std::string name;
        aiMatrix4x4 offsetMatrix;
    };

    struct SubMeshInfo {
        std::vector<ActorVertex> originalVertices;
        irr::scene::SMeshBuffer* buffer = nullptr;
    };

    class LoadActor {
    public:
        LoadActor(irr::scene::ISceneManager* smgr, irr::video::IVideoDriver* driver);
        ~LoadActor();

        bool Load(const std::string& filePath);
        bool LoadTexture(const std::string& texturePath);

        void Update(float deltaTime);
        void Draw(); // Conservé pour compatibilité

        void SetPosition(float x, float y, float z);
        void SetRotation(float pitch, float yaw, float roll);
        void SetScale(float x, float y, float z);

        void SetAnim(const std::string& animName, bool loop = true, float speed = 1.0f);
        void SetAnim(const std::string& animName, float speed);
        void StopAnim();
        void SetAnimSpeed(float speed) { m_animSpeed = speed; }
        void SetAnimLoop(bool loop) { m_animLoop = loop; }

        void cleanup();

        irr::scene::IMeshSceneNode* getSceneNode() const { return m_node; }

    private:
        void applyCpuSkinning();
        void updateBoneHierarchy(const aiNode* node, const aiMatrix4x4& parentTransform, float animTime);

        irr::scene::ISceneManager* m_smgr = nullptr;
        irr::video::IVideoDriver* m_driver = nullptr;

        irr::scene::SMesh* m_mesh = nullptr;
        irr::scene::IMeshSceneNode* m_node = nullptr;

        std::vector<SubMeshInfo> m_subMeshes;
        std::unordered_map<std::string, ActorBoneInfo> m_boneMapping;
        std::vector<aiMatrix4x4> m_boneTransforms;
        const aiScene* m_rawScene = nullptr;

        irr::core::vector3df m_position{0.0f, 0.0f, 0.0f};
        irr::core::vector3df m_rotation{0.0f, 0.0f, 0.0f};
        irr::core::vector3df m_scale{1.0f, 1.0f, 1.0f};

        std::string m_currentAnim = "Walk";
        float m_animTime = 0.0f;
        float m_animSpeed = 1.0f;
        bool m_animLoop = true;
        bool m_isPlaying = false;
    };

} // namespace Vortex3D

#endif // LOAD_ACTOR_H
