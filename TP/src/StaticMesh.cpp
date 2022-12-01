#include "StaticMesh.h"

#include <glad/glad.h>

namespace OM3D {

StaticMesh::StaticMesh(const MeshData& data) :
    _vertex_buffer(data.vertices),
    _index_buffer(data.indices) {
    glm::vec3 origin = {0,0,0};
    for (size_t i = 0; i < data.vertices.size(); ++i) {
        origin += data.vertices[i].position;
    }
    origin /= data.vertices.size();

    float radius = 0;
    size_t radius_idx = -1;
    for (size_t i = 0; i < data.vertices.size(); ++i) {
        
        float dist = glm::distance(origin, data.vertices[i].position);
        if (radius < dist) {
            radius = dist;
            radius_idx = i;
        }
    }

    _bounding_sphere = {origin, radius};
}

bool StaticMesh::is_visible(Camera camera, Frustum frustum) {

    return true;
    // Frustum culling
    glm::vec3 dir = _bounding_sphere.center_pos - camera.position();
    float r = _bounding_sphere.radius;


    
    return glm::dot(dir, frustum._bottom_normal) > -r
        && glm::dot(dir, frustum._top_normal) > -r
        && glm::dot(dir, frustum._near_normal) > -r
        && glm::dot(dir, frustum._left_normal) > -r
        && glm::dot(dir, frustum._right_normal) > -r;
}

void StaticMesh::draw() const {
    _vertex_buffer.bind(BufferUsage::Attribute);
    _index_buffer.bind(BufferUsage::Index);

    // Vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
    // Vertex normal
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    // Vertex uv
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
    // Tangent / bitangent sign
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(8 * sizeof(float)));
    // Vertex color
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(12 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    glDrawElements(GL_TRIANGLES, int(_index_buffer.element_count()), GL_UNSIGNED_INT, nullptr);
}

void StaticMesh::bind_enable() const {
    _vertex_buffer.bind(BufferUsage::Attribute);
    _index_buffer.bind(BufferUsage::Index);

    // Vertex position
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), nullptr);
    // Vertex normal
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    // Vertex uv
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
    // Tangent / bitangent sign
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(8 * sizeof(float)));
    // Vertex color
    glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(12 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
}

}
