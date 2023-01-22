#ifndef SCENE_H
#define SCENE_H

#include <SceneObject.h>
#include <PointLight.h>
#include <Camera.h>
#include <Framebuffer.h>
#include <shader_structs.h>

#include <vector>
#include <memory>

namespace OM3D {

class Scene : NonMovable {

    public:
        Scene();

        static Result<std::unique_ptr<Scene>> from_gltf(const std::string& file_name);

        void render(const Camera& camera) const;
        void render_transparent(const Camera& camera, Texture &head_list, Texture &ll_buffer) const;
        void deferred_render(const Camera &camera) const;
        void point_lights_render(const Camera &camera, std::shared_ptr<StaticMesh> sphere_mesh) const;
        void tiled_render(const Camera &camera, glm::uvec2 window_size, size_t tile_size) const;

        void add_object(SceneObject obj);
        void add_object(PointLight obj);
        void order_objects_in_lists();
        const std::shared_ptr<StaticMesh> get_mesh(size_t obj_index) const;

        void force_transparency(std::shared_ptr<Program> prog, int group_index); 

    private:
        std::vector<SceneObject> _objects;
        std::vector<std::vector<size_t>> _transparentInstanceGroups;
        std::vector<std::vector<size_t>> _instanceGroups; 
        std::vector<PointLight> _point_lights;
        glm::vec3 _sun_direction = glm::vec3(0.2f, 1.0f, 0.1f);
        Framebuffer g_buffer;
        
};

}

#endif // SCENE_H
