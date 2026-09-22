#include "LoadMdl.h"
#include <fstream>
#include <iostream>
#include <cstdint>

namespace Vortex3D {

    struct TempVertexBuffer {
        unsigned int numVertices = 0;
        unsigned int vertexSize = 0;
        unsigned int elementMask = 0;
        std::vector<unsigned char> data;
    };

    struct TempIndexBuffer {
        unsigned int numIndices = 0;
        unsigned int indexSize = 0;
        std::vector<unsigned char> data;
    };

    LoadMdl::LoadMdl(irr::scene::ISceneManager* smgr, irr::video::IVideoDriver* driver)
    : m_smgr(smgr), m_driver(driver) {}

    LoadMdl::~LoadMdl() {
        cleanup();
    }

    bool LoadMdl::Load(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[LoadMdl] Erreur : Impossible d'ouvrir " << filePath << std::endl;
            return false;
        }

        // 1. Identification de l'en-tête "UMDL"
        char magic[4];
        file.read(magic, 4);
        if (magic[0] != 'U' || magic[1] != 'M' || magic[2] != 'D' || magic[3] != 'L') {
            std::cerr << "[LoadMdl] Erreur : " << filePath << " n'est pas un fichier UMDL valide.\n";
            return false;
        }

        cleanup();
        m_mesh = new irr::scene::SMesh();

        // 2. Lecture des Vertex Buffers
        unsigned int numVertexBuffers = 0;
        file.read(reinterpret_cast<char*>(&numVertexBuffers), sizeof(unsigned int));

        std::vector<TempVertexBuffer> vbDescs(numVertexBuffers);

        for (unsigned int i = 0; i < numVertexBuffers; ++i) {
            auto& vb = vbDescs[i];
            file.read(reinterpret_cast<char*>(&vb.numVertices), sizeof(unsigned int));
            file.read(reinterpret_cast<char*>(&vb.elementMask), sizeof(unsigned int));

            unsigned int morphStart = 0, morphCount = 0;
            file.read(reinterpret_cast<char*>(&morphStart), sizeof(unsigned int));
            file.read(reinterpret_cast<char*>(&morphCount), sizeof(unsigned int));

            // Décodage du masque d'attributs binaire
            unsigned int vertexSize = 0;
            if (vb.elementMask & 1)   vertexSize += 12; // Position (Vector3)
            if (vb.elementMask & 2)   vertexSize += 12; // Normal (Vector3)
            if (vb.elementMask & 4)   vertexSize += 4;  // Color (UBYTE4)
            if (vb.elementMask & 8)   vertexSize += 8;  // TexCoord 0 (Vector2)
            if (vb.elementMask & 16)  vertexSize += 8;  // TexCoord 1 (Vector2)
            if (vb.elementMask & 32)  vertexSize += 12; // TexCoord3D 0 (Vector3)
            if (vb.elementMask & 64)  vertexSize += 12; // TexCoord3D 1 (Vector3)
            if (vb.elementMask & 128) vertexSize += 16; // Tangent (Vector4)
            if (vb.elementMask & 256) vertexSize += 16; // BlendWeights (Vector4)
            if (vb.elementMask & 512) vertexSize += 4;  // BlendIndices (UBYTE4)

            vb.vertexSize = vertexSize;
            vb.data.resize(vb.numVertices * vertexSize);
            file.read(reinterpret_cast<char*>(vb.data.data()), vb.data.size());
        }

        // 3. Lecture des Index Buffers
        unsigned int numIndexBuffers = 0;
        file.read(reinterpret_cast<char*>(&numIndexBuffers), sizeof(unsigned int));

        std::vector<TempIndexBuffer> ibDescs(numIndexBuffers);

        for (unsigned int i = 0; i < numIndexBuffers; ++i) {
            auto& ib = ibDescs[i];
            file.read(reinterpret_cast<char*>(&ib.numIndices), sizeof(unsigned int));
            file.read(reinterpret_cast<char*>(&ib.indexSize), sizeof(unsigned int));

            ib.data.resize(ib.numIndices * ib.indexSize);
            file.read(reinterpret_cast<char*>(ib.data.data()), ib.data.size());
        }

        // 4. Lecture des sous-maillages
        unsigned int numGeometries = 0;
        file.read(reinterpret_cast<char*>(&numGeometries), sizeof(unsigned int));

        for (unsigned int i = 0; i < numGeometries; ++i) {
            unsigned int boneMappingCount = 0;
            file.read(reinterpret_cast<char*>(&boneMappingCount), sizeof(unsigned int));
            if (boneMappingCount > 0) {
                file.seekg(boneMappingCount * sizeof(unsigned int), std::ios::cur);
            }

            unsigned int numLodLevels = 0;
            file.read(reinterpret_cast<char*>(&numLodLevels), sizeof(unsigned int));

            for (unsigned int j = 0; j < numLodLevels; ++j) {
                float lodDistance = 0.0f;
                unsigned int primitiveType = 0, vbRef = 0, ibRef = 0, drawStart = 0, drawCount = 0;

                file.read(reinterpret_cast<char*>(&lodDistance), sizeof(float));
                file.read(reinterpret_cast<char*>(&primitiveType), sizeof(unsigned int));
                file.read(reinterpret_cast<char*>(&vbRef), sizeof(unsigned int));
                file.read(reinterpret_cast<char*>(&ibRef), sizeof(unsigned int));
                file.read(reinterpret_cast<char*>(&drawStart), sizeof(unsigned int));
                file.read(reinterpret_cast<char*>(&drawCount), sizeof(unsigned int));

                // On extrait uniquement le niveau de détail principal (LOD 0)
                if (j == 0) {
                    irr::scene::SMeshBuffer* buffer = new irr::scene::SMeshBuffer();
                    const auto& vb = vbDescs[vbRef];
                    const auto& ib = ibDescs[ibRef];

                    // Extraction des sommets
                    for (unsigned int v = 0; v < vb.numVertices; ++v) {
                        const unsigned char* vertPtr = vb.data.data() + (v * vb.vertexSize);
                        size_t offset = 0;

                        irr::core::vector3df pos(0, 0, 0);
                        irr::core::vector3df norm(0, 1, 0);
                        irr::video::SColor color(255, 255, 255, 255);
                        irr::core::vector2df uv(0, 0);

                        if (vb.elementMask & 1) { // Position
                            const float* p = reinterpret_cast<const float*>(vertPtr + offset);
                            pos.set(p[0], p[1], p[2]);
                            offset += 12;
                        }
                        if (vb.elementMask & 2) { // Normal
                            const float* n = reinterpret_cast<const float*>(vertPtr + offset);
                            norm.set(n[0], n[1], n[2]);
                            offset += 12;
                        }
                        if (vb.elementMask & 4) { // Color
                            const unsigned char* c = vertPtr + offset;
                            color.set(c[3], c[0], c[1], c[2]);
                            offset += 4;
                        }
                        if (vb.elementMask & 8) { // TexCoord
                            const float* tex = reinterpret_cast<const float*>(vertPtr + offset);
                            uv.set(tex[0], tex[1]);
                            offset += 8;
                        }

                        buffer->Vertices.push_back(irr::video::S3DVertex(pos, norm, color, uv));
                    }

                    // Extraction des indices
                    if (ib.indexSize == sizeof(uint16_t)) {
                        const uint16_t* indices = reinterpret_cast<const uint16_t*>(ib.data.data()) + drawStart;
                        for (unsigned int idx = 0; idx < drawCount; ++idx) {
                            buffer->Indices.push_back(indices[idx]);
                        }
                    } else if (ib.indexSize == sizeof(uint32_t)) {
                        const uint32_t* indices = reinterpret_cast<const uint32_t*>(ib.data.data()) + drawStart;
                        for (unsigned int idx = 0; idx < drawCount; ++idx) {
                            buffer->Indices.push_back(static_cast<uint16_t>(indices[idx]));
                        }
                    }

                    buffer->recalculateBoundingBox();
                    m_mesh->addMeshBuffer(buffer);
                    buffer->drop();
                }
            }
        }

        m_mesh->recalculateBoundingBox();
        m_node = m_smgr->addMeshSceneNode(m_mesh);
        m_mesh->drop();
        m_mesh = nullptr;

        std::cout << "[LoadMdl] Modèle statique charge : " << filePath << std::endl;
        return true;
    }

    bool LoadMdl::LoadTexture(const std::string& texturePath) {
        if (!m_node) return false;
        irr::video::ITexture* texture = m_driver->getTexture(texturePath.c_str());
        if (!texture) return false;

        m_node->setMaterialTexture(0, texture);

        // 1. Désactive le calcul de la lumière native Irrlicht (évite le modèle noir)
        m_node->setMaterialFlag(irr::video::EMF_LIGHTING, false);

        // 2. Activer la correction pour les textures avec transparence
        m_node->setMaterialFlag(irr::video::EMF_BILINEAR_FILTER, true);

        return true;
    }

    void LoadMdl::SetPosition(float x, float y, float z) { if (m_node) m_node->setPosition({x, y, z}); }
    void LoadMdl::SetRotation(float pitch, float yaw, float roll) { if (m_node) m_node->setRotation({pitch, yaw, roll}); }
    void LoadMdl::SetScale(float x, float y, float z) { if (m_node) m_node->setScale({x, y, z}); }

    void LoadMdl::cleanup() {
        if (m_node) { m_node->remove(); m_node = nullptr; }
        if (m_mesh) { m_mesh->drop(); m_mesh = nullptr; }
    }

} // namespace Vortex3D
