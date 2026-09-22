#include "LoadModel.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

namespace Vortex3D {

    LoadModel::LoadModel(irr::scene::ISceneManager* smgr, irr::video::IVideoDriver* driver)
    : m_smgr(smgr), m_driver(driver) {
    }

    LoadModel::~LoadModel() {
        cleanup();
    }

    bool LoadModel::loadObject(const std::string& modelPath) {
        if (!m_smgr || !m_driver) {
            std::cerr << "[LoadModel] Erreur : SceneManager ou VideoDriver non initialise.\n";
            return false;
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(modelPath,
                                                 aiProcess_Triangulate |
                                                 aiProcess_GenSmoothNormals |
                                                 aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "[LoadModel] Erreur Assimp : " << importer.GetErrorString() << "\n";
            return false;
        }

        // Nettoyage préalable si un modèle était déjà chargé
        cleanup();

        m_mesh = new irr::scene::SMesh();

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
            aiMesh* mesh = scene->mMeshes[m];
            irr::scene::SMeshBuffer* buffer = new irr::scene::SMeshBuffer();

            // Remplissage des sommets (Vertices)
            for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                irr::video::S3DVertex vertex;

                // Position
                vertex.Pos.X = mesh->mVertices[i].x;
                vertex.Pos.Y = mesh->mVertices[i].y;
                vertex.Pos.Z = mesh->mVertices[i].z;

                // Normales
                if (mesh->HasNormals()) {
                    vertex.Normal.X = mesh->mNormals[i].x;
                    vertex.Normal.Y = mesh->mNormals[i].y;
                    vertex.Normal.Z = mesh->mNormals[i].z;
                } else {
                    vertex.Normal.set(0.0f, 0.0f, 0.0f);
                }

                // Coordonnées de texture (UVs)
                if (mesh->HasTextureCoords(0)) {
                    vertex.TCoords.X = mesh->mTextureCoords[0][i].x;
                    vertex.TCoords.Y = mesh->mTextureCoords[0][i].y;
                } else {
                    vertex.TCoords.set(0.0f, 0.0f);
                }

                vertex.Color = irr::video::SColor(255, 255, 255, 255);
                buffer->Vertices.push_back(vertex);
            }

            // Remplissage des indices
            for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
                const aiFace& face = mesh->mFaces[i];
                if (face.mNumIndices == 3) {
                    buffer->Indices.push_back(face.mIndices[0]);
                    buffer->Indices.push_back(face.mIndices[1]);
                    buffer->Indices.push_back(face.mIndices[2]);
                }
            }

            buffer->recalculateBoundingBox();
            m_mesh->addMeshBuffer(buffer);
            buffer->drop(); // Transfert de la propriété du buffer vers m_mesh
        }

        if (m_mesh->getMeshBufferCount() == 0) {
            std::cerr << "[LoadModel] Erreur : Aucun sous-maillage genere.\n";
            m_mesh->drop();
            m_mesh = nullptr;
            return false;
        }

        m_mesh->recalculateBoundingBox();

        // Création du Scene Node dans Irrlicht
        m_node = m_smgr->addMeshSceneNode(m_mesh);
        if (!m_node) {
            std::cerr << "[LoadModel] Erreur : Impossible de creer le MeshSceneNode.\n";
            m_mesh->drop();
            m_mesh = nullptr;
            return false;
        }

        // Transfert de la propriété de m_mesh au nœud de scène
        m_mesh->drop();
        m_mesh = nullptr;

        // Désactivation de l'éclairage par défaut
        m_node->setMaterialFlag(irr::video::EMF_LIGHTING, false);

        m_isLoaded = true;
        return true;
    }

    bool LoadModel::setTexture(const std::string& texturePath) {
        if (!m_isLoaded || !m_node) {
            std::cerr << "[LoadModel] Erreur : Aucun objet charge pour appliquer la texture.\n";
            return false;
        }

        irr::video::ITexture* texture = m_driver->getTexture(texturePath.c_str());
        if (!texture) {
            std::cerr << "[LoadModel] Erreur chargement texture : " << texturePath << "\n";
            return false;
        }

        m_node->setMaterialTexture(0, texture);
        m_hasTexture = true;
        return true;
    }

    void LoadModel::setPosition(float x, float y, float z) {
        m_position.set(x, y, z);
        if (m_node) {
            m_node->setPosition(m_position);
        }
    }

    void LoadModel::setRotation(float pitch, float yaw, float roll) {
        m_rotation.set(pitch, yaw, roll);
        if (m_node) {
            m_node->setRotation(m_rotation);
        }
    }

    void LoadModel::setScale(float x, float y, float z) {
        m_scale.set(x, y, z);
        if (m_node) {
            m_node->setScale(m_scale);
        }
    }

    void LoadModel::draw() {
        if (m_node) {
            m_node->setVisible(true);
        }
    }

    void LoadModel::cleanup() {
        if (m_node) {
            m_node->remove(); // Supprime le nœud du SceneManager et libère le maillage associé
            m_node = nullptr;
        }

        if (m_mesh) {
            m_mesh->drop();
            m_mesh = nullptr;
        }

        m_isLoaded = false;
        m_hasTexture = false;
    }

} // namespace Vortex3D
