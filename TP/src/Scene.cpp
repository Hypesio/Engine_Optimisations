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
                    0.0f};
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
                    0.0f};
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
                _objects[obj_index].render(camera, frustum);
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
            const auto &pos = _point_lights[i].position();
            const auto &radius = _point_lights[i].radius();

            // Compute only on point lights in the frustum
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
                    0.0f};
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

    void Scene::tiled_render(const Camera &camera, glm::uvec2 window_size, size_t tile_size) const {

        glm::vec3 camera_forward = camera.forward();
        glm::vec3 camera_up = camera.up();
        glm::vec3 camera_right = camera.right();

        float fov_y = camera.fov();
        float fov_x = std::atan(std::tan(fov_y) * camera.aspect_ratio());

        float n_x_tiles = window_size.x / tile_size;
        float n_y_tiles = window_size.y / tile_size;

        std::vector<uint> plights_indices;
        std::vector<uint> indices;
        uint counter = 0;

        for (size_t i = 0; i < window_size.y; i += tile_size)
        {
            float top_fov = (fov_y * 0.5f) * -(1.0f - (((float(i) / float(tile_size) + 1.0f) * 2.0f) / n_y_tiles));  
            float bottom_fov = (fov_y * 0.5f) * (1.0f - ((float(i) / float(tile_size) * 2.0f) / n_y_tiles));
            
            glm::vec3 top_normal = camera_forward * std::sin(top_fov) - camera_up * std::cos(top_fov);
            glm::vec3 bottom_normal = camera_forward * std::sin(bottom_fov) + camera_up * std::cos(bottom_fov);

            for (size_t j = 0; j < window_size.x; j += tile_size)
            {
                float left_fov = (fov_x * 0.5f) * (1.0f - ((float(j) / float(tile_size) * 2.0f) / n_x_tiles));
                //std::cout << "ref: " << (fov_x * 0.5f) << "; get: " << left_fov << "; j: " << j << std::endl;
                float right_fov = (fov_x * 0.5f) * -(1.0f - (((float(j) / float(tile_size) + 1.0f) * 2.0f) / n_x_tiles));

                glm::vec3 left_normal = camera_forward * std::sin(left_fov) + camera_right * std::cos(left_fov);
                glm::vec3 right_normal = camera_forward * std::sin(right_fov) - camera_right * std::cos(right_fov);
                //std::cout << "right: " << right_normal.x << ", " << right_normal.y << ", " << right_normal.z << std::endl;

                Frustum frustum = { 
                    camera_forward, // Stays the same
                    top_normal, 
                    bottom_normal, 
                    right_normal, 
                    left_normal
                };

                /*if (i == 0 && j == 0) {
                    std::cout << "top : " << frustum._top_normal.x << "," << frustum._top_normal.y << "," << frustum._top_normal.z << std::endl
                              << "bottom : " << frustum._bottom_normal.x << "," << frustum._bottom_normal.y << "," << frustum._bottom_normal.z << std::endl
                              << "left : " << frustum._left_normal.x << "," << frustum._left_normal.y << "," << frustum._left_normal.z << std::endl
                              << "right : " << frustum._right_normal.x << "," << frustum._right_normal.y << "," << frustum._right_normal.z << std::endl
                              << std::endl;
                }*/
                
                for (size_t l = 0; l < _point_lights.size(); l++)
                {
                    const auto &pos = _point_lights[l].position();
                    const auto &radius = _point_lights[l].radius();

                    // Find the lights that alter the tile
                    BoundingSphere bounds = {pos, radius};
                    if (bounds.is_visible(camera, frustum)) {
                        plights_indices.push_back(l);
                        counter++;
                    }
                }

                indices.push_back(counter);
            }
        }

        // Bind everything for the compute

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

        TypedBuffer<uint> indices_buffer(nullptr, std::max(indices.size(), size_t(1)));
        {
            auto mapping = indices_buffer.map(AccessType::WriteOnly);
            for (size_t i = 0; i != indices.size(); ++i)
                mapping[i] = indices[i];
        }
        indices_buffer.bind(BufferUsage::Storage, 2);
        TypedBuffer<uint> plights_indices_buffer(nullptr, std::max(plights_indices.size(), size_t(1)));
        {
            auto mapping = plights_indices_buffer.map(AccessType::WriteOnly);
            for (size_t i = 0; i != plights_indices.size(); ++i)
                mapping[i] = plights_indices[i];
        }
        plights_indices_buffer.bind(BufferUsage::Storage, 3);

        glDispatchCompute(align_up_to(window_size.x, 8) / 8, align_up_to(window_size.y, 8) / 8, 1);
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

    void Scene::force_transparency(std::shared_ptr<Program> prog)
    {
        for (size_t i = 0; i < _instanceGroups[1].size(); i++)
        {
            _objects[_instanceGroups[1][i]].get_material()->set_blend_mode(BlendMode::Alpha);
            _objects[_instanceGroups[1][i]].get_material()->set_depth_mask(GL_FALSE);
            _objects[_instanceGroups[1][i]].get_material()->set_depth_test_mode(DepthTestMode::Reversed);
            _objects[_instanceGroups[1][i]].get_material()->set_program(prog);
        }
        this->order_objects_in_lists();
    }
}
