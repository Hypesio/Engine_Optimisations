#include "SceneObject.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
namespace OM3D
{

    SceneObject::SceneObject(std::shared_ptr<StaticMesh> mesh, std::shared_ptr<Material> material) : _mesh(std::move(mesh)),
                                                                                                     _material(std::move(material))
    {
    }

    bool SceneObject::is_visible(Camera camera, Frustum frustum) const
    {
        // Frustum culling
        BoundingSphere bounding_sphere = _mesh->_bounding_sphere;
        glm::vec3 real_center = _position + glm::vec3(glm::vec4(bounding_sphere.center_pos, 1.0) * _transform);
        glm::vec3 dir = real_center - camera.position();
        float r = bounding_sphere.radius * glm::length(_scale);

        return glm::dot(dir, frustum._bottom_normal) > -r && glm::dot(dir, frustum._top_normal) > -r && glm::dot(dir, frustum._near_normal) > -r && glm::dot(dir, frustum._left_normal) > -r && glm::dot(dir, frustum._right_normal) > -r;
    }

    

    void SceneObject::render(Camera camera, Frustum frustum, bool front_and_back) const
    {
        if (!_material || !_mesh)
        {
            return;
        }

        TypedBuffer<glm::mat4> model_buffer(&transform(), 1);
        model_buffer.bind(BufferUsage::Storage, 2);
        
        
        if (this->is_visible(camera, frustum)) 
        {
            if (front_and_back)
            {
                _material->bind(CullMode::Frontface);
                _mesh->draw(); 
                _material->bind(CullMode::Backface);
                _mesh->draw();
            }
            else 
            {
                _material->bind();
                _mesh->draw();
            }
        }
    }

    void SceneObject::set_transform(const glm::mat4 &tr)
    {
        _transform = tr;
        _position = glm::vec3(tr[3][0], tr[3][1], tr[3][2]);
        _scale = glm::vec3(tr[0][0], tr[1][1], tr[2][2]);
    }

    const glm::mat4 &SceneObject::transform() const
    {
        return _transform;
    }

    bool SceneObject::same_type(const SceneObject &rhs)
    {
        // TODO improve by comparing vertex data
        return _material == rhs._material;
    }

}
