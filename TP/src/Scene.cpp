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
                    light.intensity()};
            }
        }
        light_buffer.bind(BufferUsage::Storage, 1);

        // Draw instanced
        for (const std::vector<size_t> &instanceList : _instanceGroups)
        {
            size_t nb_instances_max = instanceList.size();

            // Init SSBO for models
            std::vector<glm::mat4> models;
            for (size_t i = 0; i < nb_instances_max; i++)
            {
                const SceneObject &obj = _objects[instanceList[i]];
                // Instance culling
                if (obj.is_visible(camera, frustum))
                    models.push_back(obj.transform());
            }
            size_t nb_instances = models.size();
            if (nb_instances == 0)
                continue;

            const std::shared_ptr<Material> material = _objects[instanceList[0]].get_material();
            const std::shared_ptr<StaticMesh> mesh = _objects[instanceList[0]].get_mesh();
            material->bind();
            mesh->bind_enable();

            TypedBuffer<glm::mat4> model_buffer(models.data(), nb_instances);
            model_buffer.bind(BufferUsage::Storage, 2);

            glDrawElementsInstanced(GL_TRIANGLES, int(mesh->get_index_buffer().element_count()), GL_UNSIGNED_INT, 0, nb_instances);
        }
    }

    void Scene::render_transparent(const Camera &camera, Texture &head_list, Texture &ll_buffer) const
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
                    light.intensity()};
            }
        }
        light_buffer.bind(BufferUsage::Storage, 1);

        glm::vec3 cam_pos = camera.position();
        const glm::vec3 const_cam_pos = glm::vec3(cam_pos.x, cam_pos.y, cam_pos.z);
        TypedBuffer<glm::vec3> camera_pos(&const_cam_pos, 1);
        camera_pos.bind(BufferUsage::Uniform, 1);

        // Bind image2D HeadTexture;
        head_list.bind_as_image(1, AccessType::ReadWrite);

        // Bind SSBO - ListNodes
        ll_buffer.bind_as_buffer(0);

        uint counter = 0;
        GLuint atomicsBuffer;
        ByteBuffer::bind_atomic_buffer(atomicsBuffer, counter);
        
        for (const std::vector<size_t> &instanceList : _transparentInstanceGroups)
        {
            for (const size_t &obj_index : instanceList)
            {
                _objects[obj_index].render(camera, frustum, false);
            }
        }
    }

    void Scene::point_lights_render(const Camera &camera, std::shared_ptr<StaticMesh> sphere_mesh) const
    {
        Frustum frustum = camera.build_frustum();

        // Fill and bind frame models buffer
        TypedBuffer<shader::FrameData> buffer(nullptr, 1);
        {
            auto mapping = buffer.map(AccessType::WriteOnly);
            mapping[0].camera.view_proj = camera.view_proj_matrix();
            mapping[0].camera.inv_view_proj = glm::inverse(camera.view_proj_matrix());
        }
        buffer.bind(BufferUsage::Uniform, 0);

        for (size_t i = 0; i < _point_lights.size(); i++)
        {
            // TODO compute only on point lights in the frustum
            const auto &pos = _point_lights[i].position();
            const auto &radius = _point_lights[i].radius();

            BoundingSphere bounds = {pos, radius};
            if (!bounds.is_visible(camera, frustum))
                continue;

            // Fill and bind light buffer
            TypedBuffer<shader::PointLight> light_buffer(nullptr, 1);
            {
                auto mapping = light_buffer.map(AccessType::WriteOnly);
                mapping[0] = {
                    pos,
                    radius,
                    _point_lights[i].color(),
                    _point_lights[i].intensity()};
            }
            light_buffer.bind(BufferUsage::Uniform, 1);

            glm::mat4 transform = glm::mat4(0.0f);
            // Scale
            transform[0][0] = radius;
            transform[1][1] = radius;
            transform[2][2] = radius;
            // Translation
            transform[0][3] = pos[0];
            transform[1][3] = pos[1];
            transform[2][3] = pos[2];
            // final one
            transform[3][3] = 1.0f;

            transform = glm::transpose(transform);
            TypedBuffer<glm::mat4> model_buffer(&transform, 1);
            model_buffer.bind(BufferUsage::Uniform, 2);

            sphere_mesh->draw();
        }
    }

    void add_object_in_group(std::vector<std::vector<size_t>> &groups, std::vector<SceneObject> &objects, const SceneObject &obj, const int i)
    {
        size_t index = 0;
        // Find the good object in vector if doesn't exist -> create new vector
        for (; index < groups.size(); index++)
        {
            if (objects[groups[index][0]].same_type(obj))
            {
                groups[index].emplace_back(i);
                break;
            }
        }

        if (index == groups.size())
        {
            std::vector<size_t> newVector = std::vector<size_t>();
            newVector.push_back(i);
            groups.push_back(newVector);
        }
    }

    void Scene::order_objects_in_lists()
    {
        // TODO Could be better to do this directly when the object is added to _objects list
        _instanceGroups = std::vector<std::vector<size_t>>();
        _transparentInstanceGroups = std::vector<std::vector<size_t>>();

        for (size_t i = 0; i < _objects.size(); i++)
        {
            const SceneObject &obj = _objects[i];

            if (obj.get_material()->is_transparent())
            {
                add_object_in_group(_transparentInstanceGroups, _objects, obj, i);
            }
            else
                add_object_in_group(_instanceGroups, _objects, obj, i);
        }
    }

    const std::shared_ptr<StaticMesh> Scene::get_mesh(size_t obj_index) const
    {
        return _objects[obj_index].get_mesh();
    }

    void Scene::force_transparency(std::shared_ptr<Program> prog, int group_index)
    {
        for (size_t i = 0; i < _instanceGroups[group_index].size(); i++)
        {
            _objects[_instanceGroups[group_index][i]].get_material()->set_blend_mode(BlendMode::Alpha);
            _objects[_instanceGroups[group_index][i]].get_material()->set_depth_mask(GL_FALSE);
            _objects[_instanceGroups[group_index][i]].get_material()->set_depth_test_mode(DepthTestMode::Reversed);
            _objects[_instanceGroups[group_index][i]].get_material()->set_program(prog);
        }
        this->order_objects_in_lists();
    }
}
