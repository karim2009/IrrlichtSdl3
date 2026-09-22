#include "LoadActor.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Vortex3D {

    namespace {
        const aiAnimation* findAnimation(const aiScene* scene, const std::string& animName) {
            if (!scene || scene->mNumAnimations == 0) return nullptr;

            for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
                std::string rawName = scene->mAnimations[i]->mName.C_Str();
                size_t pipePos = rawName.find_last_of('|');
                std::string cleanName = (pipePos != std::string::npos) ? rawName.substr(pipePos + 1) : rawName;

                if (rawName == animName || cleanName == animName) {
                    return scene->mAnimations[i];
                }
            }
            return scene->mAnimations[0];
        }

        aiVector3D getAnimatedVector(const aiVectorKey* keys, unsigned int numKeys, double animTime, double duration, const aiVector3D& fallback) {
            if (numKeys == 0) return fallback;
            if (numKeys == 1) return keys[0].mValue;
            if (animTime <= keys[0].mTime) return keys[0].mValue;

            if (animTime >= keys[numKeys - 1].mTime) {
                double t1 = keys[numKeys - 1].mTime;
                if (duration > t1) {
                    float factor = static_cast<float>((animTime - t1) / (duration - t1));
                    factor = std::max(0.0f, std::min(1.0f, factor));
                    return keys[numKeys - 1].mValue + (keys[0].mValue - keys[numKeys - 1].mValue) * factor;
                }
                return keys[numKeys - 1].mValue;
            }

            for (unsigned int i = 0; i < numKeys - 1; ++i) {
                if (animTime < keys[i + 1].mTime) {
                    double t1 = keys[i].mTime;
                    double t2 = keys[i + 1].mTime;
                    float factor = static_cast<float>((animTime - t1) / (t2 - t1));
                    factor = std::max(0.0f, std::min(1.0f, factor));
                    return keys[i].mValue + (keys[i + 1].mValue - keys[i].mValue) * factor;
                }
            }
            return keys[numKeys - 1].mValue;
        }

        aiQuaternion getAnimatedQuaternion(const aiQuatKey* keys, unsigned int numKeys, double animTime, double duration, const aiQuaternion& fallback) {
            if (numKeys == 0) return fallback;
            if (numKeys == 1) return keys[0].mValue;
            if (animTime <= keys[0].mTime) return keys[0].mValue;

            auto interpolateQ = [](const aiQuaternion& q1, const aiQuaternion& q2_in, float factor) {
                aiQuaternion q2 = q2_in;
                float dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
                if (dot < 0.0f) {
                    q2.x = -q2.x; q2.y = -q2.y; q2.z = -q2.z; q2.w = -q2.w;
                }
                aiQuaternion result;
                aiQuaternion::Interpolate(result, q1, q2, factor);
                return result.Normalize();
            };

            if (animTime >= keys[numKeys - 1].mTime) {
                double t1 = keys[numKeys - 1].mTime;
                if (duration > t1) {
                    float factor = static_cast<float>((animTime - t1) / (duration - t1));
                    factor = std::max(0.0f, std::min(1.0f, factor));
                    return interpolateQ(keys[numKeys - 1].mValue, keys[0].mValue, factor);
                }
                return keys[numKeys - 1].mValue;
            }

            for (unsigned int i = 0; i < numKeys - 1; ++i) {
                if (animTime < keys[i + 1].mTime) {
                    double t1 = keys[i].mTime;
                    double t2 = keys[i + 1].mTime;
                    float factor = static_cast<float>((animTime - t1) / (t2 - t1));
                    factor = std::max(0.0f, std::min(1.0f, factor));
                    return interpolateQ(keys[i].mValue, keys[i + 1].mValue, factor);
                }
            }
            return keys[numKeys - 1].mValue;
        }
    }

    LoadActor::LoadActor(irr::scene::ISceneManager* smgr, irr::video::IVideoDriver* driver)
    : m_smgr(smgr), m_driver(driver) {
    }

    LoadActor::~LoadActor() {
        cleanup();
    }

    bool LoadActor::Load(const std::string& filePath) {
        if (!m_smgr || !m_driver) {
            std::cerr << "[LoadActor] Erreur : SceneManager ou VideoDriver non initialise.\n";
            return false;
        }

        cleanup();

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_LimitBoneWeights |
            aiProcess_FlipUVs
        );

        if (!scene || !scene->mRootNode) {
            std::cerr << "[LoadActor] Erreur Assimp : " << importer.GetErrorString() << std::endl;
            return false;
        }

        m_rawScene = importer.GetOrphanedScene();
        m_mesh = new irr::scene::SMesh();

        int boneCounter = 0;

        for (unsigned int m = 0; m < m_rawScene->mNumMeshes; ++m) {
            const aiMesh* mesh = m_rawScene->mMeshes[m];
            irr::scene::SMeshBuffer* buffer = new irr::scene::SMeshBuffer();
            SubMeshInfo subMesh;
            subMesh.buffer = buffer;

            for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                ActorVertex v;
                v.position.set(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

                if (mesh->HasNormals()) {
                    v.normal.set(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                } else {
                    v.normal.set(0.0f, 0.0f, 0.0f);
                }

                if (mesh->HasTextureCoords(0)) {
                    v.uv.set(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                } else {
                    v.uv.set(0.0f, 0.0f);
                }

                subMesh.originalVertices.push_back(v);

                irr::video::S3DVertex irrVert(
                    v.position,
                    v.normal,
                    irr::video::SColor(255, 255, 255, 255),
                                              v.uv
                );
                buffer->Vertices.push_back(irrVert);
            }

            for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
                const aiFace& face = mesh->mFaces[i];
                if (face.mNumIndices == 3) {
                    buffer->Indices.push_back(face.mIndices[0]);
                    buffer->Indices.push_back(face.mIndices[1]);
                    buffer->Indices.push_back(face.mIndices[2]);
                }
            }

            for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
                const aiBone* bone = mesh->mBones[b];
                std::string boneName = bone->mName.C_Str();
                int boneID = -1;

                if (m_boneMapping.find(boneName) == m_boneMapping.end()) {
                    ActorBoneInfo newBone;
                    newBone.id = boneCounter;
                    newBone.name = boneName;
                    newBone.offsetMatrix = bone->mOffsetMatrix;

                    m_boneMapping[boneName] = newBone;
                    boneID = boneCounter++;
                } else {
                    boneID = m_boneMapping[boneName].id;
                }

                for (unsigned int weightIdx = 0; weightIdx < bone->mNumWeights; ++weightIdx) {
                    const aiVertexWeight& weight = bone->mWeights[weightIdx];
                    subMesh.originalVertices[weight.mVertexId].addBone(boneID, weight.mWeight);
                }
            }

            buffer->recalculateBoundingBox();
            m_mesh->addMeshBuffer(buffer);
            buffer->drop(); // Transfert de propriété du buffer à m_mesh

            m_subMeshes.push_back(subMesh);
        }

        m_boneTransforms.resize(boneCounter);
        m_mesh->recalculateBoundingBox();

        m_node = m_smgr->addMeshSceneNode(m_mesh);
        if (!m_node) {
            std::cerr << "[LoadActor] Erreur : Impossible de creer le MeshSceneNode.\n";
            m_mesh->drop();
            m_mesh = nullptr;
            return false;
        }

        m_mesh->drop(); // Transfert de propriété du mesh au nœud
        m_mesh = nullptr;

        m_node->setMaterialFlag(irr::video::EMF_LIGHTING, false);

        applyCpuSkinning();

        std::cout << "[LoadActor] Modèle GLTF/GLB/FBX charge : " << filePath
        << " (" << boneCounter << " os)." << std::endl;
        return true;
    }

    bool LoadActor::LoadTexture(const std::string& texturePath) {
        if (!m_node) return false;

        irr::video::ITexture* texture = m_driver->getTexture(texturePath.c_str());
        if (!texture) {
            std::cerr << "[LoadActor] Erreur chargement texture : " << texturePath << std::endl;
            return false;
        }

        m_node->setMaterialTexture(0, texture);
        std::cout << "[LoadActor] Texture appliquee : " << texturePath << std::endl;
        return true;
    }

    void LoadActor::SetPosition(float x, float y, float z) {
        m_position.set(x, y, z);
        if (m_node) m_node->setPosition(m_position);
    }

    void LoadActor::SetRotation(float pitch, float yaw, float roll) {
        m_rotation.set(pitch, yaw, roll);
        if (m_node) m_node->setRotation(m_rotation);
    }

    void LoadActor::SetScale(float x, float y, float z) {
        m_scale.set(x, y, z);
        if (m_node) m_node->setScale(m_scale);
    }

    void LoadActor::SetAnim(const std::string& animName, bool loop, float speed) {
        m_currentAnim = animName;
        m_animLoop = loop;
        m_animSpeed = speed;
        m_animTime = 0.0f;
        m_isPlaying = true;
    }

    void LoadActor::SetAnim(const std::string& animName, float speed) { SetAnim(animName, true, speed); }
    void LoadActor::StopAnim() { m_isPlaying = false; }

    void LoadActor::applyCpuSkinning() {
        for (auto& subMesh : m_subMeshes) {
            irr::scene::SMeshBuffer* buffer = subMesh.buffer;
            if (!buffer) continue;

            for (size_t i = 0; i < subMesh.originalVertices.size(); ++i) {
                const auto& v = subMesh.originalVertices[i];
                aiVector3D skinnedPos(0.0f, 0.0f, 0.0f);
                aiVector3D skinnedNormal(0.0f, 0.0f, 0.0f);
                aiVector3D origPos(v.position.X, v.position.Y, v.position.Z);
                aiVector3D origNorm(v.normal.X, v.normal.Y, v.normal.Z);

                float totalWeight = 0.0f;
                for (int j = 0; j < 4; ++j) {
                    int boneID = v.boneIDs[j];
                    float weight = v.weights[j];
                    if (boneID >= 0 && boneID < (int)m_boneTransforms.size() && weight > 0.0f) {
                        totalWeight += weight;
                    }
                }

                if (totalWeight > 0.001f) {
                    for (int j = 0; j < 4; ++j) {
                        int boneID = v.boneIDs[j];
                        float weight = v.weights[j];

                        if (boneID >= 0 && boneID < (int)m_boneTransforms.size() && weight > 0.0f) {
                            float normalizedWeight = weight / totalWeight;
                            const aiMatrix4x4& mat = m_boneTransforms[boneID];

                            skinnedPos += (mat * origPos) * normalizedWeight;
                            aiMatrix3x3 rotMat(mat);
                            skinnedNormal += (rotMat * origNorm) * normalizedWeight;
                        }
                    }
                } else {
                    skinnedPos = origPos;
                    skinnedNormal = origNorm;
                }

                buffer->Vertices[i].Pos.set(skinnedPos.x, skinnedPos.y, skinnedPos.z);
                buffer->Vertices[i].Normal.set(skinnedNormal.x, skinnedNormal.y, skinnedNormal.z);
            }

            buffer->recalculateBoundingBox();
            buffer->setDirty(irr::scene::EBT_VERTEX);
        }
    }

    void LoadActor::updateBoneHierarchy(const aiNode* node, const aiMatrix4x4& parentTransform, float animTime) {
        std::string nodeName = node->mName.C_Str();
        aiMatrix4x4 nodeTransform = node->mTransformation;

        if (m_rawScene && m_rawScene->mNumAnimations > 0) {
            const aiAnimation* anim = findAnimation(m_rawScene, m_currentAnim);

            if (anim && anim->mDuration > 0.0) {
                const aiNodeAnim* channel = nullptr;
                for (unsigned int i = 0; i < anim->mNumChannels; ++i) {
                    if (std::string(anim->mChannels[i]->mNodeName.C_Str()) == nodeName) {
                        channel = anim->mChannels[i];
                        break;
                    }
                }

                if (channel) {
                    double ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0;
                    double timeInTicks = animTime * ticksPerSecond;
                    double animationTime = m_animLoop ? fmod(timeInTicks, anim->mDuration) : std::min(timeInTicks, anim->mDuration);

                    aiVector3D defaultPos(node->mTransformation.a4, node->mTransformation.b4, node->mTransformation.c4);
                    aiVector3D defaultScale(1.0f, 1.0f, 1.0f);
                    aiQuaternion defaultRot;
                    node->mTransformation.Decompose(defaultScale, defaultRot, defaultPos);

                    aiVector3D pos = getAnimatedVector(channel->mPositionKeys, channel->mNumPositionKeys, animationTime, anim->mDuration, defaultPos);
                    aiQuaternion rot = getAnimatedQuaternion(channel->mRotationKeys, channel->mNumRotationKeys, animationTime, anim->mDuration, defaultRot);
                    aiVector3D scale = getAnimatedVector(channel->mScalingKeys, channel->mNumScalingKeys, animationTime, anim->mDuration, defaultScale);

                    aiMatrix4x4 posMat, rotMat, scaleMat;
                    aiMatrix4x4::Translation(pos, posMat);
                    rotMat = aiMatrix4x4(rot.GetMatrix());
                    aiMatrix4x4::Scaling(scale, scaleMat);

                    nodeTransform = posMat * rotMat * scaleMat;
                }
            }
        }

        aiMatrix4x4 globalTransform = parentTransform * nodeTransform;

        if (m_boneMapping.find(nodeName) != m_boneMapping.end()) {
            int boneID = m_boneMapping[nodeName].id;
            m_boneTransforms[boneID] = globalTransform * m_boneMapping[nodeName].offsetMatrix;
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            updateBoneHierarchy(node->mChildren[i], globalTransform, animTime);
        }
    }

    void LoadActor::Update(float deltaTime) {
        if (!m_isPlaying) return;

        m_animTime += deltaTime * m_animSpeed;

        aiMatrix4x4 identity;
        if (m_rawScene && m_rawScene->mRootNode) {
            updateBoneHierarchy(m_rawScene->mRootNode, identity, m_animTime);
        }
        applyCpuSkinning();
    }

    void LoadActor::Draw() {
        if (m_node) {
            m_node->setVisible(true);
        }
    }

    void LoadActor::cleanup() {
        if (m_node) {
            m_node->remove();
            m_node = nullptr;
        }

        if (m_mesh) {
            m_mesh->drop();
            m_mesh = nullptr;
        }

        if (m_rawScene) {
            delete m_rawScene;
            m_rawScene = nullptr;
        }

        m_subMeshes.clear();
        m_boneMapping.clear();
        m_boneTransforms.clear();
        m_isPlaying = false;
    }

} // namespace Vortex3D
