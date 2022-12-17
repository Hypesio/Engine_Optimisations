#ifndef STATICMESH_H
#define STATICMESH_H

#include "Camera.h"

#include <graphics.h>
#include <TypedBuffer.h>
#include <Vertex.h>

#include <vector>

namespace OM3D
{

    struct MeshData
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;
    };

    struct BoundingSphere
    {
        glm::vec3 center_pos;
        float radius;
    };

    class StaticMesh : NonCopyable
    {

    public:
        StaticMesh() = default;
        StaticMesh(StaticMesh &&) = default;
        StaticMesh &operator=(StaticMesh &&) = default;

        StaticMesh(const MeshData &data);
        
        void draw() const;
        void bind_enable() const;
        const TypedBuffer<u32>& get_index_buffer() const {
            return _index_buffer;
        }

        BoundingSphere _bounding_sphere;

    private:
        TypedBuffer<Vertex> _vertex_buffer;
        TypedBuffer<u32> _index_buffer;
    };

}

#endif // STATICMESH_H
