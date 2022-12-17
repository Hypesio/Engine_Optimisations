#include "Scene.h"

#include <TypedBuffer.h>

#include <shader_structs.h>

#include <glad/glad.h>
#include <iostream>

namespace OM3D
{

    Scene::Scene()
    {
    }

    void Scene::add_object(SceneObject obj)
    {
        _objects.emplace_back(std::move(obj));
    }

    void Scene::add_object(PointLight obj)
    {
        _point_lights.emplace_back(std::move(obj));
    }

    void Scene::deferred_render(const Camera &camera) const 
    {
        // Fill and bind frame models buffer
        TypedBuffer<shader::FrameData> buffer(nullptr, 1);
        {
            auto mapping = buffer.map(AccessType::WriteOnly);
            mapping[0].camera.view_proj = camera.view_proj_matrix();
            mapping[0].camera.inv_view_proj = glm::inverse(camera.view_proj_matrix());
            mapping[0].point_light_count = u32(_point_lights.size());
            mapping[0].sun_color = glm::vec3(1.0f, 1.0f, 1.0f);
            mapping[0].sun_dir = glm::normalize(_sun_direction);
        }
        buffer.bind(BufferUsage::Uniform, 0);

        // Fill and bind lights buffer
        TypedBuffer<shader::PointLight> light_buffer(nullptr, std::max(_point_lights.size(), size_t(1)));
        {
            auto mapping = light_buffer.map(AccessType::WriteOnly);
            for (size_t i = 0; i != _point_lights.size(); ++i)
            {
                const auto &light = _point_lights[i];
                mapping[i] = {
                    light.position(),
                    light.radius(),
                    light.color(),
                    0.0f};
            }
        }
        light_buffer.bind(BufferUsage::Storage, 1);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void Scene::render(const Camera &camera) const
    {
        Frustum frustum = camera.build_frustum();

        // Fill and bind frame models buffer
        TypedBuffer<shader::FrameData> buffer(nullptr, 1);
        {
            auto mapping = buffer.map(AccessType::WriteOnly);
            mapping[0].camera.view_proj = camera.view_proj_matrix();
            mapping[0].point_light_count = u32(_point_lights.size());
            mapping[0].sun_color = glm::vec3(1.0f, 1.0f, 1.0f);
            mapping[0].sun_dir = glm::normalize(_sun_direction);
        }
        buffer.bind(BufferUsage::Uniform, 0);

        // Fill and bind lights buffer
        TypedBuffer<shader::PointLight> light_buffer(nullptr, std::max(_point_lights.size(), size_t(1)));
        {
            auto mapping = light_buffer.map(AccessType::WriteOnly);
            for (size_t i = 0; i != _point_lights.size(); ++i)
            {
                const auto &light = _point_lights[i];
                mapping[i] = {
                    light.position(),
                    light.radius(),
                    light.color(),
                    0.0f};
            }
        }
        light_buffer.bind(BufferUsage::Storage, 1);

        // Draw instanced
        for (const std::vector<size_t> &instanceList : _instanceGroups)
        {
            size_t nb_instances_max = instanceList.size();

            const std::shared_ptr<Material> material = _objects[instanceList[0]].get_material();
            const std::shared_ptr<StaticMesh> mesh = _objects[instanceList[0]].get_mesh();
            
            // Init SSBO for models
            std::vector<glm::mat4> models;
            for (size_t i = 0; i < nb_instances_max; i++)
            {
                const SceneObject& obj = _objects[instanceList[i]];
                // Instance culling
                if (obj.is_visible(camera, frustum))
                    models.push_back(obj.transform());
            }
            size_t nb_instances = models.size();
            if (nb_instances == 0)
                continue;

            TypedBuffer<glm::mat4> model_buffer(models.data(), nb_instances);
            model_buffer.bind(BufferUsage::Storage, 2);

            material->bind();
            mesh->bind_enable();

            glDrawElementsInstanced(GL_TRIANGLES, int(mesh->get_index_buffer().element_count()), GL_UNSIGNED_INT, 0, nb_instances);
        }
    }

    void Scene::order_objects_in_lists()
    {

        // TODO Could be better to do this directly when the object is added to _objects list
        _instanceGroups = std::vector<std::vector<size_t>>();

        for (size_t i = 0; i < _objects.size(); i++)
        {
            const SceneObject &obj = _objects[i];
            // Find the good object in vector if doesn't exist -> create new vector
            size_t index = 0;
            for (; index < _instanceGroups.size(); index++)
            {
                if (_objects[_instanceGroups[index][0]].same_type(obj))
                {
                    _instanceGroups[index].emplace_back(i);
                    break;
                }
            }

            if (index == _instanceGroups.size())
            {
                std::vector<size_t> newVector = std::vector<size_t>();
                newVector.push_back(i);
                _instanceGroups.push_back(newVector);
            }
        }
    }

}
