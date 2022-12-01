#include "SceneObject.h"

#include <glm/gtc/matrix_transform.hpp>

namespace OM3D
{

    SceneObject::SceneObject(std::shared_ptr<StaticMesh> mesh, std::shared_ptr<Material> material) : _mesh(std::move(mesh)),
                                                                                                     _material(std::move(material))
    {
    }

    void SceneObject::render(Camera camera, Frustum frustum) const
    {
        if (!_material || !_mesh)
        {
            return;
        }

        _material->set_uniform(HASH("model"), transform());
        _material->bind();
        if (_mesh->is_visible(camera, frustum))
            _mesh->draw();
    }

    void SceneObject::set_transform(const glm::mat4 &tr)
    {
        _transform = tr;
    }

    const glm::mat4 &SceneObject::transform() const
    {
        return _transform;
    }

    float SceneObject::get_scale() {
        // TODO
    }

    bool SceneObject::same_type(const SceneObject& rhs) {
        // TODO improve by comparing vertex data
        return _material == rhs._material;
    }

}