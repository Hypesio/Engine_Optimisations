#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include "Camera.h"

#include <StaticMesh.h>
#include <Material.h>

#include <memory>

#include <glm/matrix.hpp>

namespace OM3D {

class SceneObject : NonCopyable {

    public:
        SceneObject(std::shared_ptr<StaticMesh> mesh = nullptr, std::shared_ptr<Material> material = nullptr);

        void render(Camera camera, Frustum frustum, bool front_and_back = false) const;

        void set_transform(const glm::mat4& tr);
        const glm::mat4& transform() const;
        bool same_type(const SceneObject& rhs);
        
        const std::shared_ptr<Material> get_material() const {
            return _material;
        } 

        void set_material(const std::shared_ptr<Material> new_mat) {
            _material = new_mat;
        } 

        const std::shared_ptr<StaticMesh> get_mesh() const {
            return _mesh;
        } 

        bool is_visible(Camera camera, Frustum frustum) const;

    private:
        glm::mat4 _transform = glm::mat4(1.0f);
        glm::vec3 _position = glm::vec3(0, 0, 0);
        glm::vec3 _scale = glm::vec3(1, 1, 1);

        std::shared_ptr<StaticMesh> _mesh;
        std::shared_ptr<Material> _material;
};


}

#endif // SCENEOBJECT_H
