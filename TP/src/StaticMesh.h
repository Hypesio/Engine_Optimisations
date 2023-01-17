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

        bool is_visible(Camera camera, Frustum frustum)
        {
            // Frustum culling
            glm::vec3 dir = glm::vec3(glm::vec4(center_pos, 1.0)) - camera.position();
            float r = radius;

            return glm::dot(dir, frustum._bottom_normal) > -r && glm::dot(dir, frustum._top_normal) > -r && glm::dot(dir, frustum._near_normal) > -r && glm::dot(dir, frustum._left_normal) > -r && glm::dot(dir, frustum._right_normal) > -r;
        }
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
        const TypedBuffer<u32> &get_index_buffer() const
        {
            return _index_buffer;
        }

        BoundingSphere _bounding_sphere;

    private:
        TypedBuffer<Vertex> _vertex_buffer;
        TypedBuffer<u32> _index_buffer;
    };

}

#endif // STATICMESH_H
