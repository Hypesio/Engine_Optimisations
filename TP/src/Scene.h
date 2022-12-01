#ifndef SCENE_H
#define SCENE_H

#include <SceneObject.h>
#include <PointLight.h>
#include <Camera.h>
#include <Framebuffer.h>

#include <vector>
#include <memory>

namespace OM3D {

class Scene : NonMovable {

    public:
        Scene();

        static Result<std::unique_ptr<Scene>> from_gltf(const std::string& file_name);

        void render(const Camera& camera) const;
        

        void add_object(SceneObject obj);
        void add_object(PointLight obj);
        void order_objects_in_lists(); 

    private:
        std::vector<SceneObject> _objects;
        std::vector<std::vector<size_t>> _instanceGroups; 
        std::vector<PointLight> _point_lights;
        glm::vec3 _sun_direction = glm::vec3(0.2f, 1.0f, 0.1f);
        Framebuffer g_buffer;
        
};

}

#endif // SCENE_H
